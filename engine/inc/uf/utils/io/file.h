#pragma once

#include <uf/config.h>
#include <uf/utils/memory/string.h>
#include <uf/utils/memory/vector.h>
#include <fstream>

namespace pod {
	struct Range {
		size_t start;
		size_t len;
	};
}

namespace uf {
	namespace io {
		extern UF_API const uf::stl::string root;

		uf::stl::string UF_API absolute( const uf::stl::string& );
		uf::stl::string UF_API filename( const uf::stl::string& );
		uf::stl::string UF_API basename( const uf::stl::string& );
		uf::stl::string UF_API extension( const uf::stl::string& );
		uf::stl::string UF_API extension( const uf::stl::string&, int32_t );
		uf::stl::string UF_API directory( const uf::stl::string& );
		uf::stl::string UF_API sanitize( const uf::stl::string&, const uf::stl::string& = "" );
		size_t UF_API size( const uf::stl::string& );
		
		uf::stl::string UF_API readAsString( const uf::stl::string&, const uf::stl::string& = "" );
		uf::stl::vector<uint8_t> UF_API readAsBuffer( const uf::stl::string&, const uf::stl::string& = "" );
		uf::stl::vector<uint8_t> UF_API readAsBuffer( const uf::stl::string&, size_t start, size_t len, const uf::stl::string& = "" );
		uf::stl::vector<uint8_t> UF_API readAsBuffer( const uf::stl::string&, const uf::stl::vector<pod::Range>& ranges, const uf::stl::string& = "" );

		size_t UF_API write( const uf::stl::string& filename, const void*, size_t = SIZE_MAX );
		template<typename T> inline size_t write( const uf::stl::string& filename, const uf::stl::vector<T>& buffer, size_t size = SIZE_MAX ) {
			return write( filename, buffer.data(), std::min( buffer.size(), size ) );
		}
		inline size_t write( const uf::stl::string& filename, const uf::stl::string& string, size_t size = SIZE_MAX ) {
			return write( filename, string.c_str(), std::min( string.size(), size ) );
		}
		
		uf::stl::vector<uint8_t> UF_API decompress( const uf::stl::string& );
		uf::stl::vector<uint8_t> UF_API decompress( const uf::stl::string&, size_t, size_t );
		uf::stl::vector<uint8_t> UF_API decompress( const uf::stl::string&, const uf::stl::vector<pod::Range>& );

		size_t UF_API compress( const uf::stl::string&, const void*, size_t = SIZE_MAX );
		template<typename T> inline size_t compress( const uf::stl::string& filename, const uf::stl::vector<T>& buffer, size_t size = SIZE_MAX ) {
			return compress( filename, buffer.data(), std::min( buffer.size(), size ) );
		}
		inline size_t compress( const uf::stl::string& filename, const uf::stl::string& string, size_t size = SIZE_MAX ) {
			return compress( filename, string.c_str(), std::min( string.size(), size ) );
		}

		uf::stl::string UF_API hash( const uf::stl::string& );
		bool UF_API exists( const uf::stl::string& );
		size_t UF_API mtime( const uf::stl::string& );
		bool UF_API mkdir( const uf::stl::string& );
		uf::stl::string UF_API resolveURI( const uf::stl::string&, const uf::stl::string& = "" );
	}
}