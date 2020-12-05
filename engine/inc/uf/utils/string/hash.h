#pragma once

#include <uf/config.h>

#include <vector>
#include <string>
#include <stdint.h>
#include <picosha2.h>

namespace uf {
	namespace string {
		template<typename T>
		std::string sha256( const T& input ) {
		//	std::vector<unsigned char> hash(picosha2::k_digest_size);
		//	picosha2::hash256(input.begin(), input.end(), hash);
		//	return picosha2::bytes_to_hex_string(hash.begin(), hash.end());
			return picosha2::hash256_hex_string(input);
		}
	}
}