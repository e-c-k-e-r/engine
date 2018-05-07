#include <uf/config.h>
#include "default.h"

// 	Converts from UTF-16 to default encoding (UTF-32)
template<typename Input>
Input UF_API_CALL uf::locale::Utf16::decode( Input begin, Input end, Encoding::Literal& output, Encoding::Literal fallback) {
	uint16_t first = *begin++;
	if ((first >= 0xD800) && (first <= 0xDBFF)) {
		if (begin < end) {
			uint32_t second = *begin++;
			output = ((second >= 0xDC00) && (second <= 0xDFFF)) ? static_cast<uint32_t>(((first - 0xD800) << 10) + (second - 0xDC00) + 0x0010000) : fallback;
		}
		else begin = end, output = fallback;
	} else output = first;

	return begin;
}

// 	Converts from default encoding (UTF-32) to UTF-16
template<typename Input, typename Output>
Output UF_API_CALL uf::locale::Utf16::encode( Input input, Output output, Literal fallback ) {
	if (input < 0xFFFF) {
		if ((input >= 0xD800) && (input <= 0xDFFF)) {
			if (fallback) 	*output++ = fallback;
			else 			*output++ = static_cast<uint16_t>(input);
		}
	} else if (input > 0x0010FFFF) {
		if (fallback) 		*output++ = fallback;
	} else {
		input -= 0x0010000;
		*output++ = static_cast<uint16_t>((input >> 10)	 	+ 0xD800);
		*output++ = static_cast<uint16_t>((input & 0x3FFUL)	+ 0xDC00);
	}
	return output;
}

// 	Go to the next character in UTF-16's sequence
template<typename Input>
Input UF_API_CALL uf::locale::Utf16::next(Input begin, Input end) {
	uint32_t codepoint;
	return Utf16::decode( begin, end, codepoint );
}

// 	Number of characters in UTF-16's sequence
template<typename Input>
Input UF_API_CALL uf::locale::Utf16::length(Input begin, Input end) {
	std::size_t length = 0;
	while ( begin < end ) begin = Utf16::next( begin, end ), ++length;
	return length;
}