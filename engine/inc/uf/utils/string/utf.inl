// References:
//
// http://www.unicode.org/
// http://www.unicode.org/Public/PROGRAMS/CVTUTF/ConvertUTF.c
// http://www.unicode.org/Public/PROGRAMS/CVTUTF/ConvertUTF.h
// http://people.w3.org/rishida/scripts/uniview/conversion

#include <uf/config.h>

template <typename in_t>
in_t UF_API_CALL uf::Utf8::decode(in_t begin, in_t end, uint32_t& output, uint32_t replacement) {
	// Some useful precomputed data
	static const int trailing[256] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5
	};
	static const uint32_t offsets[6] = {
		0x00000000, 0x00003080, 0x000E2080, 0x03C82080, 0xFA082080, 0x82082080
	};

	// decode the character
	int trailingBytes = trailing[static_cast<uint8_t>(*begin)];
	if (begin + trailingBytes < end) {
		output = 0;
		switch (trailingBytes) {
			case 5: output += static_cast<uint8_t>(*begin++); output <<= 6;
			case 4: output += static_cast<uint8_t>(*begin++); output <<= 6;
			case 3: output += static_cast<uint8_t>(*begin++); output <<= 6;
			case 2: output += static_cast<uint8_t>(*begin++); output <<= 6;
			case 1: output += static_cast<uint8_t>(*begin++); output <<= 6;
			case 0: output += static_cast<uint8_t>(*begin++);
		}
		output -= offsets[trailingBytes];
	}
	else {
		// incomplete character
		begin = end;
		output = replacement;
	}

	return begin;
}


template <typename out_t>
out_t UF_API_CALL uf::Utf8::encode(uint32_t input, out_t output, uint8_t replacement) {
	// Some useful precomputed data
	static const uint8_t firstBytes[7] = {
		0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC
	};

	// encode the character
	if ((input > 0x0010FFFF) || ((input >= 0xD800) && (input <= 0xDBFF))) {
		// invalid character
		if (replacement)
			*output++ = replacement;
	}
	else {
		// Valid character

		// Get the number of bytes to write
		std::size_t bytestoWrite = 1;
		if	  	(input <  0x80)	   		bytestoWrite = 1;
		else if (input <  0x800)	  	bytestoWrite = 2;
		else if (input <  0x10000)		bytestoWrite = 3;
		else if (input <= 0x0010FFFF) 	bytestoWrite = 4;

		// Extract the bytes to write
		uint8_t bytes[4];
		switch (bytestoWrite) {
			case 4: bytes[3] = static_cast<uint8_t>((input | 0x80) & 0xBF); input >>= 6;
			case 3: bytes[2] = static_cast<uint8_t>((input | 0x80) & 0xBF); input >>= 6;
			case 2: bytes[1] = static_cast<uint8_t>((input | 0x80) & 0xBF); input >>= 6;
			case 1: bytes[0] = static_cast<uint8_t> (input | firstBytes[bytestoWrite]);
		}

		// Add them to the output
		output = std::copy(bytes, bytes + bytestoWrite, output);
	}

	return output;
}


template <typename in_t>
in_t UF_API_CALL uf::Utf8::next(in_t begin, in_t end) {
	uint32_t codepoint;
	return decode(begin, end, codepoint);
}


template <typename in_t>
std::size_t UF_API_CALL uf::Utf8::count(in_t begin, in_t end) {
	std::size_t length = 0;
	while (begin < end) {
		begin = next(begin, end);
		++length;
	}

	return length;
}


template <typename in_t, typename out_t>
out_t UF_API_CALL uf::Utf8::fromAnsi(in_t begin, in_t end, out_t output, const std::locale& locale) {
	while (begin < end) {
		uint32_t codepoint = uf::Utf32::decodeAnsi(*begin++, locale);
		output = encode(codepoint, output);
	}

	return output;
}


template <typename in_t, typename out_t>
out_t UF_API_CALL uf::Utf8::fromWide(in_t begin, in_t end, out_t output) {
	while (begin < end) {
		uint32_t codepoint = uf::Utf32::decodeWide(*begin++);
		output = encode(codepoint, output);
	}

	return output;
}


template <typename in_t, typename out_t>
out_t UF_API_CALL uf::Utf8::fromLatin1(in_t begin, in_t end, out_t output) {
	// Latin-1 is directly compatible with Unicode encodings,
	// and can thus be treated as (a sub-range of) UTF-32
	while (begin < end)
		output = encode(*begin++, output);

	return output;
}


