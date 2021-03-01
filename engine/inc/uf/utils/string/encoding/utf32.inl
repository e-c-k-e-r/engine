#include <uf/config.h>
#include "default.h"

// 	Converts from UTF-32 to default encoding (UTF-32) (nothing happens)
template<typename Input>
Input UF_API_CALL uf::locale::Utf32::decode( Input begin, Input end, Encoding::Literal& output, Encoding::Literal fallback) {
	output = *begin++;
	return begin;
}

// 	Converts from default encoding (UTF-32) to UTF-32 (nothing happens)
template<typename Input, typename Output>
Output UF_API_CALL uf::locale::Utf32::encode( Input input, Output output, Literal fallback ) {
	*output++ = input;
	return output;
}

// 	Go to the next character in UTF-32's sequence
template<typename Input>
Input UF_API_CALL uf::locale::Utf32::next(Input begin, Input end) {
	return ++begin;
}

// 	Number of characters in UTF-32's sequence
template<typename Input>
Input UF_API_CALL uf::locale::Utf32::length(Input begin, Input end) {
	return begin - end;
}