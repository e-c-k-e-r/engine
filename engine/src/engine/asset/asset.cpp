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

#include <mutex>

namespace {
	bool retrieve( const uf::stl::string& url, const uf::stl::string& filename, const uf::stl::string& hash = "" ) {
		uf::Http http = uf::http::get( url );
		if ( http.code < 200 || http.code > 300 ) {
			UF_MSG_ERROR("HTTP Error " << http.code << " on GET " << url);
			return false;
		}

		uf::stl::string actual = hash;
		if ( hash != "" && (actual = uf::string::sha256(url)) != hash ) {
			UF_MSG_ERROR("HTTP hash mismatch on GET " << url << ": expected " << hash << ", got " <<  actual);
			return false;
		}

		uf::io::write( "./" + filename, http.response );
		return true;
	}
	uf::stl::string hashed( const uf::stl::string& uri ) {
		return uf::string::sha256(uri);
	/*
		std::size_t value = std::hash<uf::stl::string>{}( uri );
		uf::stl::stringstream stream;
		stream << std::hex << value;
		return stream.str();
	*/
	}

	std::mutex mutex;
	uint64_t uid = 0;
	struct Job {
		uf::stl::string callback;
		uf::stl::string type;
		uf::stl::string uri;
		uf::stl::string hash;
		uf::stl::string category;
	};
}

uf::Asset uf::Asset::masterAssetLoader;