template <typename in_t, typename out_t>
out_t UF_API_CALL uf::Utf8::toAnsi(in_t begin, in_t end, out_t output, char replacement, const std::locale& locale) {
	while (begin < end) {
		uint32_t codepoint;
		begin = decode(begin, end, codepoint);
		output = uf::Utf32::encodeAnsi(codepoint, output, replacement, locale);
	}

	return output;
}


template <typename in_t, typename out_t>
out_t UF_API_CALL uf::Utf8::toWide(in_t begin, in_t end, out_t output, wchar_t replacement) {
	while (begin < end) {
		uint32_t codepoint;
		begin = decode(begin, end, codepoint);
		output = uf::Utf32::encodeWide(codepoint, output, replacement);
	}

	return output;
}


template <typename in_t, typename out_t>
out_t UF_API_CALL uf::Utf8::toLatin1(in_t begin, in_t end, out_t output, char replacement) {
	// Latin-1 is directly compatible with Unicode encodings,
	// and can thus be treated as (a sub-range of) UTF-32
	while (begin < end) {
		uint32_t codepoint;
		begin = decode(begin, end, codepoint);
		*output++ = codepoint < 256 ? static_cast<char>(codepoint) : replacement;
	}

	return output;
}


template <typename in_t, typename out_t>
out_t UF_API_CALL uf::Utf8::toUtf8(in_t begin, in_t end, out_t output) {
	return std::copy(begin, end, output);
}


template <typename in_t, typename out_t>
out_t UF_API_CALL uf::Utf8::toUtf16(in_t begin, in_t end, out_t output) {
	while (begin < end) {
		uint32_t codepoint;
		begin = decode(begin, end, codepoint);
		output = uf::Utf16::encode(codepoint, output);
	}

	return output;
}


template <typename in_t, typename out_t>
out_t UF_API_CALL uf::Utf8::toUtf32(in_t begin, in_t end, out_t output) {
	while (begin < end) {
		uint32_t codepoint;
		begin = decode(begin, end, codepoint);
		*output++ = codepoint;
	}

	return output;
}


template <typename in_t>
in_t UF_API_CALL uf::Utf16::decode(in_t begin, in_t end, uint32_t& output, uint32_t replacement) {
	uint16_t first = *begin++;

	// If it's a surrogate pair, first convert to a single UTF-32 character
	if ((first >= 0xD800) && (first <= 0xDBFF)) {
		if (begin < end) {
			uint32_t second = *begin++;
			if ((second >= 0xDC00) && (second <= 0xDFFF)) {
				// The second element is valid: convert the two elements to a UTF-32 character
				output = static_cast<uint32_t>(((first - 0xD800) << 10) + (second - 0xDC00) + 0x0010000);
			}
			else {
				// invalid character
				output = replacement;
			}
		}
		else {
			// invalid character
			begin = end;
			output = replacement;
		}
	}
	else {
		// We can make a direct copy
		output = first;
	}

	return begin;
}


template <typename out_t>
out_t UF_API_CALL uf::Utf16::encode(uint32_t input, out_t output, uint16_t replacement) {
	if (input < 0xFFFF) {
		// The character can be copied directly, we just need to check if it's in the valid range
		if ((input >= 0xD800) && (input <= 0xDFFF)) {
			// invalid character (this range is reserved)
			if (replacement)
				*output++ = replacement;
		}
		else {
			// Valid character directly convertible to a single UTF-16 character
			*output++ = static_cast<uint16_t>(input);
		}
	}
	else if (input > 0x0010FFFF) {
		// invalid character (greater than the maximum Unicode value)
		if (replacement)
			*output++ = replacement;
	}
	else {
		// The input character will be converted to two UTF-16 elements
		input -= 0x0010000;
		*output++ = static_cast<uint16_t>((input >> 10)	 + 0xD800);
		*output++ = static_cast<uint16_t>((input & 0x3FFUL) + 0xDC00);
	}

	return output;
}


template <typename in_t>
in_t UF_API_CALL uf::Utf16::next(in_t begin, in_t end) {
	uint32_t codepoint;
	return decode(begin, end, codepoint);
}


template <typename in_t>
std::size_t UF_API_CALL uf::Utf16::count(in_t begin, in_t end) {
	std::size_t length = 0;
	while (begin < end) {
		begin = next(begin, end);
		++length;
	}

	return length;
}


template <typename in_t, typename out_t>
out_t UF_API_CALL uf::Utf16::fromAnsi(in_t begin, in_t end, out_t output, const std::locale& locale) {
	while (begin < end) {
		uint32_t codepoint = uf::Utf32::decodeAnsi(*begin++, locale);
		output = encode(codepoint, output);
	}

	return output;
}


template <typename in_t, typename out_t>
out_t UF_API_CALL uf::Utf16::fromWide(in_t begin, in_t end, out_t output) {
	while (begin < end) {
		uint32_t codepoint = uf::Utf32::decodeWide(*begin++);
		output = encode(codepoint, output);
	}

	return output;
}


