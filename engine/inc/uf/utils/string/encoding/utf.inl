// 	Converts from [this encoding] to [arbitrary encoding]
template<typename Input>
Input UF_API_CALL uf::locale::Utf8::decode( Input begin, Input end, Literal& output, Literal fallback) {
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
		output = replacement;
	}

	return begin;
}

template<typename Input>
Literal UF_API_CALL uf::locale::Utf8::decode( Input begin, Input end, Literal fallback ) {
	Literal literal;
	this->decode(begin, end, literal, fallback);
	return literal;
}

// 	Converts from [arbitrary encoding] to [this encoding]
template<typename Input, typename Output>
Output UF_API_CALL uf::locale::Utf8::encode( Input input, Output output, Output fallback ) {
	static const uint8_t firstBytes[7] = {
		0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC
	};

	if ((input > 0x0010FFFF) || ((input >= 0xD800) && (input <= 0xDBFF))) {
		if (replacement) *output++ = replacement;
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

// 	Go to the next character in [this encoding]'s sequence
template<typename Input>
Input UF_API_CALL uf::locale::Utf8::next(Input begin, Input end) {
	uint32_t codepoint;
	return this->decode( begin, end, codepoint );
}

// 	Number of characters in [this encoding]'s sequence
template<typename Input>
Input UF_API_CALL uf::locale::Utf8::length(Input begin, Input end) {
	std::size_t length = 0;
	while ( begin < end ) begin = this->next( begin, end ), ++length;
	return length;
}