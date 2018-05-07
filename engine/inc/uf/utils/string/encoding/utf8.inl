#include <uf/config.h>
#include "default.h"

// Converts from UTF-8 to default encoding (UTF-32)
template<typename Input>
Input UF_API_CALL uf::locale::Utf8::decode( Input begin, Input end, Encoding::Literal& output, Encoding::Literal fallback) {
	static const int utf8_table[256] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5
	};
	static const uint32_t utf32_offsets[6] = {
		0x00000000, 0x00003080, 0x000E2080, 0x03C82080, 0xFA082080, 0x82082080
	};
	int length = utf8_table[static_cast<uint8_t>(*begin)];

	if ( uf::locale::Encoding::getName() == "UTF-32" ) {
		if (begin + length < end) {
			output = 0;
			switch (length) {
				case 5: output += static_cast<uint8_t>(*begin++); output <<= 6;
				case 4: output += static_cast<uint8_t>(*begin++); output <<= 6;
				case 3: output += static_cast<uint8_t>(*begin++); output <<= 6;
				case 2: output += static_cast<uint8_t>(*begin++); output <<= 6;
				case 1: output += static_cast<uint8_t>(*begin++); output <<= 6;
				case 0: output += static_cast<uint8_t>(*begin++);
			}
			output -= utf32_offsets[length];
		} else {
			begin = end;
			output = fallback;
		}
	}
	return begin;
}

// Converts from default encoding (UTF-32) to UTF-8
template<typename Input, typename Output>
Output UF_API_CALL uf::locale::Utf8::encode( Input input, Output output, Literal fallback ) {
	static const uint8_t firstBytes[7] = {
		0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC
	};

	if ((input > 0x0010FFFF) || ((input >= 0xD800) && (input <= 0xDBFF))) {
		if (fallback) *output++ = fallback;
	} else {
		uint8_t characters[4];
		std::size_t length = 1;
		if	  		(input <  0x00000080) length = 1;
		else if 	(input <  0x00000800) length = 2;
		else if 	(input <  0x00010000) length = 3;
		else if 	(input <= 0x0010FFFF) length = 4;
		switch (length) {
			case 4: characters[3] = static_cast<uint8_t>((input | 0x80) & 0xBF); 		input >>= 6;
			case 3: characters[2] = static_cast<uint8_t>((input | 0x80) & 0xBF); 		input >>= 6;
			case 2: characters[1] = static_cast<uint8_t>((input | 0x80) & 0xBF); 		input >>= 6;
			case 1: characters[0] = static_cast<uint8_t>((input | firstBytes[length]));
		}
		output = std::copy(characters, characters + length, output);
	}
	return output;
}

// Go to the next character in UTF-8's sequence
template<typename Input>
Input UF_API_CALL uf::locale::Utf8::next(Input begin, Input end) {
	uint32_t codepoint;
	return Utf8::decode( begin, end, codepoint );
}

// Number of characters in UTF-8's sequence
template<typename Input>
Input UF_API_CALL uf::locale::Utf8::length(Input begin, Input end) {
	std::size_t length = 0;
	while ( begin < end ) begin = Utf8::next( begin, end ), ++length;
	return length;
}