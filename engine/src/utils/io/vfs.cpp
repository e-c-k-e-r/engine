#include <uf/utils/io/vfs.h>
#include <uf/utils/io/file.h>
#include <uf/utils/math/hash.h>
#include <algorithm>
#include <sys/stat.h>

#if UF_ENV_DREAMCAST
	#include <kos/fs.h>
	#include <kos/limits.h>
#else
	#include <filesystem>
#endif

namespace {
#if UF_ENV_DREAMCAST
	void ls_kos( const uf::stl::string& basePath, const uf::stl::string& currentSubDir, const uf::stl::string& extension, bool recursive, uf::stl::vector<uf::stl::string>& files ) {
		uf::stl::string fullPath = basePath + currentSubDir;

		file_t fd = fs_open(fullPath.c_str(), O_DIR | O_RDONLY);
		if (fd == FILEHND_INVALID) return;

		const dirent_t* entry;
		while ( (entry = fs_readdir(fd)) != nullptr ) {
			if (entry->name[0] == '.' && (entry->name[1] == '\0' || (entry->name[1] == '.' && entry->name[2] == '\0'))) {
				continue;
			}

			uf::stl::string fname = entry->name;
			uf::stl::string relPath = currentSubDir.empty() ? fname : currentSubDir + "/" + fname;
			bool isDir = (entry->size < 0) || (entry->attr & O_DIR);
			if ( isDir ) {
				if ( recursive ) {
					::ls_kos(basePath, relPath, extension, recursive, files);
				}
			} else {
				if (extension.empty() || fname.ends_with(extension)) {
					files.emplace_back(relPath);
				}
			}
		}
		fs_close(fd);
	}

#endif
	bool vfs_exists( pod::Mount& mount, const uf::stl::string& file ) {
		uf::stl::string path = mount.path + file;
		#if UF_ENV_DREAMCAST
			FILE* f = fopen(path.c_str(), "r");
			if ( f ) {
				fclose(f);
				return true;
			}
			return false;
		#else
			static struct stat buffer;
			return stat(path.c_str(), &buffer) == 0;
		#endif
	}
	size_t vfs_size( pod::Mount& mount, const uf::stl::string& file ) {
		uf::stl::string path = mount.path + file;
		std::ifstream is(path, std::ios::binary | std::ios::in | std::ios::ate);
		if ( !is.is_open() ) return 0;
		is.seekg(0, std::ios::end);
		return is.tellg();
	}
	size_t vfs_mtime( pod::Mount& mount, const uf::stl::string& file ) {
		uf::stl::string path = mount.path + file;
		static struct stat buffer;
		if ( stat(path.c_str(), &buffer) != 0 ) return 0;
		return buffer.st_mtime;
	}
	bool vfs_read( pod::Mount& mount, const uf::stl::string& file, uf::stl::vector<uint8_t>& buffer ) {
		uf::stl::string path = mount.path + file;
		std::ifstream is(path, std::ios::binary | std::ios::ate);
		if (!is.is_open()) return false;

		size_t len = is.tellg();
		is.seekg(0, std::ios::beg);

		buffer.resize(len);
		is.read((char*)buffer.data(), len);
		return true;
	}

	size_t vfs_write( pod::Mount& mount, const uf::stl::string& file, const void* buffer, size_t size ) {
		uf::stl::string path = mount.path + file;
		std::ofstream output(path, std::ios::binary);
		if (!output.is_open()) return 0;
		output.write((const char*)buffer, size);
		output.close();
		return size;
	}