void uf::Asset::processQueue() {
	auto& jobs = this->getComponent<std::queue<Job>>();
	mutex.lock();
	while ( !jobs.empty() ) {
		auto job = jobs.front();
		jobs.pop();
		uf::stl::string callback = job.callback;
		uf::stl::string type = job.type;
		uf::stl::string uri = job.uri;
		uf::stl::string hash = job.hash;
		uf::stl::string category = job.category;
		if ( uri == "" || callback == "" ) {
			continue;
		}
		uf::stl::string filename = type == "cache" ? this->cache(uri, hash, category) : this->load(uri, hash, category);
		if ( callback != "" && filename != "" ) {
			uf::Serializer payload;
			payload["filename"] = filename;
			payload["hash"] = hash;
			payload["category"] = category;
			uf::hooks.call(callback, payload);
		}
	}
	mutex.unlock();
}
void uf::Asset::cache( const uf::stl::string& callback, const uf::stl::string& uri, const uf::stl::string& hash, const uf::stl::string& category ) {
	mutex.lock();
	auto& jobs = this->getComponent<std::queue<Job>>();
	jobs.push({ callback, "cache", uri, hash, category });
	mutex.unlock();
}
void uf::Asset::load( const uf::stl::string& callback, const uf::stl::string& uri, const uf::stl::string& hash, const uf::stl::string& category ) {
	mutex.lock();
	auto& jobs = this->getComponent<std::queue<Job>>();
	jobs.push({ callback, "load", uri, hash, category });
	mutex.unlock();
}
uf::stl::string uf::Asset::cache( const uf::stl::string& uri, const uf::stl::string& hash, const uf::stl::string& category ) {
	uf::stl::string filename = uri;
	uf::stl::string extension = uf::io::extension( uri );
	if ( uri.substr(0,5) == "https" ) {
		uf::stl::string hash = hashed( uri );
		uf::stl::string cached = uf::io::root + "/cache/http/" + hash + "." + extension;
		if ( !uf::io::exists( cached ) && !retrieve( uri, cached, hash ) ) {
			UF_MSG_ERROR("Failed to preload `" + uri + "` (`" + cached + "`): HTTP error");
			return "";
		}
		filename = cached;
	}
	if ( !uf::io::exists( filename ) ) {
		UF_MSG_ERROR("Failed to preload `" + filename + "`: Does not exist");
		return "";
	}
	uf::stl::string actual = hash;
	if ( hash != "" && (actual = uf::io::hash( filename )) != hash ) {
		UF_MSG_ERROR("Failed to preload `" << filename << "`: Hash mismatch; expected " << hash <<  ", got " << actual);
		return "";
	}
	return filename;
}
uf::stl::string uf::Asset::load( const uf::stl::string& uri, const uf::stl::string& hash, const uf::stl::string& category ) {
	uf::stl::string filename = uri;
	uf::stl::string extension = uf::io::extension( uri );
	if ( uri.substr(0,5) == "https" ) {
		uf::stl::string hash = hashed( uri );
		uf::stl::string cached = uf::io::root + "/cache/http/" + hash + "." + extension;
		if ( !uf::io::exists( cached ) && !retrieve( uri, cached, hash ) ) {
			UF_MSG_ERROR("Failed to load `" + uri + "` (`" + cached + "`): HTTP error");
			return "";
		}
		filename = cached;
	}
	if ( !uf::io::exists( filename ) ) {
		UF_MSG_ERROR("Failed to load `" + filename + "`: Does not exist");
		return "";
	}
	uf::stl::string actual = hash;
	if ( hash != "" && (actual = uf::io::hash( filename )) != hash ) {
		UF_MSG_ERROR("Failed to load `" << filename << "`: Hash mismatch; expected " << hash <<  ", got " << actual);
		return "";
	}

	#define UF_ASSET_REGISTER(type)\
		auto& container = this->getContainer<type>();\
		if ( !ext::json::isNull( map[extension][uri]["index"] ) ) return filename;\
		if ( !ext::json::isNull( map[extension][filename]["index"] ) ) return filename;\
		map[extension][uri]["index"] = container.size();\
		map[extension][filename]["index"] = container.size();\
		type& asset = container.emplace_back();


	auto& map = this->getComponent<uf::Serializer>();
	// deduce PNG, load as texture
	if ( category == "images" || (category == "" && extension == "png") ) {
		UF_ASSET_REGISTER(uf::Image)
		asset.open(filename);
	} else if ( category == "audio" || (category == "" && extension == "ogg") ) {
		UF_ASSET_REGISTER(uf::Audio)
		asset.open(filename);
	} else if ( category == "audio-stream" || (category == "" && extension == "ogg") ) {
		UF_ASSET_REGISTER(uf::Audio)
		asset.stream(filename);
	} else if ( category == "entities" || (category == "" && extension == "json") ) {
		UF_ASSET_REGISTER(uf::Serializer)
		asset.readFromFile(filename);
	#if UF_USE_LUA
	} else if ( category == "scripts" || (category == "" && extension == "lua") ) {
		UF_ASSET_REGISTER(pod::LuaScript)
		asset = ext::lua::script( filename );
	#endif
	} else if ( category == "models" || (category == "" && (extension == "gltf" || extension == "glb" || extension == "graph" ) ) ) {
		UF_ASSET_REGISTER(pod::Graph)
		auto& metadata = this->getComponent<uf::Serializer>();

	#if UF_USE_OPENGL_FIXED_FUNCTION
		metadata[uri]["flags"]["ATLAS"] = false;
		metadata[uri]["flags"]["SEPARATE"] = true;
	#elif UF_GRAPH_INDIRECT_DRAW
		metadata[uri]["flags"]["ATLAS"] = false;
		metadata[uri]["flags"]["SEPARATE"] = false;
	#endif
		asset = uf::graph::load( filename, metadata[uri] );
		uf::graph::process( asset );
		if ( asset.metadata["debug"]["print stats"].as<bool>() ) UF_MSG_INFO(uf::graph::stats( asset ).dump(1,'\t'));
		if ( asset.metadata["debug"]["print tree"].as<bool>() ) UF_MSG_INFO(uf::graph::print( asset ));
		if ( !asset.metadata["debug"]["no cleanup"].as<bool>() ) uf::graph::cleanup( asset );
	} else {
		UF_MSG_ERROR("Failed to parse `" + filename + "`: Unimplemented extension: " + extension + " or category: " + category );
	}
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