#include <uf/engine/asset/asset.h>
#include <regex>
#include <functional>
#include <iomanip>
#include <sys/stat.h>
#include <fstream>
#include <iostream>

#include <uf/utils/image/image.h>
#include <uf/utils/audio/audio.h>
#include <uf/utils/serialize/serializer.h>
#include <uf/utils/string/ext.h>
#include <uf/utils/string/io.h>
#include <uf/utils/string/hash.h>
#include <uf/utils/thread/thread.h>
#include <uf/ext/lua/lua.h>
#include <uf/ext/gltf/gltf.h>
#include <uf/engine/graph/graph.h>

#include <mutex>

namespace {
	bool retrieve( const uf::stl::string& url, const uf::stl::string& filename, const uf::stl::string& hash = "" ) {
		uf::Http http = uf::http::get( url );
		if ( http.code < 200 || http.code > 300 ) {
			UF_MSG_ERROR("HTTP Error {} on GET {}", http.code, url);
			return false;
		}

		uf::stl::string actual = hash;
		if ( hash != "" && (actual = uf::string::sha256(url)) != hash ) {
			UF_MSG_ERROR("HTTP hash mismatch on GET {}: expected {}, got {}", url, hash, actual);
			return false;
		}

		uf::io::write( "./" + filename, http.response );
		return true;
	}

	std::mutex mutex;
	uint64_t uid = 0;
	struct Job {
		typedef uf::stl::vector<Job> container_t;

		uf::stl::string callback;
		uf::stl::string type;
		uf::Asset::Payload payload;
	};
}

uf::Asset uf::Asset::masterAssetLoader;
bool uf::Asset::assertionLoad = true;

#define UF_ASSET_MULTITHREAD 1

void uf::Asset::processQueue() {
#if UF_ASSET_MULTITHREAD
	auto tasks = uf::thread::schedule(true, false);
#else
	auto tasks = uf::thread::schedule(false);
#endif

	mutex.lock();
	auto jobs = std::move(this->getComponent<Job::container_t>());
	mutex.unlock();

	for ( auto& job : jobs ) tasks.queue([=, this]{ // C++20 retardation
		uf::stl::string callback = job.callback;
		uf::stl::string type = job.type;
		uf::Asset::Payload payload = job.payload;

		if ( payload.filename == "" || callback == "" ) return;

		uf::stl::string filename = type == "cache" ? this->cache(payload) : this->load(payload);
			
		if ( callback != "" && filename != "" ) uf::hooks.call(callback, payload);
	});

//	bool bitch = !tasks.container.empty();
//	if ( bitch ) UF_MSG_DEBUG("Batching Jobs: {}", tasks.container.size());
	uf::thread::execute( tasks );
//	if ( bitch ) UF_MSG_DEBUG("Finished Jobs: {}", tasks.container.size());
}
void uf::Asset::cache( const uf::stl::string& callback, const uf::Asset::Payload& payload ) {
	mutex.lock();
	auto& jobs = this->getComponent<Job::container_t>();
	jobs.emplace_back(Job{ callback, "cache", payload });
	mutex.unlock();
}
void uf::Asset::load( const uf::stl::string& callback, const uf::Asset::Payload& payload ) {
	mutex.lock();
	auto& jobs = this->getComponent<Job::container_t>();
	jobs.emplace_back(Job{ callback, "load", payload });
	mutex.unlock();
}

