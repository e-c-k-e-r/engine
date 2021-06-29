#pragma once

#include <uf/config.h>

#include <algorithm>
#include <locale>
#include <uf/utils/memory/string.h>
#include <cstdlib>

////
#define template_in 		template<typename in_t> static in_t UF_API_CALL
#define template_in_int 	template<typename in_t> static int_t UF_API_CALL
#define template_in_size 	template<typename in_t> static std::size_t UF_API_CALL
#define template_in_out 	template<typename in_t, typename out_t> static out_t UF_API_CALL
#define template_out 		template<typename out_t> static out_t UF_API_CALL
#define template_out_in 	template<typename out_t, typename in_t> static out_t UF_API_CALL
///
#define declarations \
	template_in decode(in_t begin, in_t end, uint32_t& output, uint32_t replacement = 0); \
	template_out encode(uint32_t input, out_t output, int_t replacement = 0); \
	template_in next(in_t begin, in_t end); \
	template_in_size count(in_t begin, in_t end); \
	template_in_out fromAnsi(in_t begin, in_t end, out_t output, const std::locale& locale = std::locale()); \
	template_in_out fromWide(in_t begin, in_t end, out_t output); \
	template_in_out fromLatin1(in_t begin, in_t end, out_t output); \
	template_in_out toAnsi(in_t begin, in_t end, out_t output, char replacement = 0, const std::locale& locale = std::locale()); \
	template_in_out toWide(in_t begin, in_t end, out_t output, wchar_t replacement = 0); \
	template_in_out toLatin1(in_t begin, in_t end, out_t output, char replacement = 0); \
	template_in_out toUtf8(in_t begin, in_t end, out_t output); \
	template_in_out toUtf16(in_t begin, in_t end, out_t output); \
	template_in_out toUtf32(in_t begin, in_t end, out_t output); \
	template_in_int decodeAnsi(in_t input, const std::locale& locale = std::locale()); \
	template_in_int decodeWide(in_t input); \
	template_out encodeAnsi(uint32_t codepoint, out_t output, char replacement = 0, const std::locale& locale = std::locale()); \
	template_out encodeWide(uint32_t codepoint, out_t output, wchar_t replacement = 0);
///
#define template_declaration(utf_name, int_type) \
	class UF_API utf_name { \
	public: \
		typedef int_type int_t; \
		declarations; \
	};
////
namespace uf {
	template_declaration( Utf8,  uint8_t)
	template_declaration(Utf16, uint16_t)
	template_declaration(Utf32, uint32_t)
	class UF_API Utf {
	public:
		typedef uf::Utf8 							default_encoding_t;
		typedef uf::Utf::default_encoding_t::int_t 	default_int_t;
		typedef uint32_t 							max_default_int_t;
	};
	
#undef template_in
#undef template_in_int
#undef template_in_size
#undef template_in_out
#undef template_out
#undef template_out_in
#undef declarations
#undef template_declaration
}
#include "utf.inl"