	uf::stl::vector<uf::stl::string> vfs_list( pod::Mount& mount, const uf::stl::string& dir, const uf::stl::string& extension = "", bool recursive = false ) {
		uf::stl::vector<uf::stl::string> files;
		
		uf::stl::string path = mount.path + dir;
	#if UF_ENV_DREAMCAST
		if ( !path.ends_with("/") ) path += "/";
		::ls_kos( path, "", extension, recursive, files );
	#else
		if ( !std::filesystem::exists(path) ) return files;

		if ( recursive ) {
			for ( const auto& entry : std::filesystem::recursive_directory_iterator(path) ) {
				if ( entry.is_regular_file() ) {
					uf::stl::string fname = entry.path().filename().string();
					if ( extension.empty() || fname.ends_with(extension) ) {
						uf::stl::string relPath = entry.path().string().substr(mount.path.length());
						std::replace(relPath.begin(), relPath.end(), '\\', '/');
						files.emplace_back(relPath);
					}
				}
			}
		} else {
			for ( const auto& entry : std::filesystem::directory_iterator(path) ) {
				if ( entry.is_regular_file() ) {
					uf::stl::string fname = entry.path().filename().string();
					if ( extension.empty() || fname.ends_with(extension) ) {
						uf::stl::string relPath = entry.path().string().substr(mount.path.length());
						std::replace(relPath.begin(), relPath.end(), '\\', '/');
						files.emplace_back(relPath);
					}
				}
			}
		}
	#endif
		return files;
	}

	bool vfs_mkdir( pod::Mount& mount, const uf::stl::string& file ) {
		uf::stl::string path = mount.path + file;
		#if UF_ENV_DREAMCAST || UF_ENV_LINUX
			return false;
		#else
			int status = ::mkdir(path.c_str());
			return status != -1;
		#endif
	}

	bool vfs_readRange( pod::Mount& mount, const uf::stl::string& file, size_t start, size_t len, uf::stl::vector<uint8_t>& buffer ) {
		uf::stl::string path = mount.path + file;
		std::ifstream is(path, std::ios::binary);
		if (!is.is_open()) return false;

		is.seekg(start, std::ios::beg);
		buffer.resize(len);
		is.read((char*)buffer.data(), len);
		buffer.resize(static_cast<size_t>(is.gcount())); // to-do: adjust if EOF hit early
		return true;
	}

	bool vfs_readRanges( pod::Mount& mount, const uf::stl::string& file, const uf::stl::vector<pod::Range>& ranges, uf::stl::vector<uint8_t>& buffer ) {
		uf::stl::string path = mount.path + file;
		std::ifstream is(path, std::ios::binary);
		if (!is.is_open()) return false;

		size_t totalBytes = 0;
		for (const auto& r : ranges) totalBytes += r.len;
		buffer.resize(totalBytes);

		size_t currentOffset = 0;
		for (const auto& r : ranges) {
			is.seekg(r.start, std::ios::beg);
			is.read((char*)(buffer.data() + currentOffset), r.len);
			currentOffset += static_cast<size_t>(is.gcount());
		}
		buffer.resize(currentOffset);
		return true;
	}

	bool vfs_stream( pod::Mount& mount, const uf::stl::string& file, size_t chunkSize, std::function<bool(const uint8_t* data, size_t size)> callback ) {
		uf::stl::string path = mount.path + file;
		std::ifstream is(path, std::ios::binary);
		if ( !is.is_open() ) return false;

		uf::stl::vector<uint8_t> buffer(chunkSize);
		while ( is.good() ) {
			is.read((char*)buffer.data(), chunkSize);
			size_t bytesRead = is.gcount();
			if ( bytesRead == 0 ) break;

			if ( !callback(buffer.data(), bytesRead) ) break;
		}
		return true;
	}

	//
	size_t disk_file_read(void* handle, void* buffer, size_t bytes) {
		return fread(buffer, 1, bytes, (FILE*)handle);
	}
	bool disk_file_seek(void* handle, long offset, int origin) {
		return fseek((FILE*)handle, offset, origin) == 0;
	}
	size_t disk_file_tell(void* handle) {
		return ftell((FILE*)handle);
	}
	void disk_file_close(void* handle) {
		if (handle) fclose((FILE*)handle);
	}

