#if UF_USE_VALVE
#include <uf/ext/valve/bsp.h>
#include <uf/ext/valve/mdl.h>
#include <uf/ext/valve/vtf.h>
#include <uf/ext/valve/vpk.h>
#include <uf/ext/valve/common.h>

#include <uf/utils/io/vfs.h>
#include <uf/utils/userdata/userdata.h>
#include <uf/engine/asset/asset.h>
#include <uf/utils/memory/memcpy.h>

#if defined(_WIN32)
	#include <windows.h>
#else
	#include <cstdlib>
#endif
#include <fstream>

namespace impl {
	uf::stl::vector<uf::stl::string> getSteamLibraries() {
		uf::stl::vector<uf::stl::string> libraries;
		uf::stl::string steamPath = "";

	#if defined(_WIN32)
		HKEY hKey;
		if ( RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Valve\\Steam", 0, KEY_READ, &hKey) == ERROR_SUCCESS ) {
			char pathBuf[MAX_PATH];
			DWORD bufferSize = sizeof(pathBuf);
			if ( RegQueryValueExA(hKey, "SteamPath", nullptr, nullptr, (LPBYTE)pathBuf, &bufferSize) == ERROR_SUCCESS ) {
				steamPath = pathBuf;
			}
			RegCloseKey(hKey);
		}
	#else
		const char* home = std::getenv("HOME");
		if (home) {
		#if defined(__APPLE__)
			steamPath = uf::stl::string(home) + "/Library/Application Support/Steam";
		#else
			steamPath = uf::stl::string(home) + "/.steam/steam"; // or ~/.local/share/Steam
		#endif
		}
	#endif

		if ( steamPath.empty() ) {
			UF_MSG_WARNING("Could not locate base Steam installation.");
			return libraries;
		}

		libraries.emplace_back(steamPath + "/steamapps/common");

		uf::stl::string vdfPath = steamPath + "/steamapps/libraryfolders.vdf";
		std::ifstream file(vdfPath);
		if ( !file.is_open() ) return libraries;

		uf::stl::string line;
		while ( std::getline(file, line) ) {
			uf::stl::string key, value;

			if ( !impl::parseKeyValue(line, key, value) ) continue;
			if ( key != "path" ) continue;
			
			std::replace( value.begin(), value.end(), '\\', '/' );
			value = uf::string::replace(value, "//", "/");

			libraries.emplace_back(value + "/steamapps/common");
		}

		return libraries;
	}
}

namespace {
	struct VpkMountState {
		uf::stl::string path;
		uf::stl::string name;

		bool loaded = false;
		pod::VpkArchive* archive = NULL;
		
		pod::VpkArchive* get() {
			if ( !loaded ) {
				loaded = true;
		
				archive = &uf::asset::add<pod::VpkArchive>(name);
				ext::valve::loadVpk( *archive, path );
			}
			return archive;
		}
	};

	bool exists( pod::Mount& mount, const uf::stl::string& path ) {
		auto& state = uf::pointeredUserdata::get<VpkMountState>( mount.userdata );
		auto archive = state.get();
		return archive && archive->files.count( uf::string::lowercase( path ) ) > 0;
	};
	size_t size( pod::Mount& mount, const uf::stl::string& path ) {
		auto& state = uf::pointeredUserdata::get<VpkMountState>( mount.userdata );
		auto archive = state.get();
		if ( !archive ) return 0;
		auto it = archive->files.find( uf::string::lowercase( path ) );
		return it != archive->files.end() ? (it->second.metadata.preloadBytes + it->second.metadata.entryLength) : 0;
	};
	size_t mtime( pod::Mount& mount, const uf::stl::string& ) {
		return 0;
	};
	bool _read( pod::Mount& mount, const uf::stl::string& path, uf::stl::vector<uint8_t>& buffer) {
		auto& state = uf::pointeredUserdata::get<VpkMountState>( mount.userdata );
		auto archive = state.get();
		return archive && ext::valve::readVpk(*archive, uf::string::lowercase( path ), buffer);
	};
	bool readRange( pod::Mount& mount, const uf::stl::string& path, size_t start, size_t len, uf::stl::vector<uint8_t>& buffer) {
		auto& state = uf::pointeredUserdata::get<VpkMountState>( mount.userdata );
		auto archive = state.get();
		return archive && ext::valve::readVpkRange(*archive, uf::string::lowercase( path ), start, len, buffer);
	};
	uf::stl::vector<uf::stl::string> list( pod::Mount& mount, const uf::stl::string& dir, const uf::stl::string& extension = "", bool recursive = false ) {
		uf::stl::vector<uf::stl::string> results;
		auto& state = uf::pointeredUserdata::get<VpkMountState>( mount.userdata );
		auto archive = state.get();
		if ( !archive ) return results;

		uf::stl::string searchDir = uf::string::lowercase(dir);
		uf::stl::string searchExt = uf::string::lowercase(extension);

		for ( const auto& [filePath, entry] : archive->files ) {
			if ( !searchDir.empty() && !filePath.starts_with(searchDir) ) continue;

			if ( !recursive ) {
				uf::stl::string remainder = filePath.substr(searchDir.length());
				if ( remainder.find('/') != uf::stl::string::npos ) continue;
			}

			if ( !searchExt.empty() && !filePath.ends_with(searchExt) ) continue;

			results.emplace_back(filePath);
		}

		return results;
	};

