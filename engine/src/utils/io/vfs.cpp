#include <uf/utils/io/vfs.h>
#include <uf/utils/io/file.h>
#include <uf/utils/math/hash.h>
#include <algorithm>
#include <sys/stat.h>

namespace {
	bool vfs_exists( pod::Mount& mount, const uf::stl::string& file ) {
		uf::stl::string path = mount.path + file;
		#if UF_ENV_DREAMCAST
			FILE* file = fopen(path.c_str(), "r");
			if ( file ) {
				fclose(file);
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
		.readRange = ::vfs_readRange,
		.readRanges = ::vfs_readRanges
	};
}

uf::stl::vector<pod::Mount> uf::vfs::mounts;
size_t uf::vfs::mount( const pod::Mount& mount ) {
	// compute hash
	size_t hash = {};
	uf::hash( hash, mount.prefix, mount.path );

	// already exists
	for ( auto& m : mounts ) {
		size_t hash2 = {};
		uf::hash( hash2, m.prefix, m.path );
		if ( hash == hash2 ) {
			return hash;
		}
	}

	// add mount
	mounts.emplace_back( mount );

	// resort
	std::sort( mounts.begin(), mounts.end(), [](const pod::Mount& a, const pod::Mount& b) {
		return a.priority > b.priority;
	});

	return hash;
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
			if ( mount.exists( mount, relative ) ) return mount.read( mount, relative, buffer );
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
			if ( mount.write ) return mount.write( mount, relative, buffer, size );
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

UF_VFS_MOUNT_CPP( uf::vfs::createDiskMount, "mdl://./data/models", 100 );
UF_VFS_MOUNT_CPP( uf::vfs::createDiskMount, "scene://./data/scenes", 100 );
UF_VFS_MOUNT_CPP( uf::vfs::createDiskMount, "ent://./data/entities", 100 );
UF_VFS_MOUNT_CPP( uf::vfs::createDiskMount, "tex://./data/textures", 100 );
UF_VFS_MOUNT_CPP( uf::vfs::createDiskMount, "snd://./data/audio", 100 );
UF_VFS_MOUNT_CPP( uf::vfs::createDiskMount, "spv://./data/shaders", 100 );
UF_VFS_MOUNT_CPP( uf::vfs::createDiskMount, "lua://./data/scripts", 100 );

#if UF_USE_DREAMCAST
UF_VFS_MOUNT_CPP( uf::vfs::createDiskMount, "data:///cd/", 50 );
#else
UF_VFS_MOUNT_CPP( uf::vfs::createDiskMount, "data://./data", 50 );
#endif
UF_VFS_MOUNT_CPP( uf::vfs::createDiskMount, "", 0 );

UF_VFS_MOUNT_CPP( uf::vfs::createDiskMount, "sys://", 999 );