	pod::File vfs_open( pod::Mount& mount, const uf::stl::string& file ) {
		uf::stl::string path = mount.path + file;
		FILE* f = fopen(path.c_str(), "rb");
		if ( !f ) return pod::File{};

		return pod::File{
			.handle = f,
			.read = disk_file_read,
			.seek = disk_file_seek,
			.tell = disk_file_tell,
			.close = disk_file_close
		};
	}

	// what a mess
	struct FallbackFileState {
		pod::Mount* mount;
		uf::stl::string relativePath;
		size_t offset;
		size_t totalSize;
	};

	size_t fallback_file_read(void* handle, void* buffer, size_t bytes) {
		auto* state = (FallbackFileState*)(handle);
		if (!state || !state->mount || !state->mount->readRange) return 0;

		size_t bytesLeft = state->totalSize - state->offset;
		size_t toRead = std::min(bytes, bytesLeft);
		if (toRead == 0) return 0;

		uf::stl::vector<uint8_t> tempBuffer;
		if (state->mount->readRange(*state->mount, state->relativePath, state->offset, toRead, tempBuffer)) {
			std::memcpy(buffer, tempBuffer.data(), tempBuffer.size());
			state->offset += tempBuffer.size();
			return tempBuffer.size();
		}
		return 0;
	}

	bool fallback_file_seek(void* handle, long offset, int origin) {
		auto* state = (FallbackFileState*)(handle);
		if (!state) return false;

		long long targetOffset = 0;
		if (origin == SEEK_SET) targetOffset = offset;
		else if (origin == SEEK_CUR) targetOffset = (long long)state->offset + offset;
		else if (origin == SEEK_END) targetOffset = (long long)state->totalSize + offset;

		if (targetOffset < 0) targetOffset = 0;
		if (targetOffset > (long long)state->totalSize) targetOffset = state->totalSize;

		state->offset = (size_t)targetOffset;
		return true;
	}

	size_t fallback_file_tell(void* handle) {
		auto* state = (FallbackFileState*)(handle);
		return state ? state->offset : 0;
	}

	void fallback_file_close(void* handle) {
		auto* state = (FallbackFileState*)(handle);
		if (state) delete state;
	}
}

uf::vfs::Mount::~Mount() {
	if ( temp ) uf::vfs::unmount( hash );
}

pod::Mount uf::vfs::createDiskMount( const uf::stl::string& uri, int priority) {
	uf::stl::string prefix;
	uf::stl::string path;
	uf::io::splitUri( uri, prefix, path );
	if ( !path.empty() && path.back() != '/' && path.back() != '\\' ) path += '/';

	return pod::Mount{
		.prefix = prefix,
		.path = path,
		.priority = priority,
		.exists = ::vfs_exists,
		.size = ::vfs_size,
		.mtime = ::vfs_mtime,
		.read = ::vfs_read,
		.write = ::vfs_write,
		.mkdir = ::vfs_mkdir,
		.stream = ::vfs_stream,
		.open = ::vfs_open,
		.list = ::vfs_list,
		.readRange = ::vfs_readRange,
		.readRanges = ::vfs_readRanges,
	};
}

uf::stl::vector<pod::Mount> uf::vfs::mounts;
uf::vfs::Mount uf::vfs::mount( const pod::Mount& mount, bool temp ) {
	// compute hash
	size_t hash = {};
	uf::hash( hash, mount.prefix, mount.path );

	// already exists
	for ( auto& m : mounts ) {
		size_t hash2 = {};
		uf::hash( hash2, m.prefix, m.path );

		if ( hash == hash2 ) {
			return uf::vfs::Mount{ hash, NULL, false }; // do not honor temp request to avoid breaking mounts in the future
		}
	}

	// add mount
	mounts.emplace_back( mount );

	// resort
	std::sort( mounts.begin(), mounts.end(), [](const pod::Mount& a, const pod::Mount& b) {
		return a.priority > b.priority;
	});

	return uf::vfs::Mount{ hash, &mount, temp };
}

