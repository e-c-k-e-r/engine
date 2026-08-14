#include <uf/engine/asset/asset.h>
#include <functional>
#include <iomanip>
#include <sys/stat.h>
#include <fstream>

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
#include <uf/engine/scene/scene.h>
#include <uf/ext/zlib/zlib.h>
#include <uf/ext/texconv/texconv.h>

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

	namespace jobs {
		std::mutex mutex;
		uf::asset::Job::container_t queue;
		uf::asset::Job::container_t finished;
	};

	namespace io_read {
		std::mutex mutex;
		uf::asset::Read::container_t queue;
		uf::asset::Read::container_t finished;
	};

	namespace io_stream {
		std::mutex mutex;
		uf::asset::Stream::container_t queue;
		uf::asset::Stream::container_t finished;
	};
}

// uf::asset uf::asset::masterAssetLoader;
bool uf::asset::assertionLoad = true;
bool uf::asset::asyncQueue = true;
bool uf::asset::deferQueues = false;
uf::stl::unordered_map<uf::stl::string, uf::asset::userdata_t> uf::asset::map;
uf::Serializer uf::asset::metadata;

void uf::asset::processQueue() {
	STATIC_THREAD_LOCAL(uf::asset::Job::container_t, jobs);
	uf::asset::Job::container_t finishedJobs;

	::jobs::mutex.lock();
	std::swap( jobs, ::jobs::queue );
	std::swap( finishedJobs, ::jobs::finished );
	::jobs::mutex.unlock();
	
	if ( jobs.empty() && finishedJobs.empty() ) return;

	auto tasks = uf::thread::schedule(uf::asset::asyncQueue ? uf::thread::asyncThreadName : uf::thread::mainThreadName, false);

	if ( !finishedJobs.empty() ) {
		tasks.queue([jobs = std::move(finishedJobs)]() {
			for ( auto& job : jobs ) {
				uf::hooks.call( job.callback, job.payload );
			}
		});
	}
	for ( auto& job : jobs ) tasks.queue([=]{
		auto callback = job.callback;
		auto type = job.type;
		auto payload = job.payload;

		if ( payload.filename == "" || callback == "" ) return;

		uf::stl::string filename = type == "cache" ? uf::asset::cache(payload) : uf::asset::load(payload);
		
		if ( callback != "" && filename != "" ) {
			::jobs::mutex.lock();
			::jobs::finished.emplace_back( job );
			::jobs::mutex.unlock();
		}
	});

	uf::thread::execute( tasks );
}

void uf::asset::processIO() {
	STATIC_THREAD_LOCAL(uf::asset::Read::container_t, pendingReads);
	STATIC_THREAD_LOCAL(uf::asset::Stream::container_t, pendingStreams);

	::io_read::mutex.lock();
	std::swap(pendingReads, ::io_read::queue);
	::io_read::mutex.unlock();
	::io_stream::mutex.lock();
	std::swap(pendingStreams, ::io_stream::queue);
	::io_stream::mutex.unlock();

	if ( pendingReads.empty() && pendingStreams.empty() ) return;

	uf::asset::processIO( pendingReads, pendingStreams, uf::asset::asyncQueue, false );	
}
void uf::asset::processIO( const uf::asset::Read::container_t& pendingReads, bool async, bool wait ) {
	return uf::asset::processIO( pendingReads, {}, async, wait );
}
void uf::asset::processIO( const uf::asset::Stream::container_t& pendingStreams, bool async, bool wait ) {
	return uf::asset::processIO( {}, pendingStreams, async, wait );
}
void uf::asset::processIO( const uf::asset::Read::container_t& pendingReads, const uf::asset::Stream::container_t& pendingStreams, bool async, bool wait ) {
	auto tasks = uf::thread::schedule(async ? uf::thread::asyncThreadName : uf::thread::mainThreadName, wait);
	if ( pendingReads.empty() && pendingStreams.empty() ) return;

	for ( auto& [filename, requests] : pendingReads ) {
		tasks.queue([filename, requests = std::move(requests)]() {
			uf::stl::vector<pod::ScatterRequest> scatters(requests.size());
			uf::stl::vector<uf::stl::vector<uint8_t>> localBuffers(requests.size());
			
			for ( auto i = 0; i < requests.size(); ++i ) {
				auto& req = requests[i];
				scatters[i] = { req.offset, req.length, req.dest };
				
				if ( !req.dest && req.length > 0 ) {
					localBuffers[i].resize( req.length );
					scatters[i].dest = localBuffers[i].data();
				}
			}

			uf::io::readScatter( filename, scatters );

			for ( auto i = 0; i < requests.size(); ++i ) {
				auto& req = requests[i];
				if ( req.callback ) req.callback();
				else if ( req.callbackBuffered ) req.callbackBuffered( std::move( localBuffers[i] ) );
			}
		});
	}

	for (auto& [filename, requests] : pendingStreams) {
		tasks.queue([filename = filename, requests = std::move(requests)]() mutable {
			size_t maxEndOffset = 0;
			for ( auto& req : requests ) {
				maxEndOffset = std::max(maxEndOffset, req.offset + req.length);
			}

			size_t readChunkSize = ext::zlib::bufferSize;
			for ( auto& req : requests ) {
				if ( req.chunkSize > 0 ) readChunkSize = std::min(readChunkSize, req.chunkSize);
			}

			size_t currentOffset = 0;
			uf::vfs::stream( filename, readChunkSize, [&](const uint8_t* data, size_t size) -> bool {
				size_t chunkStart = currentOffset;
				size_t chunkEnd = currentOffset + size;
				currentOffset += size;

				bool anyActive = false;
				for ( auto& req : requests ) {
					if ( !req.callback ) continue;
					anyActive = true;

					if ( chunkEnd > req.offset && chunkStart < req.offset + req.length ) {
						size_t copyStart = (chunkStart < req.offset) ? (req.offset - chunkStart) : 0;
						size_t copyLen = std::min(size - copyStart, (req.offset + req.length) - (chunkStart + copyStart));

						size_t absoluteFileOffset = chunkStart + copyStart;
						if ( !req.callback(data + copyStart, copyLen, absoluteFileOffset) ) req.callback = nullptr;
					}
					if ( chunkEnd >= req.offset + req.length ) req.callback = nullptr;
				}
				if ( chunkEnd >= maxEndOffset || !anyActive ) return false;

				return true;
			});
		});
	}

	uf::thread::execute(tasks);
}