template <typename in_t, typename out_t>
out_t UF_API_CALL uf::Utf16::fromLatin1(in_t begin, in_t end, out_t output) {
	// Latin-1 is directly compatible with Unicode encodings,
	// and can thus be treated as (a sub-range of) UTF-32
	return std::copy(begin, end, output);
}


template <typename in_t, typename out_t>
out_t UF_API_CALL uf::Utf16::toAnsi(in_t begin, in_t end, out_t output, char replacement, const std::locale& locale) {
	while (begin < end) {
		uint32_t codepoint;
		begin = decode(begin, end, codepoint);
		output = uf::Utf32::encodeAnsi(codepoint, output, replacement, locale);
	}

	return output;
}


template <typename in_t, typename out_t>
out_t UF_API_CALL uf::Utf16::toWide(in_t begin, in_t end, out_t output, wchar_t replacement) {
	while (begin < end) {
		uint32_t codepoint;
		begin = decode(begin, end, codepoint);
		output = uf::Utf32::encodeWide(codepoint, output, replacement);
	}

	return output;
}


template <typename in_t, typename out_t>
out_t UF_API_CALL uf::Utf16::toLatin1(in_t begin, in_t end, out_t output, char replacement) {
	// Latin-1 is directly compatible with Unicode encodings,
	// and can thus be treated as (a sub-range of) UTF-32
	while (begin < end) {
		*output++ = *begin < 256 ? static_cast<char>(*begin) : replacement;
		begin++;
	}

	return output;
}


template <typename in_t, typename out_t>
out_t UF_API_CALL uf::Utf16::toUtf8(in_t begin, in_t end, out_t output) {
	while (begin < end) {
		uint32_t codepoint;
		begin = decode(begin, end, codepoint);
		output = uf::Utf8::encode(codepoint, output);
	}

	return output;
}


template <typename in_t, typename out_t>
out_t UF_API_CALL uf::Utf16::toUtf16(in_t begin, in_t end, out_t output) {
	return std::copy(begin, end, output);
}


template <typename in_t, typename out_t>
out_t UF_API_CALL uf::Utf16::toUtf32(in_t begin, in_t end, out_t output) {
	while (begin < end) {
		uint32_t codepoint;
		begin = decode(begin, end, codepoint);
		*output++ = codepoint;
	}

	return output;
}


template <typename in_t>
in_t UF_API_CALL uf::Utf32::decode(in_t begin, in_t /*end*/, uint32_t& output, uint32_t /*replacement*/) {
	output = *begin++;
	return begin;
}


template <typename out_t>
out_t UF_API_CALL uf::Utf32::encode(uint32_t input, out_t output, uint32_t /*replacement*/) {
	*output++ = input;
	return output;
}


template <typename in_t>
in_t UF_API_CALL uf::Utf32::next(in_t begin, in_t /*end*/) {
	return ++begin;
}


template <typename in_t>
std::size_t UF_API_CALL uf::Utf32::count(in_t begin, in_t end) {
	return begin - end;
}


template <typename in_t, typename out_t>
out_t UF_API_CALL uf::Utf32::fromAnsi(in_t begin, in_t end, out_t output, const std::locale& locale) {
	while (begin < end)
		*output++ = decodeAnsi(*begin++, locale);

	return output;
}


template <typename in_t, typename out_t>
out_t UF_API_CALL uf::Utf32::fromWide(in_t begin, in_t end, out_t output) {
	while (begin < end)
		*output++ = decodeWide(*begin++);

	return output;
}


template <typename in_t, typename out_t>
out_t UF_API_CALL uf::Utf32::fromLatin1(in_t begin, in_t end, out_t output) {
	// Latin-1 is directly compatible with Unicode encodings,
	// and can thus be treated as (a sub-range of) UTF-32
	return std::copy(begin, end, output);
}


template <typename in_t, typename out_t>
out_t UF_API_CALL uf::Utf32::toAnsi(in_t begin, in_t end, out_t output, char replacement, const std::locale& locale) {
	while (begin < end)
		output = encodeAnsi(*begin++, output, replacement, locale);

	return output;
}


template <typename in_t, typename out_t>
out_t UF_API_CALL uf::Utf32::toWide(in_t begin, in_t end, out_t output, wchar_t replacement) {
	while (begin < end)
		output = encodeWide(*begin++, output, replacement);

	return output;
}