	struct VpkFileStreamContext {
		size_t preloadBytes = 0;
		size_t entryLength = 0;
		const uint8_t* preloadData = nullptr;

		FILE* chunkHandle = nullptr;
		size_t chunkBaseOffset = 0;

		size_t cursor = 0;
	};

	size_t vpk_file_read(void* handle, void* buffer, size_t bytes) {
		VpkFileStreamContext* ctx = (VpkFileStreamContext*)handle;
		size_t totalSize = ctx->preloadBytes + ctx->entryLength;

		if (ctx->cursor >= totalSize) return 0;

		size_t bytesToRead = std::min(bytes, totalSize - ctx->cursor);
		size_t bytesRead = 0;
		uint8_t* out = (uint8_t*)buffer;

		if (ctx->cursor < ctx->preloadBytes) {
			size_t preloadRead = std::min(bytesToRead, ctx->preloadBytes - ctx->cursor);
			uf::stl::memcpy(out, ctx->preloadData + ctx->cursor, preloadRead);

			ctx->cursor += preloadRead;
			out += preloadRead;
			bytesRead += preloadRead;
			bytesToRead -= preloadRead;
		}

		if (bytesToRead > 0 && ctx->chunkHandle) {
			size_t physicalOffset = ctx->chunkBaseOffset + (ctx->cursor - ctx->preloadBytes);
			fseek(ctx->chunkHandle, physicalOffset, SEEK_SET);

			size_t chunkRead = fread(out, 1, bytesToRead, ctx->chunkHandle);
			ctx->cursor += chunkRead;
			bytesRead += chunkRead;
		}

		return bytesRead;
	}

	bool vpk_file_seek(void* handle, long offset, int origin) {
		VpkFileStreamContext* ctx = (VpkFileStreamContext*)handle;
		long newCursor = ctx->cursor;
		long totalSize = ctx->preloadBytes + ctx->entryLength;

		if (origin == SEEK_SET) newCursor = offset;
		else if (origin == SEEK_CUR) newCursor += offset;
		else if (origin == SEEK_END) newCursor = totalSize + offset;

		if (newCursor < 0) newCursor = 0;
		if (newCursor > totalSize) newCursor = totalSize;

		ctx->cursor = newCursor;
		return true;
	}

	size_t vpk_file_tell(void* handle) {
		return ((VpkFileStreamContext*)handle)->cursor;
	}

	void vpk_file_close(void* handle) {
		VpkFileStreamContext* ctx = (VpkFileStreamContext*)handle;
		if (ctx->chunkHandle) fclose(ctx->chunkHandle);
		delete ctx;
	}

	pod::File open(pod::Mount& mount, const uf::stl::string& path) {
		auto& state = uf::pointeredUserdata::get<VpkMountState>( mount.userdata );
		auto archive = state.get();
		if ( !archive ) return pod::File{};

		auto it = archive->files.find( uf::string::lowercase(path) );
		if ( it == archive->files.end() ) return pod::File{};

		const auto& entry = it->second;

		VpkFileStreamContext* ctx = new VpkFileStreamContext();
		ctx->preloadBytes = entry.metadata.preloadBytes;
		ctx->entryLength = entry.metadata.entryLength;
		ctx->preloadData = entry.metadata.preloadBytes > 0 ? entry.preloadData.data() : nullptr;
		ctx->cursor = 0;

		if ( ctx->entryLength > 0 ) {
			uf::stl::string archivePath;

			if ( entry.metadata.archiveIndex == 0x7FFF ) {
				archivePath = archive->basePath + "_dir.vpk";
				ctx->chunkBaseOffset = entry.dirFileOffset + entry.metadata.entryOffset;
			} else {
				archivePath = FMT_FORMAT("{}_{:03d}.vpk", archive->basePath, entry.metadata.archiveIndex);
				ctx->chunkBaseOffset = entry.metadata.entryOffset;
			}

			ctx->chunkHandle = fopen(archivePath.c_str(), "rb");
			if ( !ctx->chunkHandle ) {
				UF_MSG_ERROR("Failed to open VPK chunk for streaming: {}", archivePath);
				delete ctx;
				return pod::File{};
			}
		}

		return pod::File{
			.handle = ctx,
			.read = vpk_file_read,
			.seek = vpk_file_seek,
			.tell = vpk_file_tell,
			.close = vpk_file_close
		};
	}
}