void uf::asset::cache( const uf::asset::callback_t& callback, const uf::asset::Payload& payload ) {
	std::lock_guard<std::mutex> lock(::jobs::mutex);
	::jobs::queue.emplace_back(uf::asset::Job{ callback, "cache", payload });
}
void uf::asset::load( const uf::asset::callback_t& callback, const uf::asset::Payload& payload ) {
	std::lock_guard<std::mutex> lock(::jobs::mutex);
	::jobs::queue.emplace_back(uf::asset::Job{ callback, "load", payload });
}

void uf::asset::read( const uf::stl::string& filename, size_t offset, size_t length, uint8_t* dest, std::function<void()> callback ) {
	std::lock_guard<std::mutex> lock(::io_read::mutex);
	uf::asset::read( ::io_read::queue, filename, offset, length, dest, callback );
}
void uf::asset::read( const uf::stl::string& filename, size_t offset, size_t length, std::function<void(uf::stl::vector<uint8_t>&&)> callback ) {
	std::lock_guard<std::mutex> lock(::io_read::mutex);
	uf::asset::read( ::io_read::queue, filename, offset, length, callback );
}

void uf::asset::stream( const uf::stl::string& filename, size_t offset, size_t length, size_t chunkSize, std::function<bool(const uint8_t* data, size_t size, size_t fileOffset)> callback ) {
	std::lock_guard<std::mutex> lock(::io_stream::mutex);
	uf::asset::stream( ::io_stream::queue, filename, offset, length, chunkSize, callback );
}

void uf::asset::read( uf::asset::Read::container_t& container, const uf::stl::string& filename, size_t offset, size_t length, uint8_t* dest, std::function<void()> callback ) {
	container[filename].emplace_back(uf::asset::Read{ offset, length, dest, callback });
}
void uf::asset::read( uf::asset::Read::container_t& container, const uf::stl::string& filename, size_t offset, size_t length, std::function<void(uf::stl::vector<uint8_t>&&)> callback ) {
	container[filename].emplace_back(uf::asset::Read{ offset, length, nullptr, nullptr, callback });
}

void uf::asset::stream( uf::asset::Stream::container_t& container, const uf::stl::string& filename, size_t offset, size_t length, size_t chunkSize, std::function<bool(const uint8_t* data, size_t size, size_t fileOffset)> callback ) {
	container[filename].emplace_back(uf::asset::Stream{ offset, length, chunkSize, callback });
}