uf::Asset::Payload uf::Asset::resolveToPayload( const uf::stl::string& uri, const uf::stl::string& mime ) {
	uf::stl::string extension = uf::string::lowercase( uf::io::extension( uri, -1 ) );
	uf::stl::string basename = uf::string::lowercase( uf::string::replace( uf::io::filename( uri ), "/.(?:gz|lz4?)$/", "" ) );
	uf::Asset::Payload payload;

	static uf::stl::unordered_map<uf::stl::string,uf::Asset::Type> typemap = {
		{ "jpg", 	uf::Asset::Type::IMAGE },
		{ "jpeg", 	uf::Asset::Type::IMAGE },
		{ "png", 	uf::Asset::Type::IMAGE },
		
		{ "ogg", 	uf::Asset::Type::AUDIO },

		{ "json", 	uf::Asset::Type::JSON },
		{ "bson", 	uf::Asset::Type::JSON },
		{ "cbor", 	uf::Asset::Type::JSON },
		{ "msgpack",uf::Asset::Type::JSON },
		{ "ubjson", uf::Asset::Type::JSON },
		{ "bjdata", uf::Asset::Type::JSON },
		{ "toml", uf::Asset::Type::JSON },

		{ "lua", 	uf::Asset::Type::LUA },
		
		{ "glb",  	uf::Asset::Type::GRAPH },
	#if !UF_USE_OPENGL
		{ "gltf", 	uf::Asset::Type::GRAPH },
		{ "mdl",  	uf::Asset::Type::GRAPH },
	#endif
	};

	payload.filename = uri;
	payload.mime = mime;

	if ( typemap.count( extension ) == 1 ) payload.type = typemap[extension];
	if ( basename == "graph.json" ) payload.type = uf::Asset::Type::GRAPH;
	else if ( basename == "graph.bson" ) payload.type = uf::Asset::Type::GRAPH;
	else if ( basename == "graph.cbor" ) payload.type = uf::Asset::Type::GRAPH;
	else if ( basename == "graph.msgpack" ) payload.type = uf::Asset::Type::GRAPH;
	else if ( basename == "graph.ubjson" ) payload.type = uf::Asset::Type::GRAPH;
	else if ( basename == "graph.bjdata" ) payload.type = uf::Asset::Type::GRAPH;
	else if ( basename == "graph.toml" ) payload.type = uf::Asset::Type::GRAPH;

	return payload;
}

bool uf::Asset::isExpected( const uf::Asset::Payload& payload, uf::Asset::Type expected ) {
	if ( payload.filename == "" ) return false;
	if ( payload.type != expected ) return false;
	return true;
}