pod::Mount ext::valve::createVpkMount( const uf::stl::string& uri, int priority ) {
	uf::stl::string prefix;
	uf::stl::string path;
	uf::io::splitUri( uri, prefix, path );

	pod::Mount mount;
	mount.prefix = prefix;
	mount.path = path;
	mount.priority = priority;
	mount.userdata = uf::pointeredUserdata::create<VpkMountState>();
	mount.exists = ::exists;
	mount.size = ::size;
	mount.list = ::list;
	mount.open = ::open;
	mount.mtime = ::mtime;
	mount.read = ::_read;
	mount.readRange = ::readRange;
	
	auto& state = uf::pointeredUserdata::get<VpkMountState>( mount.userdata );
	auto libraries = impl::getSteamLibraries();
	for ( const auto& lib : libraries ) {
		uf::stl::string fullPath = lib + "/" + path;
		if ( !uf::io::exists(fullPath) ) {
			continue;
		}
		state.path = fullPath;
		break;
	}

	if ( state.path.empty() ) {
		UF_MSG_WARNING("Could not resolve VPK path: {}", path);
		state.path = path;
	} else {
		UF_MSG_DEBUG("Mounted VPK: {}", state.path);
	}
	state.name = FMT_FORMAT("vpk://{}", path);

	return mount;
}
bool ext::valve::loadVpk( pod::VpkArchive& vpk, const uf::stl::string& path ) {
	uf::stl::vector<uint8_t> buffer;
	if ( !uf::io::readAsBuffer(buffer, path) ) return false;

	size_t dirPos = path.find("_dir.vpk");
	if ( dirPos == uf::stl::string::npos ) return false;
	vpk.basePath = path.substr(0, dirPos);

	size_t offset = 0;

	auto readBytes = [&](void* dest, size_t len) -> bool {
		if (offset + len > buffer.size()) return false;
		uf::stl::memcpy(dest, buffer.data() + offset, len);
		offset += len;
		return true;
	};

	auto readString = [&]() -> uf::stl::string {
		uf::stl::string str;
		while (offset < buffer.size()) {
			char c = (char)buffer[offset++];
			if (c == '\0') break;
			str += c;
		}
		return str;
	};

	uint32_t signature;
	if ( !readBytes(&signature, 4) || signature != 0x55aa1234 ) return false;

	uint32_t version, treeSize;
	if ( !readBytes(&version, 4) || !readBytes(&treeSize, 4) ) return false;

	// skip for now
	if ( version == 2 ) offset += 16;

	uint32_t headerSize = (version == 1) ? 12 : 28;

	while ( true ) {
		uf::stl::string ext = readString();
		if ( ext.empty() ) break;

		while ( true ) {
			uf::stl::string dir = readString();
			if ( dir.empty() ) break;

			while ( true ) {
				uf::stl::string name = readString();
				if ( name.empty() ) break;

				// construct path
				uf::stl::string fullPath = dir == " " ? "" : (dir + "/");
				fullPath += name + "." + ext;
				std::replace( fullPath.begin(), fullPath.end(), '\\', '/' );
				std::transform( fullPath.begin(), fullPath.end(), fullPath.begin(), ::tolower );

				// read data
				auto& entry = vpk.files[fullPath];
				//UF_MSG_DEBUG("path={}, entry={}", path, fullPath);
				if ( !readBytes(&entry.metadata, sizeof(pod::VpkData)) ) return false;

				if ( entry.metadata.preloadBytes > 0 ) {
					entry.preloadData.resize(entry.metadata.preloadBytes);
					if ( !readBytes(entry.preloadData.data(), entry.metadata.preloadBytes) ) return false;
				}

				entry.dirFileOffset = headerSize + treeSize;
			}
		}
	}
	return true;
}