uf::asset::Payload uf::asset::resolveToPayload( const uf::stl::string& uri, const uf::stl::string& mime ) {
	uf::stl::string extension = uf::string::lowercase( uf::io::extension( uri, -1 ) );
	uf::stl::string basename = uf::string::lowercase( uf::string::replace( uf::io::filename( uri ), "/.(?:gz|lz4?)$/", "" ) );
	uf::asset::Payload payload;

	static uf::stl::unordered_map<uf::stl::string,uf::asset::Type> typemap = {
		{ "jpg", 	uf::asset::Type::IMAGE },
		{ "jpeg", 	uf::asset::Type::IMAGE },
		{ "png", 	uf::asset::Type::IMAGE },
		{ "dtex", 	uf::asset::Type::IMAGE },
		
		{ "ogg", 	uf::asset::Type::AUDIO },
		{ "wav", 	uf::asset::Type::AUDIO },
		{ "adp", 	uf::asset::Type::AUDIO },

		{ "json", 	uf::asset::Type::JSON },
		{ "bson", 	uf::asset::Type::JSON },
		{ "cbor", 	uf::asset::Type::JSON },
		{ "msgpack",uf::asset::Type::JSON },
		{ "ubjson", uf::asset::Type::JSON },
		{ "bjdata", uf::asset::Type::JSON },
		{ "toml", uf::asset::Type::JSON },

		{ "lua", 	uf::asset::Type::LUA },
		
	#if UF_USE_GLTF
		{ "glb",  	uf::asset::Type::GRAPH },
		{ "gltf", 	uf::asset::Type::GRAPH },
		{ "mdl",  	uf::asset::Type::GRAPH },
	#endif
		{ "bsp",  	uf::asset::Type::GRAPH },
		{ "mis",  	uf::asset::Type::GRAPH },
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
//	DC_STATS();
#endif
	return payload.filename = filename;
}
uf::stl::string uf::asset::load( uf::asset::Payload& payload ) {
	uf::stl::string filename = payload.filename;
	uf::stl::string extension = uf::string::lowercase(uf::io::extension( payload.filename, -1 ));
	uf::stl::string basename = uf::string::lowercase( uf::string::replace( uf::io::filename( payload.filename ), "/.(?:gz|lz4?)$/", "" ) );

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
//	DC_STATS();
#endif

	#define UF_ASSET_REGISTER(type)\
		type& asset = payload.asComponent && payload.object.pointer ? payload.object.pointer->getComponent<type>() : uf::asset::get<type>( payload );

	//	if ( uf::asset::has( filename ) ) return filename;
	//	if ( uf::asset::has( payload.filename ) ) return filename;

	switch ( payload.type ) {
		case uf::asset::Type::IMAGE: {
			UF_ASSET_REGISTER(uf::Image)
			asset.open(filename);
	
		#if UF_USE_DC_TEXCONV
			uf::stl::string target = "dtex";
		#if !UF_CONVERSION_TO_CACHE
			uf::stl::string output = uf::io::replace_extension( filename, target );
			if ( !uf::vfs::isPhysical( filename ) ) output = "cache://" + output;
		#else
			uf::stl::string output = "cache://" + uf::io::replace_extension( filename, target );
		#endif
			bool stale = uf::io::exists( output ) ? uf::io::mtime( filename ) > uf::io::mtime( output ) : true;
			if ( extension != target && stale ) {
				UF_MSG_DEBUG("Converting image: {} => {}", filename, output );
				pod::Vector2ui size = { 64, 64 };
				uf::stl::string filter = "linear";
				uf::stl::string format = "auto";
				size = uf::vector::clamp( size, pod::Vector2ui{ 4, 4 }, asset.getDimensions() );

				auto img = asset.scale( size, filter );
				auto dtex = ext::texconv::convert( img, format );
				ext::texconv::save( dtex, output );
			}
		#endif
		} break;
		case uf::asset::Type::AUDIO: {
			UF_ASSET_REGISTER(pod::AudioClip)
			asset.streamed = payload.metadata["streamed"].as<bool>( asset.streamed );
			uf::audio::load(asset, filename);

		#if UF_USE_ADP_ENCODER
			uf::stl::string target = "adp";
		#if !UF_CONVERSION_TO_CACHE
			uf::stl::string output = uf::io::replace_extension( filename, target );
			if ( !uf::vfs::isPhysical( filename ) ) output = "cache://" + output;
		#else
			uf::stl::string output = "cache://" + uf::io::replace_extension( filename, target );
		#endif
			bool stale = uf::io::exists( output ) ? uf::io::mtime( filename ) > uf::io::mtime( output ) : true;
			if ( extension != target && stale ) {
				pod::PCM pcm;
				uf::stl::vector<uint8_t> encoded;
				if ( uf::audio::decode( filename, pcm ) ) {
					encoded = uf::audio::encode( target, pcm );
				}				
				if ( !encoded.empty() ) {
					UF_MSG_DEBUG("Converting audio: {} => {}", filename, output);
					uf::io::write( output, encoded.data(), encoded.size() );
				}
			}
		#endif
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

			// pre-bind root to object
		#if 0
			// instance buffer fails to attach for some reason
			// to-do: fix it?
			if ( payload.asComponent && payload.object.pointer ) {
				asset.root.entity = payload.object.pointer;
				if ( uf::graph::storageMode == pod::Graph::Storage::OBJECT ) {
					asset.root.entity->addComponent<pod::Graph::Storage>();
				}
			}
		#endif

			uf::graph::load( asset, filename, payload.metadata );
			uf::graph::process( asset );

		#if !UF_ENV_DREAMCAST
			if ( asset.metadata["debug"]["print"]["stats"].as<bool>() ) UF_MSG_INFO("{}", uf::graph::stats( asset ));
			if ( asset.metadata["debug"]["print"]["tree"].as<bool>() ) UF_MSG_INFO("{}", uf::graph::print( asset ));
		#endif
			//if ( !asset.metadata["debug"]["no cleanup"].as<bool>() ) uf::graph::cleanup( asset );
		} break;
		default: {
			UF_MSG_ERROR("Failed to parse {}: unimplemented extension: {}", filename, extension );
		}
	}

#if UF_ENV_DREAMCAST
	UF_MSG_DEBUG("Loaded {}", filename);
//	DC_STATS();
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
	auto userdata = std::move(uf::asset::map[url]);
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
	auto userdata = std::move(uf::asset::get( url ));
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