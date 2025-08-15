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
		uf::asset::Payload payload;
	};
}

// uf::asset uf::asset::masterAssetLoader;
bool uf::asset::assertionLoad = true;
uf::asset::Job::container_t uf::asset::jobs;
uf::stl::unordered_map<uf::stl::string, uf::asset::userdata_t> uf::asset::map;
uf::Serializer uf::asset::metadata;

#define UF_ASSET_MULTITHREAD 1

void uf::asset::processQueue() {
#if UF_ASSET_MULTITHREAD
	auto tasks = uf::thread::schedule(true, false);
#else
	auto tasks = uf::thread::schedule(false);
#endif

	mutex.lock();
	auto jobs = std::move(uf::asset::jobs);
	mutex.unlock();

	for ( auto& job : jobs ) tasks.queue([=]{
		uf::stl::string callback = job.callback;
		uf::stl::string type = job.type;
		uf::asset::Payload payload = job.payload;

		if ( payload.filename == "" || callback == "" ) return;

		uf::stl::string filename = type == "cache" ? uf::asset::cache(payload) : uf::asset::load(payload);
		
		if ( callback != "" && filename != "" ) {
			uf::hooks.call(callback, payload);
		}
	});

	uf::thread::execute( tasks );
}
void uf::asset::cache( const uf::stl::string& callback, const uf::asset::Payload& payload ) {
	mutex.lock();
	auto& jobs = uf::asset::jobs;
	jobs.emplace_back(Job{ callback, "cache", payload });
	mutex.unlock();
}
void uf::asset::load( const uf::stl::string& callback, const uf::asset::Payload& payload ) {
	mutex.lock();
	auto& jobs = uf::asset::jobs;
	jobs.emplace_back(Job{ callback, "load", payload });
	mutex.unlock();
}

uf::asset::Payload uf::asset::resolveToPayload( const uf::stl::string& uri, const uf::stl::string& mime ) {
	uf::stl::string extension = uf::string::lowercase( uf::io::extension( uri, -1 ) );
	uf::stl::string basename = uf::string::lowercase( uf::string::replace( uf::io::filename( uri ), "/.(?:gz|lz4?)$/", "" ) );
	uf::asset::Payload payload;

	static uf::stl::unordered_map<uf::stl::string,uf::asset::Type> typemap = {
		{ "jpg", 	uf::asset::Type::IMAGE },
		{ "jpeg", 	uf::asset::Type::IMAGE },
		{ "png", 	uf::asset::Type::IMAGE },
		
		{ "ogg", 	uf::asset::Type::AUDIO },
		{ "wav", 	uf::asset::Type::AUDIO },

		{ "json", 	uf::asset::Type::JSON },
		{ "bson", 	uf::asset::Type::JSON },
		{ "cbor", 	uf::asset::Type::JSON },
		{ "msgpack",uf::asset::Type::JSON },
		{ "ubjson", uf::asset::Type::JSON },
		{ "bjdata", uf::asset::Type::JSON },
		{ "toml", uf::asset::Type::JSON },

		{ "lua", 	uf::asset::Type::LUA },
		
		{ "glb",  	uf::asset::Type::GRAPH },
	#if !UF_USE_OPENGL
		{ "gltf", 	uf::asset::Type::GRAPH },
		{ "mdl",  	uf::asset::Type::GRAPH },
	#endif
	};

	payload.filename = uri;
	payload.uri = uri;
	payload.mime = mime;

	if ( payload.filename.substr(0,5) != "https" && extension == "json" ) {
		payload.filename = uf::Serializer::resolveFilename( payload.filename );
	}

	if ( typemap.count( extension ) == 1 ) payload.type = typemap[extension];
	if ( basename == "graph.json" ) payload.type = uf::asset::Type::GRAPH;
	else if ( basename == "graph.bson" ) payload.type = uf::asset::Type::GRAPH;
	else if ( basename == "graph.cbor" ) payload.type = uf::asset::Type::GRAPH;
	else if ( basename == "graph.msgpack" ) payload.type = uf::asset::Type::GRAPH;
	else if ( basename == "graph.ubjson" ) payload.type = uf::asset::Type::GRAPH;
	else if ( basename == "graph.bjdata" ) payload.type = uf::asset::Type::GRAPH;
	else if ( basename == "graph.toml" ) payload.type = uf::asset::Type::GRAPH;

	return payload;
}

bool uf::asset::isExpected( const uf::asset::Payload& payload, uf::asset::Type expected ) {
	if ( payload.filename == "" ) return false;
	if ( payload.type != expected ) return false;
	return true;
}