bool uf::vfs::unmount( size_t hash ) {
	auto it = std::remove_if( mounts.begin(), mounts.end(), [&](const pod::Mount& m) {
		size_t hash2 = {};
		uf::hash( hash2, m.prefix, m.path );
		return hash == hash2;
	});

	if ( it == mounts.end() ) return false;

	if ( it->userdata.len ) {
		uf::pointeredUserdata::destroy( it->userdata );
	}
	mounts.erase( it, mounts.end() );
	return true;
}
bool uf::vfs::unmount( const uf::vfs::Mount& mount ) {
	return uf::vfs::unmount( mount.hash );
}
bool uf::vfs::unmount( const uf::stl::string& prefix, const uf::stl::string& base ) {
	uf::stl::string cleanBase = base;
	if ( !cleanBase.empty() && cleanBase.back() != '/' && cleanBase.back() != '\\' ) cleanBase += '/';

	size_t hash = {};
	uf::hash( hash, prefix, cleanBase );

	// erase by hash first
	if ( uf::vfs::unmount( hash ) ) return true;

	auto it = std::remove_if( mounts.begin(), mounts.end(), [&](const pod::Mount& m) {
		return m.prefix == prefix && m.path == cleanBase;
	});

	if ( it == mounts.end() ) return false;
	
	if ( it->userdata.len ) {
		uf::pointeredUserdata::destroy( it->userdata );
	}

	mounts.erase( it, mounts.end() );
	return true;
}

bool uf::vfs::exists( const uf::stl::string& path ) {
	uf::stl::string prefix, relative;
	uf::io::splitUri(path, prefix, relative);

	for ( auto& mount : mounts ) {
		if ( prefix.empty() && mount.priority < 0 ) continue;
		if ( prefix.empty() || mount.prefix == prefix ) {
			if ( mount.exists( mount, relative ) ) return true;
		}
	}
	return false;
}

uf::stl::vector<uf::stl::string> uf::vfs::list( const uf::stl::string& path, const uf::stl::string& extension, bool recursive ) {
	uf::stl::vector<uf::stl::string> results;
	uf::stl::string prefix, relative;
	uf::io::splitUri(path, prefix, relative);

	if ( !relative.empty() && relative.back() != '/' ) relative += '/';

	for ( auto& mount : mounts ) {
		if ( prefix.empty() && mount.priority < 0 ) continue;
		if ( prefix.empty() || mount.prefix == prefix ) {
			if ( mount.list ) {
				auto files = mount.list( mount, relative, extension, recursive );
				results.insert(results.end(), files.begin(), files.end());
			}
		}
	}

	return results;
}

bool uf::vfs::mkdir( const uf::stl::string& path ) {
	uf::stl::string prefix, relative;
	uf::io::splitUri(path, prefix, relative);

	for ( auto& mount : mounts ) {
		if ( prefix.empty() && mount.priority < 0 ) continue;
		if ( prefix.empty() || mount.prefix == prefix ) {
			if ( mount.mkdir( mount, relative ) ) return true;
		}
	}
	return false;
}

size_t uf::vfs::size( const uf::stl::string& path ) {
	uf::stl::string prefix, relative;
	uf::io::splitUri(path, prefix, relative);

	for ( auto& mount : mounts ) {
		if ( prefix.empty() && mount.priority < 0 ) continue;
		if ( prefix.empty() || mount.prefix == prefix ) {
			if ( mount.exists( mount, relative ) ) return mount.size( mount, relative );
		}
	}
	return 0;
}

size_t uf::vfs::mtime( const uf::stl::string& path ) {
	uf::stl::string prefix, relative;
	uf::io::splitUri(path, prefix, relative);

	for ( auto& mount : mounts ) {
		if ( prefix.empty() && mount.priority < 0 ) continue;
		if ( prefix.empty() || mount.prefix == prefix ) {
			if ( mount.exists( mount, relative ) ) return mount.mtime( mount, relative );
		}
	}
	return 0;
}