template <typename in_t, typename out_t>
out_t UF_API_CALL uf::Utf32::toLatin1(in_t begin, in_t end, out_t output, char replacement) {
	// Latin-1 is directly compatible with Unicode encodings,
	// and can thus be treated as (a sub-range of) UTF-32
	while (begin < end) {
		*output++ = *begin < 256 ? static_cast<char>(*begin) : replacement;
		begin++;
	}

	return output;
}


template <typename in_t, typename out_t>
out_t UF_API_CALL uf::Utf32::toUtf8(in_t begin, in_t end, out_t output) {
	while (begin < end)
		output = uf::Utf8::encode(*begin++, output);

	return output;
}

template <typename in_t, typename out_t>
out_t UF_API_CALL uf::Utf32::toUtf16(in_t begin, in_t end, out_t output) {
	while (begin < end)
		output = uf::Utf16::encode(*begin++, output);

	return output;
}


template <typename in_t, typename out_t>
out_t UF_API_CALL uf::Utf32::toUtf32(in_t begin, in_t end, out_t output) {
	return std::copy(begin, end, output);
}


template <typename in_t>
uint32_t UF_API_CALL uf::Utf32::decodeAnsi(in_t input, const std::locale& locale) {
	// On Windows, GCC's standard library (glibc++) has almost
	// no support for Unicode stuff. As a consequence, in this
	// context we can only use the default locale and ignore
	// the one passed as parameter.

	#if defined(UF_ENV_WINDOWS) &&					   /* if Windows ... */						  \
	   (defined(__GLIBCPP__) || defined (__GLIBCXX__)) &&	 /* ... and standard library is glibc++ ... */ \
	  !(defined(__SGI_STL_PORT) || defined(_STLPORT_VERSION)) /* ... and STLPort is not used on top of it */

		(void)locale; // to avoid warnings

		wchar_t character = 0;
		mbtowc(&character, &input, 1);
		return static_cast<uint32_t>(character);

	#else

		// Get the facet of the locale which deals with character conversion
		const std::ctype<wchar_t>& facet = std::use_facet< std::ctype<wchar_t> >(locale);

		// Use the facet to convert each character of the input string
		return static_cast<uint32_t>(facet.widen(input));

	#endif
}


template <typename in_t>
uint32_t UF_API_CALL uf::Utf32::decodeWide(in_t input) {
	// The encoding of wide characters is not well defined and is left to the system;
	// however we can safely assume that it is UCS-2 on Windows and
	// UCS-4 on Unix systems.
	// in_t both cases, a simple copy is enough (UCS-2 is a subset of UCS-4,
	// and UCS-4 *is* UTF-32).

	return input;
}


template <typename out_t>
out_t UF_API_CALL uf::Utf32::encodeAnsi(uint32_t codepoint, out_t output, char replacement, const std::locale& locale) {
	// On Windows, gcc's standard library (glibc++) has almost
	// no support for Unicode stuff. As a consequence, in this
	// context we can only use the default locale and ignore
	// the one passed as parameter.

	#if defined(UF_ENV_WINDOWS) &&					   /* if Windows ... */						  \
	   (defined(__GLIBCPP__) || defined (__GLIBCXX__)) &&	 /* ... and standard library is glibc++ ... */ \
	  !(defined(__SGI_STL_PORT) || defined(_STLPORT_VERSION)) /* ... and STLPort is not used on top of it */

		(void)locale; // to avoid warnings

		char character = 0;
		if (wctomb(&character, static_cast<wchar_t>(codepoint)) >= 0)
			*output++ = character;
		else if (replacement)
			*output++ = replacement;

		return output;

	#else

		// Get the facet of the locale which deals with character conversion
		const std::ctype<wchar_t>& facet = std::use_facet< std::ctype<wchar_t> >(locale);

		// Use the facet to convert each character of the input string
		*output++ = facet.narrow(static_cast<wchar_t>(codepoint), replacement);

		return output;

	#endif
}


template <typename out_t>
out_t UF_API_CALL uf::Utf32::encodeWide(uint32_t codepoint, out_t output, wchar_t replacement) {
	// The encoding of wide characters is not well defined and is left to the system;
	// however we can safely assume that it is UCS-2 on Windows and
	// UCS-4 on Unix systems.
	// For UCS-2 we need to check if the source characters fits in (UCS-2 is a subset of UCS-4).
	// For UCS-4 we can do a direct copy (UCS-4 *is* UTF-32).

	switch (sizeof(wchar_t)) {
		case 4: {
			*output++ = static_cast<wchar_t>(codepoint);
			break;
		}

		default: {
			if ((codepoint <= 0xFFFF) && ((codepoint < 0xD800) || (codepoint > 0xDFFF))) {
				*output++ = static_cast<wchar_t>(codepoint);
			}
			else if (replacement) {
				*output++ = replacement;
			}
			break;
		}
	}

	return output;
}