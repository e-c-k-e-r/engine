#pragma once

#include <uf/config.h>

#include <uf/utils/memory/vector.h>
#include <uf/utils/memory/string.h>
#include <stdint.h>

namespace uf {
	namespace base64 {
		UF_API uf::stl::string encode( const uint8_t*, std::size_t );
		UF_API uf::stl::vector<uint8_t> decode( const uf::stl::string& );

		template<typename T>
		uf::stl::string encode( const T& input ) {
			return encode( (const uint8_t*) &input[0], input.size() * sizeof( input[0] ) );
		}
		template<typename T>
		uf::stl::vector<uint8_t> decode( const T& input ) {
			return decode( (const uint8_t*) &input[0], input.size() * sizeof( input[0] ) );
		}
	}
	namespace string {
		namespace base64 {
			UF_API uf::stl::string encode( const uf::stl::string& );
			UF_API uf::stl::string decode( const uf::stl::string& );
		}
	}
}