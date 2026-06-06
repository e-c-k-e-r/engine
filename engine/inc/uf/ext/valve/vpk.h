#pragma once

#include <uf/config.h>
#include <uf/utils/io/vfs.h>

namespace pod {
#pragma pack(push, 1)
	struct VpkData {
		uint32_t crc;
		uint16_t preloadBytes;
		uint16_t archiveIndex;
		uint32_t entryOffset;
		uint32_t entryLength;
		uint16_t terminator;
	};
#pragma pack(pop)

	struct VpkFile {
		VpkData metadata;
		uint32_t dirFileOffset;
		uf::stl::vector<uint8_t> preloadData;
	};

	struct VpkArchive {
		uf::stl::string basePath;
		uf::stl::unordered_map<uf::stl::string, VpkFile> files;
	};
};

namespace ext {
	namespace valve {
		bool UF_API loadVpk( pod::VpkArchive& vpk, const uf::stl::string& filename );
		bool UF_API readVpk( const pod::VpkArchive& vpk, const uf::stl::string& filename, uf::stl::vector<uint8_t>& buffer );
		bool UF_API readVpkRange( const pod::VpkArchive& vpk, const uf::stl::string& path, size_t start, size_t len, uf::stl::vector<uint8_t>& buffer );

		size_t UF_API mountVpk( const uf::stl::string& uri );
		pod::Mount UF_API createVpkMount( const uf::stl::string& uri, int priority = 0 );
	}
}