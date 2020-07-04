#include "asset.h"
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
#include <uf/utils/thread/thread.h>

#include <mutex>

namespace {
	inline bool exists( const std::string& name ) {
		struct stat buffer;   
		return (stat (name.c_str(), &buffer) == 0); 
	}
	bool retrieve( const std::string& url, const std::string& filename ) {
		uf::Http http = uf::http::get( url );
		if ( http.code < 200 || http.code > 300 ) {
			uf::iostream << "HTTP Error " << http.code << " on GET " << url << "\n";
			return false;
		}
		std::ofstream output;
		output.open("./" + filename, std::ios::binary);
		output.write( http.response.c_str(), http.response.size() );
		output.close();
		return true;
	}
	std::string hashed( const std::string& uri ) {
		std::size_t value = std::hash<std::string>{}( uri );
		std::stringstream stream;
		stream << std::hex << value;
		return stream.str();
	}

	std::mutex mutex;
	uint64_t uid = 0;
	struct Job {
		std::string uri;
		std::string callback;
		std::string type;
	};
	std::queue<Job> jobs;
/*
	uint64_t queueJob( const std::string& uri, const std::string& callback ) {
		mutex.lock();
		uint64_t id = ++uid;
		jobs.push({
			id, uri, callback
		});
		mutex.unlock();
		return id;
	}
	std::unordered_map<uint64_t, Job> jobs;
	Job getJob( uint64_t id ){
		mutex.lock();
		Job job = jobs[id];
		jobs.erase(id);
		mutex.unlock();
		return job;
	}
	uint64_t queueJob( const std::string& uri, const std::string& callback ) {
		mutex.lock();
		uint64_t id = ++uid;
		jobs[id] = {
			id, uri, callback
		};
		mutex.unlock();
		return id;
	}
*/
}

uf::Entity ext::Asset::masterAssetLoader;

void ext::Asset::processQueue() {
	mutex.lock();
	while ( !jobs.empty() ) {
		auto job = jobs.front();
		jobs.pop();
		std::string uri = job.uri;
		std::string type = job.type;
		std::string callback = job.callback;
		if ( uri == "" || callback == "" ) {
			continue;
		}
		std::string filename = type == "cache" ? this->cache(uri) : this->load(uri);
		if ( callback != "" ) {
			uf::Serializer payload;
			payload["filename"] = filename;
			uf::hooks.call(callback, payload);
		}
	}
	mutex.unlock();
}
void ext::Asset::cache( const std::string& uri, const std::string& callback ) {
	mutex.lock();
	jobs.push({ uri, callback, "cache" });
	mutex.unlock();
/*
	uint64_t id = queueJob(uri, callback);
	std::cout << "Queued job: " + std::to_string(id) + ": " + callback + " " + uri << std::endl;
	uf::thread::add( uf::thread::get("Aux"), [&]() -> int {
		auto job = getJob(id);
		std::string uri = job.uri;
		std::string callback = job.callback;
		if ( uri == "" || callback == "" || id == 0 ) {
			std::cout << "Failed job: " << id << std::endl;
			return 0;
		}
		std::string filename = this->cache(uri);
		if ( callback != "" ) {
			uf::Serializer payload;
			payload["filename"] = filename;
			uf::hooks.call(callback, payload);
		}
	return 0;}, true );
*/
}
void ext::Asset::load( const std::string& uri, const std::string& callback ) {
	mutex.lock();
	jobs.push({ uri, callback, "load" });
	mutex.unlock();
/*
	uf::thread::add( uf::thread::get("Aux"), [&]() -> int {
		auto job = getJob(id);
		std::string uri = job.uri;
		std::string callback = job.callback;
		if ( uri == "" || callback == "" || id == 0 ) {
			std::cout << "Failed job: " << id << std::endl;
			return 0;
		}
		std::string filename = this->load(uri);
		if ( callback != "" ) {
			uf::Serializer payload;
			payload["filename"] = filename;
			uf::hooks.call(callback, payload);
		}
	return 0;}, true );
*/
}
std::string ext::Asset::cache( const std::string& uri ) {
	std::string filename = uri;
	std::string extension = uf::string::extension( uri );
	if ( uri.substr(0,5) == "https" ) {
		std::string hash = hashed( uri );
		std::string cached = "./data/cache/" + hash + "." + extension;
		if ( !exists( cached ) && !retrieve( uri, cached ) ) {
			uf::iostream << "Failed to preload `" + uri + "` (`" + cached + "`): HTTP error" << "\n"; 
			return "";
		}
		filename = cached;
	}
	if ( !exists( filename ) ) {
		uf::iostream << "Failed to preload `" + filename + "`: Does not exist" << "\n"; 
		return "";
	}
	return filename;
}
std::string ext::Asset::load( const std::string& uri ) {
	std::string filename = uri;
	std::string extension = uf::string::extension( uri );
	if ( uri.substr(0,5) == "https" ) {
		std::string hash = hashed( uri );
		std::string cached = "./data/cache/" + hash + "." + extension;
		if ( !exists( cached ) && !retrieve( uri, cached ) ) {
			uf::iostream << "Failed to load `" + uri + "` (`" + cached + "`): HTTP error" << "\n"; 
			return "";
		}
		filename = cached;
	}
	if ( !exists( filename ) ) {
		uf::iostream << "Failed to load `" + filename + "`: Does not exist" << "\n"; 
		return "";
	}
	// deduce PNG, load as texture
	auto& map = masterAssetLoader.getComponent<uf::Serializer>();
	if ( extension == "png" ) {
		auto& container = this->getContainer<uf::Image>();

		if ( !map[extension][uri].isNull() ) return filename;
		if ( !map[extension][filename].isNull() ) return filename;

		map[extension][uri] = container.size();
		map[extension][filename] = container.size();

		uf::Image& image = container.emplace_back();
		image.open(filename);
	} else if ( extension == "ogg" ) {
		auto& container = this->getContainer<uf::Audio>();

		if ( !map[extension][uri].isNull() ) return filename;
		if ( !map[extension][filename].isNull() ) return filename;

		map[extension][uri] = container.size();
		map[extension][filename] = container.size();

		uf::Audio& audio = container.emplace_back();
		audio.load(filename);
	} else if ( extension == "json" ) {
		auto& container = this->getContainer<uf::Serializer>();

		if ( !map[extension][uri].isNull() ) return filename;
		if ( !map[extension][filename].isNull() ) return filename;

		map[extension][uri] = container.size();
		map[extension][filename] = container.size();

		uf::Serializer& json = container.emplace_back();
		json.readFromFile(filename);
	} else {
		uf::iostream << "Failed to load `" + filename + "`: Unimplemented extension: " + extension << "\n"; 
		return "";
	}
	return filename;
}
std::string ext::Asset::getOriginal( const std::string& uri ) {
	std::string extension = uf::string::extension( uri );
	auto& map = masterAssetLoader.getComponent<uf::Serializer>();
	if ( map[extension][uri].isNull() ) return uri;
	std::size_t index = map[extension][uri].asUInt64();
	for ( auto it = map[extension].begin(); it != map[extension].end(); ++it ) {
		std::string key = it.key().asString();
		std::size_t i = it->asUInt64();
		if ( index == i && key != uri ) return key;
	}
	return uri;
}