uf::stl::string uf::Asset::cache( const uf::Asset::Payload& payload ) {
	uf::stl::string filename = payload.filename;
	uf::stl::string extension = uf::io::extension( filename );
	if ( filename.substr(0,5) == "https" ) {
		uf::stl::string hash = uf::string::sha256( filename );
		uf::stl::string cached = uf::io::root + "/cache/http/" + hash + "." + extension;
		if ( !uf::io::exists( cached ) && !retrieve( filename, cached, hash ) ) {
			if ( !uf::Asset::assertionLoad ) {
				UF_MSG_ERROR("Failed to preload {} ({}): HTTP error", filename, cached);
			} else {
				UF_EXCEPTION("Failed to preload {} ({}): HTTP error", filename, cached);
			}
			return "";
		}
		filename = cached;
	} else {
		// do implicit loading of json files (could be encoded as bson, cbor, and compressed as gz, lz4)
		if ( extension == "json" ) {
			filename = uf::Serializer::resolveFilename( filename );
			extension = uf::io::extension( extension );
		}
	}
	if ( !uf::io::exists( filename ) ) {
		if ( !uf::Asset::assertionLoad ) {
			UF_MSG_ERROR("Failed to preload {}, does not exist", filename);
		} else {
			UF_EXCEPTION("Failed to preload {}, does not exist", filename);
		}
		return "";
	}
	uf::stl::string actual = payload.hash;
	if ( payload.hash != "" && (actual = uf::io::hash( filename )) != payload.hash ) {
		if ( !uf::Asset::assertionLoad ) {
			UF_MSG_ERROR("Failed to preload {}: Hash mismatch; expected {}, got {}", filename, payload.hash, actual);
		} else {
			UF_EXCEPTION("Failed to preload {}: Hash mismatch; expected {}, got {}", filename, payload.hash, actual);
		}
		return "";
	}
#if UF_ENV_DREAMCAST
	UF_MSG_DEBUG("Preloading {}", filename);
	DC_STATS();
#endif
	return filename;
}
uf::stl::string uf::Asset::load(const uf::Asset::Payload& payload ) {
	uf::stl::string filename = payload.filename;
	uf::stl::string extension = uf::string::lowercase(uf::io::extension( payload.filename, -1 ));
	uf::stl::string basename = uf::string::lowercase( uf::string::replace( uf::io::filename( payload.filename ), "/.(?:gz|lz4?)$/", "" ) );
	if ( payload.filename.substr(0,5) == "https" ) {
		uf::stl::string hash = uf::string::sha256( payload.filename );
		uf::stl::string cached = uf::io::root + "/cache/http/" + hash + "." + extension;
		if ( !uf::io::exists( cached ) && !retrieve( payload.filename, cached, hash ) ) {
			if ( !uf::Asset::assertionLoad ) {
				UF_MSG_ERROR("Failed to load {} ({}): HTTP error", payload.filename, cached);
			} else {
				UF_EXCEPTION("Failed to load {} ({}): HTTP error", payload.filename, cached);
			}
			return "";
		}
		filename = cached;
	} else {
		// do implicit loading of json files (could be encoded as bson, cbor, and compressed as gz, lz4)
		if ( extension == "json" ) {
			filename = uf::Serializer::resolveFilename( filename );
			extension = uf::io::extension( extension );
		}
	}
	if ( !uf::io::exists( filename ) ) {
		if ( !uf::Asset::assertionLoad ) {
			UF_MSG_ERROR("Failed to load {}: does not exist", filename);
		} else {
			UF_EXCEPTION("Failed to load {}: does not exist", filename);
		}
		return "";
	}
	uf::stl::string actual = payload.hash;
	if ( payload.hash != "" && (actual = uf::io::hash( filename )) != payload.hash ) {
		if ( !uf::Asset::assertionLoad ) {
			UF_MSG_ERROR("Failed to load {}: Hash mismatch; expected {}, got {}", filename, payload.hash, actual);
		} else {
			UF_EXCEPTION("Failed to load {}: Hash mismatch; expected {}, got {}", filename, payload.hash, actual);
		}
		return "";
	}

#if UF_ENV_DREAMCAST
	UF_MSG_DEBUG("Loading {}", filename);
	DC_STATS();
#endif

	auto& map = this->getComponent<uf::Serializer>();
	#define UF_ASSET_REGISTER(type)\
		auto& container = this->getContainer<type>();\
		if ( !ext::json::isNull( map[extension][payload.filename]["index"] ) ) return filename;\
		if ( !ext::json::isNull( map[extension][filename]["index"] ) ) return filename;\
		map[extension][payload.filename]["index"] = container.size();\
		map[extension][filename]["index"] = container.size();\
		type& asset = container.emplace_back();


	switch ( payload.type ) {
		case uf::Asset::Type::IMAGE: {
			UF_ASSET_REGISTER(uf::Image)
			asset.open(filename);
		} break;
		case uf::Asset::Type::AUDIO: {
			UF_ASSET_REGISTER(uf::Audio)
			asset.open(filename, true);
		} break;
		case uf::Asset::Type::JSON: {
			UF_ASSET_REGISTER(uf::Serializer)
			asset.readFromFile(filename);
		} break;
	#if UF_USE_LUA
		case uf::Asset::Type::LUA: {
			UF_ASSET_REGISTER(pod::LuaScript)
			asset = ext::lua::script( filename );
		} break;
	#endif
		case uf::Asset::Type::GRAPH: {
			UF_ASSET_REGISTER(pod::Graph)
			auto& metadata = this->getComponent<uf::Serializer>();

		#if UF_USE_OPENGL
			// combining mesh is only really a (negligent) gain on Vulkan
			// collision meshes still use separated meshes, so avoid having to duplicate a mesh for very little gains, if any
			metadata[payload.filename]["renderer"]["atlas"] = false;
			metadata[payload.filename]["renderer"]["separate"] = true;
		#endif
			asset = uf::graph::load( filename, metadata[payload.filename] );
			uf::graph::process( asset );
		#if !UF_ENV_DREAMCAST
			if ( asset.metadata["debug"]["print"]["stats"].as<bool>() ) UF_MSG_INFO("{}", uf::graph::stats( asset ));
			if ( asset.metadata["debug"]["print"]["tree"].as<bool>() ) UF_MSG_INFO("{}", uf::graph::print( asset ));
		#endif
			if ( !asset.metadata["debug"]["no cleanup"].as<bool>() ) uf::graph::cleanup( asset );
		} break;
		default: {
			UF_MSG_ERROR("Failed to parse {}: unimplemented extension: {}", filename, extension );
		}
	}

#if UF_ENV_DREAMCAST
	UF_MSG_DEBUG("Loaded {}", filename);
	DC_STATS();
#endif

	return filename;
}
uf::stl::string uf::Asset::getOriginal( const uf::stl::string& uri ) {
	uf::stl::string extension = uf::io::extension( uri );
	auto& map = this->getComponent<uf::Serializer>();
	if ( ext::json::isNull( map[extension][uri]["index"] ) ) return uri;
	std::size_t index = map[extension][uri]["index"].as<size_t>();

	uf::stl::string key = uri;
	ext::json::forEach( map[extension], [&]( const uf::stl::string& k, ext::json::Value& v ) {
		std::size_t i = v["index"].as<size_t>();
		if ( index == i && key != uri ) key = k;
	});
	return key;
}