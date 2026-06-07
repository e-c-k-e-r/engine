#include <uf/ext/valve/bsp.h>
#include <uf/ext/valve/mdl.h>
#include <uf/ext/valve/vtf.h>
#include <uf/ext/valve/vpk.h>
#include <uf/ext/valve/common.h>

#include <uf/utils/io/vfs.h>
#include <uf/utils/userdata/userdata.h>
#include <uf/engine/asset/asset.h>

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

pod::Mount ext::valve::createVpkMount( const uf::stl::string& uri, int priority ) {
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

	uf::stl::string prefix;
	uf::stl::string path;
	uf::io::splitUri( uri, prefix, path );

	pod::Mount mount;
	mount.prefix = prefix;
	mount.path = path;
	mount.priority = priority;
	mount.userdata = uf::pointeredUserdata::create<VpkMountState>();
	auto& state = uf::pointeredUserdata::get<VpkMountState>( mount.userdata );
	auto* userdata = &state;
	
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
	state.name = ::fmt::format("vpk://{}", path);

		
	mount.exists = [userdata](const uf::stl::string& p) {
		auto ptr = userdata->get();
		return ptr && ptr->files.count( uf::string::lowercase( p ) ) > 0;
	};
	mount.size = [userdata](const uf::stl::string& p) -> size_t {
		auto ptr = userdata->get();
		if ( !ptr ) return 0;
		auto it = ptr->files.find( uf::string::lowercase( p ) );
		return it != ptr->files.end() ? (it->second.metadata.preloadBytes + it->second.metadata.entryLength) : 0;
	};
	mount.mtime = [](const uf::stl::string&) -> size_t { return 0; },
	mount.read = [userdata](const uf::stl::string& p, uf::stl::vector<uint8_t>& buffer) {
		auto ptr = userdata->get();
		return ptr && ext::valve::readVpk(*ptr, uf::string::lowercase( p ), buffer);
	};
	mount.readRange = [userdata](const uf::stl::string& p, size_t start, size_t len, uf::stl::vector<uint8_t>& buffer) {
		auto ptr = userdata->get();
		return ptr && ext::valve::readVpkRange(*ptr, uf::string::lowercase( p ), start, len, buffer);
	};
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
		std::memcpy(dest, buffer.data() + offset, len);
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
		memcpy(buffer.data(), entry.preloadData.data(), entry.metadata.preloadBytes);
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
			archivePath = ::fmt::format("{}_{:03d}.vpk", vpk.basePath, entry.metadata.archiveIndex);
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

size_t ext::valve::mountVpk( const uf::stl::string& uri ) {
	return uf::vfs::mount( ext::valve::createVpkMount( ::fmt::format( "valve://{}", uri ), 10 ) );
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
		memcpy(buffer.data(), entry.preloadData.data() + start, preloadRead);

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
			archivePath = ::fmt::format("{}_{:03d}.vpk", vpk.basePath, entry.metadata.archiveIndex);
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