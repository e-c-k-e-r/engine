#pragma once

#include <uf/config.h>
#if UF_USE_ZLIB

#include <uf/utils/memory/vector.h>
#include <uf/utils/memory/string.h>
#include <uf/utils/memory/unordered_map.h>
#include <uf/utils/io/file.h>
#include <uf/utils/io/vfs.h>

namespace pod {
	struct ZipEntry {
		size_t offset;
		size_t compressedSize;
		size_t uncompressedSize;
		uint16_t compressionMethod;
	};
}

namespace ext {
	namespace zlib {
		extern UF_API size_t bufferSize;
		
		bool UF_API decompressFromFile( uf::stl::vector<uint8_t>&, const uf::stl::string& );
		bool UF_API decompressFromFile( uf::stl::vector<uint8_t>&, const uf::stl::string& filename, size_t start, size_t len );
		bool UF_API decompressFromFile( uf::stl::vector<uint8_t>&, const uf::stl::string& filename, const uf::stl::vector<pod::Range>& ranges );
		bool UF_API decompressFromMemory( uf::stl::vector<uint8_t>&, const void*, size_t, size_t );
		bool UF_API decompressScatter( const uf::stl::string& filename, uf::stl::vector<pod::ScatterRequest>& requests );
		
		size_t UF_API compressToFile( const uf::stl::string&, const void*, size_t );
		bool UF_API directory( const uf::stl::vector<uint8_t>& buffer, uf::stl::unordered_map<uf::stl::string, pod::ZipEntry>& entries );
		pod::Mount UF_API createZipMount( const uf::stl::string& uri, uf::stl::vector<uint8_t>& buffer, int priority = 0 );

		inline uf::stl::vector<uint8_t> decompressFromFile( const uf::stl::string& filename ) {
			uf::stl::vector<uint8_t> buffer;
			decompressFromFile( buffer, filename );
			return buffer;
		}
		inline uf::stl::vector<uint8_t> decompressFromFile( const uf::stl::string& filename, size_t start, size_t len ) {
			uf::stl::vector<uint8_t> buffer;
			decompressFromFile( buffer, filename, start, len );
			return buffer;
		}
		inline uf::stl::vector<uint8_t> decompressFromFile( const uf::stl::string& filename, const uf::stl::vector<pod::Range>& ranges ) {
			uf::stl::vector<uint8_t> buffer;
			decompressFromFile( buffer, filename, ranges );
			return buffer;
		}
	}
}

#endif