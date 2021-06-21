#pragma once

#include <uf/config.h>
#include <string>
#include <vector>
#include <fstream>

namespace uf {
	namespace io {
		extern UF_API const std::string root;

		std::string UF_API absolute( const std::string& );
		std::string UF_API filename( const std::string& );
		std::string UF_API basename( const std::string& );
		std::string UF_API extension( const std::string& );
		std::string UF_API extension( const std::string&, int32_t );
		std::string UF_API directory( const std::string& );
		std::string UF_API sanitize( const std::string&, const std::string& = "" );
		size_t UF_API size( const std::string& );
		
		std::string UF_API readAsString( const std::string&, const std::string& = "" );
		std::vector<uint8_t> UF_API readAsBuffer( const std::string&, const std::string& = "" );

		size_t UF_API write( const std::string& filename, const void*, size_t = SIZE_MAX );
		template<typename T> inline size_t write( const std::string& filename, const std::vector<T>& buffer, size_t size = SIZE_MAX ) {
			return write( filename, buffer.data(), std::min( buffer.size(), size ) );
		}
		inline size_t write( const std::string& filename, const std::string& string, size_t size = SIZE_MAX ) {
			return write( filename, string.c_str(), std::min( string.size(), size ) );
		}
		
		std::vector<uint8_t> UF_API decompress( const std::string& );

		size_t UF_API compress( const std::string&, const void*, size_t = SIZE_MAX );
		template<typename T> inline size_t compress( const std::string& filename, const std::vector<T>& buffer, size_t size = SIZE_MAX ) {
			return compress( filename, buffer.data(), std::min( buffer.size(), size ) );
		}
		inline size_t compress( const std::string& filename, const std::string& string, size_t size = SIZE_MAX ) {
			return compress( filename, string.c_str(), std::min( string.size(), size ) );
		}

		std::string UF_API hash( const std::string& );
		bool UF_API exists( const std::string& );
		size_t UF_API mtime( const std::string& );
		bool UF_API mkdir( const std::string& );
		std::string UF_API resolveURI( const std::string&, const std::string& = "" );
	}
}