bool uf::vfs::read( const uf::stl::string& path, uf::stl::vector<uint8_t>& buffer ) {
	uf::stl::string prefix, relative;
	uf::io::splitUri(path, prefix, relative);

	for ( auto& mount : mounts ) {
		if ( prefix.empty() && mount.priority < 0 ) continue;
		if ( prefix.empty() || mount.prefix == prefix ) {
			bool res = mount.exists( mount, relative );
			if ( mount.exists( mount, relative ) && mount.read( mount, relative, buffer ) ) return true;;
		}
	}
	return false;
}

size_t uf::vfs::write( const uf::stl::string& path, const void* buffer, size_t size ) {
	uf::stl::string prefix, relative;
	uf::io::splitUri(path, prefix, relative);

	for ( auto& mount : mounts ) {
		if ( prefix.empty() && mount.priority < 0 ) continue;
		if ( prefix.empty() || mount.prefix == prefix ) {
			if ( mount.write && mount.write( mount, relative, buffer, size ) ) return true;
		}
	}
	return 0;
}

size_t uf::vfs::write( const uf::stl::string& path, uf::stl::vector<uint8_t>& buffer ) {
	return uf::vfs::write( path, buffer.data(), buffer.size() );
}

bool uf::vfs::readRange( const uf::stl::string& path, size_t start, size_t len, uf::stl::vector<uint8_t>& buffer ) {
	uf::stl::string prefix, relative;
	uf::io::splitUri(path, prefix, relative);
	for ( auto& mount : mounts ) {
		if ( prefix.empty() && mount.priority < 0 ) continue;
		if ( prefix.empty() || mount.prefix == prefix ) {
			if ( !mount.exists( mount, relative ) ) continue;
			if ( mount.readRange ) return mount.readRange( mount, relative, start, len, buffer );
			if ( !mount.read ) continue;
			
			uf::stl::vector<uint8_t> fullBuffer;
			if ( !mount.read( mount, relative, fullBuffer ) ) continue;
			//UF_MSG_DEBUG("hitting fallback: {}", path);
			
			if ( start < fullBuffer.size() ) {
				size_t actualLen = std::min(len, fullBuffer.size() - start);
				buffer.assign(fullBuffer.begin() + start, fullBuffer.begin() + start + actualLen);
			} else {
				buffer.clear();
			}
			return true;
		}
	}
	return false;
}

bool uf::vfs::readRanges( const uf::stl::string& path, const uf::stl::vector<pod::Range>& ranges, uf::stl::vector<uint8_t>& buffer ) {
	uf::stl::string prefix, relative;
	uf::io::splitUri(path, prefix, relative);
	for ( auto& mount : mounts ) {
		if ( prefix.empty() && mount.priority < 0 ) continue;
		if ( prefix.empty() || mount.prefix == prefix ) {
			if ( !mount.exists( mount, relative ) ) continue;
			if ( mount.readRanges ) return mount.readRanges( mount, relative, ranges, buffer );
			if ( !mount.read ) continue;
			
			uf::stl::vector<uint8_t> fullBuffer;
			if ( !mount.read( mount, relative, fullBuffer ) ) continue;
			//UF_MSG_DEBUG("hitting fallback: {}", path);

			size_t totalBytes = 0;
			for ( const auto& r : ranges ) {
				if ( r.start < fullBuffer.size() ) {
					totalBytes += std::min(r.len, fullBuffer.size() - r.start);
				}
			}

			buffer.clear();
			buffer.reserve(totalBytes);

			for ( const auto& r : ranges ) {
				if ( r.start < fullBuffer.size() ) {
					size_t actualLen = std::min(r.len, fullBuffer.size() - r.start);
					buffer.insert(buffer.end(), fullBuffer.begin() + r.start, fullBuffer.begin() + r.start + actualLen);
				}
			}
			return true;
		}
	}
	return false;
}
bool uf::vfs::stream( const uf::stl::string& path, size_t chunkSize, std::function<bool(const uint8_t* data, size_t size)> callback ) {
	uf::stl::string prefix, relative;
	uf::io::splitUri(path, prefix, relative);
	for ( auto& mount : mounts ) {
		if ( prefix.empty() && mount.priority < 0 ) continue;
		if ( prefix.empty() || mount.prefix == prefix ) {
			if ( !mount.exists( mount, relative ) ) continue;
			if ( mount.stream ) return mount.stream( mount, relative, chunkSize, callback );

			if ( !mount.read ) continue;
			//UF_MSG_DEBUG("hitting fallback: {}", path);

			uf::stl::vector<uint8_t> fullBuffer;
			if ( !mount.read( mount, relative, fullBuffer ) ) continue;

			size_t offset = 0;
			size_t totalSize = fullBuffer.size();

			while ( offset < totalSize ) {
				size_t currentChunkSize = std::min(chunkSize, totalSize - offset);
				if ( !callback(fullBuffer.data() + offset, currentChunkSize) ) break;
				offset += currentChunkSize;
			}

			return true;
		}
	}
	return false;
}