bool ext::valve::readVpk( const pod::VpkArchive& vpk, const uf::stl::string& path, uf::stl::vector<uint8_t>& buffer ) {
	auto it = vpk.files.find( path );
	if ( it == vpk.files.end() ) return false;

	const auto& entry = it->second;
	buffer.resize( entry.metadata.preloadBytes + entry.metadata.entryLength );

	// copy preload
	if ( entry.metadata.preloadBytes > 0 ) {
		uf::stl::memcpy(buffer.data(), entry.preloadData.data(), entry.metadata.preloadBytes);
	}

	// read payload from disk
	if ( entry.metadata.entryLength > 0 ) {
		uf::stl::string archivePath;
		size_t fileOffset = 0;

		if ( entry.metadata.archiveIndex == 0x7FFF ) {
			// data is embedded inside _dir.vpk
			archivePath = vpk.basePath + "_dir.vpk";
			fileOffset = entry.dirFileOffset + entry.metadata.entryOffset;
		} else {
			// data is in a chunk file
			archivePath = FMT_FORMAT("{}_{:03d}.vpk", vpk.basePath, entry.metadata.archiveIndex);
			fileOffset = entry.metadata.entryOffset;
		}

		std::ifstream file(archivePath, std::ios::binary);
		if ( file ) {
			file.seekg(fileOffset, std::ios::beg);
			file.read((char*)(buffer.data() + entry.metadata.preloadBytes), entry.metadata.entryLength);
		} else {
			buffer.clear();
			UF_MSG_ERROR("Failed to open VPK chunk: {}", archivePath);
			return false;
		}
	}

	return true;
}

uf::vfs::Mount ext::valve::mountVpk( const uf::stl::string& uri, bool temp ) {
	return uf::vfs::mount( ext::valve::createVpkMount( FMT_FORMAT( "valve://{}", uri ), 10 ), temp );
}
uf::vfs::Mount ext::valve::mountGame( const uf::stl::string& uri, bool temp ) {
	uf::stl::string path = "";
	auto libraries = impl::getSteamLibraries();
	for ( const auto& lib : libraries ) {
		uf::stl::string fullPath = lib + "/" + uri;
		if ( !uf::io::exists(fullPath) ) {
			continue;
		}
		path = fullPath;
		break;
	}
	if ( path.empty() ) {
		UF_MSG_ERROR("Failed to mount game: {}", uri);
		return { 0 };
	}
	return uf::vfs::mount( uf::vfs::createDiskMount( FMT_FORMAT( "game://{}", path ), 11 ), temp ); // for some reason a lower priority makes the footstep sound CS:S's
}
bool ext::valve::readVpkRange( const pod::VpkArchive& vpk, const uf::stl::string& path, size_t start, size_t len, uf::stl::vector<uint8_t>& buffer ) {
	auto it = vpk.files.find( path );
	if ( it == vpk.files.end() ) return false;

	const auto& entry = it->second;
	size_t totalSize = entry.metadata.preloadBytes + entry.metadata.entryLength;

	if ( start >= totalSize ) {
		buffer.clear();
		return true;
	}

	len = std::min(len, totalSize - start);
	buffer.resize(len);

	size_t bufferOffset = 0;

	if ( start < entry.metadata.preloadBytes ) {
		size_t preloadRead = std::min(len, (size_t)entry.metadata.preloadBytes - start);
		uf::stl::memcpy(buffer.data(), entry.preloadData.data() + start, preloadRead);

		bufferOffset += preloadRead;
		start += preloadRead;
		len -= preloadRead;
	}

	if ( len > 0 ) {
		size_t diskStart = start - entry.metadata.preloadBytes;

		uf::stl::string archivePath;
		size_t fileOffset = 0;

		if ( entry.metadata.archiveIndex == 0x7FFF ) {
			archivePath = vpk.basePath + "_dir.vpk";
			fileOffset = entry.dirFileOffset + entry.metadata.entryOffset;
		} else {
			archivePath = FMT_FORMAT("{}_{:03d}.vpk", vpk.basePath, entry.metadata.archiveIndex);
			fileOffset = entry.metadata.entryOffset;
		}

		std::ifstream file(archivePath, std::ios::binary);
		if ( file ) {
			file.seekg(fileOffset + diskStart, std::ios::beg);
			file.read((char*)(buffer.data() + bufferOffset), len);

			size_t actuallyRead = static_cast<size_t>(file.gcount());

			if (actuallyRead < len) {
				buffer.resize(bufferOffset + actuallyRead);
				if (actuallyRead == 0 && buffer.empty()) {
					return false;
				}
			}
		} else {
			buffer.clear();
			UF_MSG_ERROR("Failed to open VPK chunk for ranged read: {}", archivePath);
			return false;
		}
	}

	return true;
}
#endif
