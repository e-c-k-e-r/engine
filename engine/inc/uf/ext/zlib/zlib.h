#pragma once

#include <uf/config.h>
#if UF_USE_ZLIB

#include <uf/utils/memory/vector.h>
#include <uf/utils/memory/string.h>
#include <uf/utils/io/file.h>

namespace ext {
	namespace zlib {
		extern UF_API size_t bufferSize;
	/*
		uf::stl::vector<uint8_t> UF_API compress( const void*, size_t );
		uf::stl::vector<uint8_t> UF_API decompress( const void*, size_t );

		template<typename T>
		uf::stl::vector<T> UF_API compress( const uf::stl::vector<T>& data ) {
			auto compressed = ext::zlib::compress( data.data(), data.size() );
			uf::stl::vector<T> vector( compressed.size() / sizeof(T) );
			memcpy( vector.data(), compressed.data(), compressed.size() );
		}
		template<typename T>
		uf::stl::vector<T> UF_API decompress( const uf::stl::vector<T>& data ) {
			auto compressed = ext::zlib::decompress( data.data(), data.size() );
			uf::stl::vector<T> vector( compressed.size() / sizeof(T) );
			memcpy( vector.data(), compressed.data(), compressed.size() );
		}
	*/
		
		uf::stl::vector<uint8_t> UF_API decompressFromFile( const uf::stl::string& );
		uf::stl::vector<uint8_t> UF_API decompressFromFile( const uf::stl::string& filename, size_t start, size_t len );
		uf::stl::vector<uint8_t> UF_API decompressFromFile( const uf::stl::string& filename, const uf::stl::vector<pod::Range>& ranges );

		size_t UF_API compressToFile( const uf::stl::string&, const void*, size_t );
	}
}

#endif