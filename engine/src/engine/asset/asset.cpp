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
	bool retrieve( const std::string& url, const std::string& filename, const std::string& hash = "" ) {
		uf::Http http = uf::http::get( url );
		if ( http.code < 200 || http.code > 300 ) {
			uf::iostream << "HTTP Error " << http.code << " on GET " << url << "\n";
			return false;
		}
		std::ofstream output;

		std::string actual = hash;
		if ( hash != "" && (actual = uf::string::sha256(url)) != hash ) {
			uf::iostream << "HTTP hash mismatch on GET " << url << ": expected " << hash << ", got " <<  actual << "\n";
			return false;
		}

		output.open("./" + filename, std::ios::binary);
		output.write( http.response.c_str(), http.response.size() );
		output.close();
		return true;
	}
	std::string hashed( const std::string& uri ) {
		return uf::string::sha256(uri);
	/*
		std::size_t value = std::hash<std::string>{}( uri );
		std::stringstream stream;
		stream << std::hex << value;
		return stream.str();
	*/
	}

	std::mutex mutex;
	uint64_t uid = 0;
	struct Job {
		std::string uri;
		std::string hash;
		std::string callback;
		std::string type;
	};
}

uf::Asset uf::Asset::masterAssetLoader;

void uf::Asset::processQueue() {
	auto& jobs = this->getComponent<std::queue<Job>>();
	mutex.lock();
	while ( !jobs.empty() ) {
		auto job = jobs.front();
		jobs.pop();
		std::string uri = job.uri;
		std::string hash = job.hash;
		std::string callback = job.callback;
		std::string type = job.type;
		if ( uri == "" || callback == "" ) {
			continue;
		}
		std::string filename = type == "cache" ? this->cache(uri, hash) : this->load(uri, hash);
		if ( callback != "" && filename != "" ) {
			uf::Serializer payload;
			payload["filename"] = filename;
			uf::hooks.call(callback, payload);
		}
	}
	mutex.unlock();
}
void uf::Asset::cache( const std::string& uri, const std::string& callback, const std::string& hash ) {
	mutex.lock();
	auto& jobs = this->getComponent<std::queue<Job>>();
	jobs.push({ uri, hash, callback, "cache" });
	mutex.unlock();
}
void uf::Asset::load( const std::string& uri, const std::string& callback, const std::string& hash ) {
	mutex.lock();
	auto& jobs = this->getComponent<std::queue<Job>>();
	jobs.push({ uri, hash, callback, "load" });
	mutex.unlock();
}
std::string uf::Asset::cache( const std::string& uri, const std::string& hash ) {
	std::string filename = uri;
	std::string extension = uf::io::extension( uri );
	if ( uri.substr(0,5) == "https" ) {
		std::string hash = hashed( uri );
		std::string cached = uf::io::root + "/cache/" + hash + "." + extension;
		if ( !uf::io::exists( cached ) && !retrieve( uri, cached, hash ) ) {
			uf::iostream << "Failed to preload `" + uri + "` (`" + cached + "`): HTTP error" << "\n"; 
			return "";
		}
		filename = cached;
	}
	if ( !uf::io::exists( filename ) ) {
		uf::iostream << "Failed to preload `" + filename + "`: Does not exist" << "\n"; 
		return "";
	}
	std::string actual = hash;
	if ( hash != "" && (actual = uf::io::hash( filename )) != hash ) {
		uf::iostream << "Failed to preload `" << filename << "`: Hash mismatch; expected " << hash <<  ", got " << actual << "\n";
		return "";
	}
	return filename;
}
std::string uf::Asset::load( const std::string& uri, const std::string& hash ) {
	std::string filename = uri;
	std::string extension = uf::io::extension( uri );
	if ( uri.substr(0,5) == "https" ) {
		std::string hash = hashed( uri );
		std::string cached = uf::io::root + "/cache/" + hash + "." + extension;
		if ( !uf::io::exists( cached ) && !retrieve( uri, cached, hash ) ) {
			uf::iostream << "Failed to load `" + uri + "` (`" + cached + "`): HTTP error" << "\n"; 
			return "";
		}
		filename = cached;
	}
	if ( !uf::io::exists( filename ) ) {
		uf::iostream << "Failed to load `" + filename + "`: Does not exist" << "\n"; 
		return "";
	}
	std::string actual = hash;
	if ( hash != "" && (actual = uf::io::hash( filename )) != hash ) {
		uf::iostream << "Failed to load `" << filename << "`: Hash mismatch; expected " << hash <<  ", got " << actual << "\n";
		return "";
	}
	#define UF_ASSET_REGISTER(type)\
		auto& container = this->getContainer<type>();\
		if ( !ext::json::isNull( map[extension][uri] ) ) return filename;\
		if ( !ext::json::isNull( map[extension][filename] ) ) return filename;\
		map[extension][uri] = container.size();\
		map[extension][filename] = container.size();\
		type& asset = container.emplace_back();


	auto& map = this->getComponent<uf::Serializer>();
	// deduce PNG, load as texture
	if ( extension == "png" ) {
		UF_ASSET_REGISTER(uf::Image)
		asset.open(filename);
	} else if ( extension == "ogg" ) {
		UF_ASSET_REGISTER(uf::Audio)
		asset.load(filename);
	} else if ( extension == "json" ) {
		UF_ASSET_REGISTER(uf::Serializer)
		asset.readFromFile(filename);
	#if UF_USE_LUA
	} else if ( extension == "lua" ) {
		UF_ASSET_REGISTER(pod::LuaScript)
		asset = ext::lua::script( filename );
	#endif
	} else if ( extension == "gltf" || extension == "glb" ) {
		UF_ASSET_REGISTER(pod::Graph)
		auto& metadata = this->getComponent<uf::Serializer>();

		metadata[uri]["flags"]["ATLAS"] = true;
		ext::gltf::load_mode_t LOAD_FLAGS = 0;
		#define LOAD_FLAG(name)\
			if ( metadata[uri]["flags"][#name].as<bool>() )\
				LOAD_FLAGS |= ext::gltf::LoadMode::name;

		LOAD_FLAG(RENDER) 				// = 0x1 << 0,
		LOAD_FLAG(COLLISION) 			// = 0x1 << 1,
		LOAD_FLAG(SEPARATE) 			// = 0x1 << 2,
		LOAD_FLAG(NORMALS) 				// = 0x1 << 3,
		LOAD_FLAG(LOAD) 				// = 0x1 << 4,
		LOAD_FLAG(ATLAS) 				// = 0x1 << 5,
		LOAD_FLAG(SKINNED) 				// = 0x1 << 6,
		LOAD_FLAG(INVERT) 				// = 0x1 << 7,
		LOAD_FLAG(TRANSFORM) 			// = 0x1 << 8,

		asset = ext::gltf::load( filename, LOAD_FLAGS, metadata[uri] );
	} else {
		uf::iostream << "Failed to parse `" + filename + "`: Unimplemented extension: " + extension << "\n"; 
	}
	// uf::iostream << "Parsed URI: `" + uri + "` -> `" + filename + "`" << "\n"; 
	return filename;
}
std::string uf::Asset::getOriginal( const std::string& uri ) {
	std::string extension = uf::io::extension( uri );
	auto& map = this->getComponent<uf::Serializer>();
	if ( ext::json::isNull( map[extension][uri] ) ) return uri;
	std::size_t index = map[extension][uri].as<size_t>();

	std::string key = uri;
	ext::json::forEach( map[extension], [&]( const std::string& k, ext::json::Value& v ) {
		std::size_t i = v.as<size_t>();
		if ( index == i && key != uri ) key = k;
	});
	return key;
}