uf::stl::string uf::asset::cache( uf::asset::Payload& payload ) {
	uf::stl::string filename = payload.filename;
	uf::stl::string extension = uf::io::extension( filename );
	if ( filename.substr(0,5) == "https" ) {
		uf::stl::string hash = uf::string::sha256( filename );
		uf::stl::string cached = uf::io::root + "/cache/http/" + hash + "." + extension;
		if ( !uf::io::exists( cached ) && !retrieve( filename, cached, hash ) ) {
			if ( !uf::asset::assertionLoad ) {
				UF_MSG_ERROR("Failed to preload {} ({}): HTTP error", filename, cached);
			} else {
				UF_EXCEPTION("Failed to preload {} ({}): HTTP error", filename, cached);
			}
			return "";
		}
		filename = cached;
	}
	if ( !uf::io::exists( filename ) ) {
		if ( !uf::asset::assertionLoad ) {
			UF_MSG_ERROR("Failed to preload {}, does not exist", filename);
		} else {
			UF_EXCEPTION("Failed to preload {}, does not exist", filename);
		}
		return "";
	}
	uf::stl::string actual = payload.hash;
	if ( payload.hash != "" && (actual = uf::io::hash( filename )) != payload.hash ) {
		if ( !uf::asset::assertionLoad ) {
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
	return payload.filename = filename;
}
uf::stl::string uf::asset::load( uf::asset::Payload& payload ) {
	uf::stl::string filename = payload.filename;
	uf::stl::string extension = uf::string::lowercase(uf::io::extension( payload.filename, -1 ));
	uf::stl::string basename = uf::string::lowercase( uf::string::replace( uf::io::filename( payload.filename ), "/.(?:gz|lz4?)$/", "" ) );

	if ( payload.filename.substr(0,5) == "https" ) {
		uf::stl::string hash = uf::string::sha256( payload.filename );
		uf::stl::string cached = uf::io::root + "/cache/http/" + hash + "." + extension;
		if ( !uf::io::exists( cached ) && !retrieve( payload.filename, cached, hash ) ) {
			if ( !uf::asset::assertionLoad ) {
				UF_MSG_ERROR("Failed to load {} ({}): HTTP error", payload.filename, cached);
			} else {
				UF_EXCEPTION("Failed to load {} ({}): HTTP error", payload.filename, cached);
			}
			return "";
		}
		filename = cached;
	}
	if ( !uf::io::exists( filename ) ) {
		if ( !uf::asset::assertionLoad ) {
			UF_MSG_ERROR("Failed to load {}: does not exist", filename);
		} else {
			UF_EXCEPTION("Failed to load {}: does not exist", filename);
		}
		return "";
	}
	uf::stl::string actual = payload.hash;
	if ( payload.hash != "" && (actual = uf::io::hash( filename )) != payload.hash ) {
		if ( !uf::asset::assertionLoad ) {
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

	#define UF_ASSET_REGISTER(type)\
		type& asset = payload.asComponent && payload.object.pointer ? payload.object.pointer->getComponent<type>() : uf::asset::get<type>( payload );

	//	if ( uf::asset::has( filename ) ) return filename;
	//	if ( uf::asset::has( payload.filename ) ) return filename;

	switch ( payload.type ) {
		case uf::asset::Type::IMAGE: {
			UF_ASSET_REGISTER(uf::Image)
			asset.open(filename);
		} break;
		case uf::asset::Type::AUDIO: {
			UF_ASSET_REGISTER(uf::Audio)
			asset.open(filename, true);
		} break;
		case uf::asset::Type::JSON: {
			UF_ASSET_REGISTER(uf::Serializer)
			asset.readFromFile(filename);
		} break;
	#if UF_USE_LUA
		case uf::asset::Type::LUA: {
			UF_ASSET_REGISTER(pod::LuaScript)
			asset = ext::lua::script( filename );
		} break;
	#endif
		case uf::asset::Type::GRAPH: {
			UF_ASSET_REGISTER(pod::Graph)

		//	asset = uf::graph::load( filename, payload.metadata );
			uf::graph::load( asset, filename, payload.metadata );
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

	return payload.filename = filename;
}
bool uf::asset::has( const uf::stl::string& url ) {
	return uf::asset::map.count( url ) > 0;
}
bool uf::asset::has( const uf::asset::Payload& payload ) {
	if ( payload.asComponent ) return true;
	return uf::asset::has( payload.filename );
}
void uf::asset::remove( const uf::stl::string& url ) {
	if ( !uf::asset::has( url ) ) return;
	auto& userdata = uf::asset::map[url];
#if UF_COMPONENT_POINTERED_USERDATA
	if ( userdata.data ) uf::pointeredUserdata::destroy( userdata );
#else
	if ( userdata ) uf::userdata::destroy( userdata );
#endif
	uf::asset::map.erase( url );
}
uf::asset::userdata_t& uf::asset::get( const uf::stl::string& url ) {
	return uf::asset::map[url];
}
uf::asset::userdata_t uf::asset::release( const uf::stl::string& url ) {
	auto userdata = uf::asset::get( url );
	uf::asset::map.erase( url );
	return userdata;
}
/*
uf::stl::string uf::asset::getOriginal( const uf::stl::string& uri ) {
	return uri;
	uf::stl::string extension = uf::io::extension( uri );
	auto& map = uf::asset::map; //getComponent<uf::Serializer>();
	if ( ext::json::isNull( map[extension][uri]["index"] ) ) return uri;
	std::size_t index = map[extension][uri]["index"].as<size_t>();

	uf::stl::string key = uri;
	ext::json::forEach( map[extension], [&]( const uf::stl::string& k, ext::json::Value& v ) {
		std::size_t i = v["index"].as<size_t>();
		if ( index == i && key != uri ) key = k;
	});
	return key;
}
*/