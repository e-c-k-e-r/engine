//  Converts from ANSI to default encoding (UTF-32)
template<typename Input>
Input UF_API_CALL uf::locale::Ansi::decode( Input input, Input, Encoding::Literal& output, Encoding::Literal fallback) {
	// On Windows, gcc's standard library (glibc++) has almost
	// no support for Unicode stuff. As a consequence, in this
	// context we can only use the default locale and ignore
	// the one passed as parameter.

	#if defined(UF_ENV_WINDOWS) && 								/* if Windows ... 								*/\
	   (defined(__GLIBCPP__) || defined (__GLIBCXX__)) && 		/* ... and standard library is glibc++ ... 		*/\
	  !(defined(__SGI_STL_PORT) || defined(_STLPORT_VERSION)) 	/* ... and STLPort is not used on top of it 	*/
		wchar_t character = 0;
		mbtowc(&character, &input, 1);
		return (uint32_t) character;
	#else
		// Get the facet of the locale which deals with character conversion
		const std::ctype<wchar_t>& facet = std::use_facet< std::ctype<wchar_t> >(std::locale(""));

		// Use the facet to convert each character of the input string
		return (uint32_t) facet.widen(input);
	#endif
}

//  Converts from default encoding (UTF-32) to ANSI
template<typename Input, typename Output>
Output UF_API_CALL uf::locale::Ansi::encode( Input input, Output output, Literal fallback ) {
	// On Windows, gcc's standard library (glibc++) has almost
	// no support for Unicode stuff. As a consequence, in this
	// context we can only use the default locale and ignore
	// the one passed as parameter.

	#if defined(UF_ENV_WINDOWS) && 								/* if Windows ... 								*/\
	   (defined(__GLIBCPP__) || defined (__GLIBCXX__)) && 		/* ... and standard library is glibc++ ... 		*/\
	  !(defined(__SGI_STL_PORT) || defined(_STLPORT_VERSION)) 	/* ... and STLPort is not used on top of it 	*/
		char character = 0;
		if (wctomb(&character, (wchar_t) input) >= 0) 	*output++ = character;
		else if (fallback) 								*output++ = fallback;
	#else
		// Get the facet of the locale which deals with character conversion
		const std::ctype<wchar_t>& facet = std::use_facet< std::ctype<wchar_t> >(std::locale(""));

		// Use the facet to convert each character of the input string
		*output++ = facet.narrow((wchar_t) input, fallback);
	#endif
	return output;
}

//  Go to the next character in ANSI's sequence
template<typename Input>
Input UF_API_CALL uf::locale::Ansi::next(Input begin, Input end) {
	return ++begin;
}

//  Number of characters in ANSI's sequence
template<typename Input>
Input UF_API_CALL uf::locale::Ansi::length(Input begin, Input end) {
	return begin - end;
}