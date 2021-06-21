#pragma once

#include <uf/config.h>
#if UF_USE_ZLIB

#include <vector>

namespace ext {
	namespace zlib {
		extern UF_API size_t bufferSize;
	/*
		std::vector<uint8_t> UF_API compress( const void*, size_t );
		std::vector<uint8_t> UF_API decompress( const void*, size_t );

		template<typename T>
		std::vector<T> UF_API compress( const std::vector<T>& data ) {
			auto compressed = ext::zlib::compress( data.data(), data.size() );
			std::vector<T> vector( compressed.size() / sizeof(T) );
			memcpy( vector.data(), compressed.data(), compressed.size() );
		}
		template<typename T>
		std::vector<T> UF_API decompress( const std::vector<T>& data ) {
			auto compressed = ext::zlib::decompress( data.data(), data.size() );
			std::vector<T> vector( compressed.size() / sizeof(T) );
			memcpy( vector.data(), compressed.data(), compressed.size() );
		}
	*/
		std::vector<uint8_t> UF_API decompressFromFile( const std::string& );
		size_t UF_API compressToFile( const std::string&, const void*, size_t );
	}
}

#endif