pod::File uf::vfs::open( const uf::stl::string& path ) {
	uf::stl::string prefix, relative;
	uf::io::splitUri(path, prefix, relative);

	for ( auto& mount : mounts ) {
		if ( prefix.empty() && mount.priority < 0 ) continue;
		if ( prefix.empty() || mount.prefix == prefix ) {
			if ( mount.open ) {
				pod::File file = mount.open( mount, relative );
				if ( file ) return file;
			}
			if ( mount.exists && mount.exists(mount, relative) ) {
				if ( mount.readRange && mount.size ) {
					FallbackFileState* state = new FallbackFileState{
						&mount,
						relative,
						0,
						mount.size(mount, relative)
					};

					return pod::File{
						.handle = state,
						.read = fallback_file_read,
						.seek = fallback_file_seek,
						.tell = fallback_file_tell,
						.close = fallback_file_close
					};
				}
			}
		}
	}
	return pod::File{};
}

uf::stl::string uf::vfs::resolveBase( const uf::stl::string& path ) {
	uf::stl::string prefix, relative;
	uf::io::splitUri(path, prefix, relative);

	for ( auto& mount : mounts ) {
		if ( prefix.empty() && mount.priority < 0 ) continue;
		if ( prefix.empty() || mount.prefix == prefix ) {
			uf::stl::string resolved = mount.path + relative;
			return resolved;
		}
	}

	return path;
}

#if UF_ENV_DREAMCAST
	#define VFS_BASE "/cd/"
#else
	#define VFS_BASE "./data/"
#endif

UF_VFS_MOUNT_CPP( uf::vfs::createDiskMount, "sys://", 999 );
UF_VFS_MOUNT_CPP( uf::vfs::createDiskMount, "mdl://" VFS_BASE "models", 100 );
UF_VFS_MOUNT_CPP( uf::vfs::createDiskMount, "scene://" VFS_BASE "scenes", 100 );
UF_VFS_MOUNT_CPP( uf::vfs::createDiskMount, "ent://" VFS_BASE "entities", 100 );
UF_VFS_MOUNT_CPP( uf::vfs::createDiskMount, "tex://" VFS_BASE "textures", 100 );
UF_VFS_MOUNT_CPP( uf::vfs::createDiskMount, "snd://" VFS_BASE "audio", 100 );
UF_VFS_MOUNT_CPP( uf::vfs::createDiskMount, "spv://" VFS_BASE "shaders", 100 );
UF_VFS_MOUNT_CPP( uf::vfs::createDiskMount, "lua://" VFS_BASE "scripts", 100 );
UF_VFS_MOUNT_CPP( uf::vfs::createDiskMount, "data://" VFS_BASE, 50 );
UF_VFS_MOUNT_CPP( uf::vfs::createDiskMount, "", 0 );