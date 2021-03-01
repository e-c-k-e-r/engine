#pragma once

#include <uf/config.h>

#include <vector>
#include <string>
#include <stdint.h>

namespace uf {
	namespace base64 {
		UF_API std::string encode( const uint8_t*, std::size_t );
		UF_API std::vector<uint8_t> decode( const std::string& );

		template<typename T>
		std::string encode( const T& input ) {
			return encode( (const uint8_t*) &input[0], input.size() * sizeof( input[0] ) );
		}
		template<typename T>
		std::vector<uint8_t> decode( const T& input ) {
			return decode( (const uint8_t*) &input[0], input.size() * sizeof( input[0] ) );
		}
	}
	namespace string {
		namespace base64 {
			UF_API std::string encode( const std::string& );
			UF_API std::string decode( const std::string& );
		}
	}
}