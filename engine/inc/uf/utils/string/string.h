#pragma once

#include <uf/config.h>

#include "encoding.h"
#include <uf/utils/memory/string.h>
#include <locale>
#include <cstring>

namespace uf {
	class UF_API String {
	public:
		typedef uf::locale::Utf8 						encoding_t;
		typedef uf::String::encoding_t::literal_t		literal_t;
		typedef std::basic_string<literal_t> 			string_t;

		typedef uf::String::string_t::iterator 			iterator_t;
		typedef uf::String::string_t::const_iterator 	const_iterator_t;
	protected:
		uf::String::string_t m_string;
	public:
	/// C-tors
	// 	Default C-tor
		UF_API_CALL String();
	// 	ANSI literals/strings
		UF_API_CALL String( char ch );
		UF_API_CALL String( const char* c_str );
		UF_API_CALL String( const std::basic_string<char>& str );

	// 	Wide literals/strings
		UF_API_CALL String( wchar_t ch );
		UF_API_CALL String( const wchar_t* c_str );
		UF_API_CALL String( const std::basic_string<wchar_t>& str );

	// 	UTF-8 literals/strings
		UF_API_CALL String( uint8_t ch );
		UF_API_CALL String( const uint8_t* c_str );
		UF_API_CALL String( const std::basic_string<uint8_t>& str );
	// 	UTF-16 literals/strings
		UF_API_CALL String( uint16_t ch );
		UF_API_CALL String( const uint16_t* c_str );
		UF_API_CALL String( const std::basic_string<uint16_t>& str );
	// 	UTF-32 literals/strings
		UF_API_CALL String( uint32_t ch );
		UF_API_CALL String( const uint32_t* c_str );
		UF_API_CALL String( const std::basic_string<uint32_t>& str );
	//	~String();

	// 	Parses bytes
		void UF_API_CALL parse( char ch );
		void UF_API_CALL parse( const char* c_str );
		void UF_API_CALL parse( const std::basic_string<char>& str );

	// 	Wide literals/strings
		void UF_API_CALL parse( wchar_t ch );
		void UF_API_CALL parse( const wchar_t* c_str );
		void UF_API_CALL parse( const std::basic_string<wchar_t>& str );

	// 	UTF-8 literals/strings
		void UF_API_CALL parse( uint8_t ch );
		void UF_API_CALL parse( const uint8_t* c_str );
		void UF_API_CALL parse( const std::basic_string<uint8_t>& str );
	// 	UTF-16 literals/strings
		void UF_API_CALL parse( uint16_t ch );
		void UF_API_CALL parse( const uint16_t* c_str );
		void UF_API_CALL parse( const std::basic_string<uint16_t>& str );
	// 	UTF-32 literals/strings
		void UF_API_CALL parse( uint32_t ch );
		void UF_API_CALL parse( const uint32_t* c_str );
		void UF_API_CALL parse( const std::basic_string<uint32_t>& str );

	// 	Converts using Encoding
		template<typename Encoding>
		std::basic_string<typename Encoding::literal_t> UF_API_CALL translate() const {
			typedef std::basic_string<typename Encoding::literal_t> string_t;
			string_t res;
			for ( auto x : this->m_string ) {
				uint32_t codepoint = Encoding::decode( &x, &x + 1 );
				uint8_t chars[4] = { 0, 0, 0, 0 };
				String::encoding_t::encode( codepoint, &chars[0] );
				res += string_t((typename Encoding::literal_t*) chars);
			}
			return res;
		}
	// 	Access internal string
		uf::String::string_t& UF_API_CALL getString();
		const uf::String::string_t& UF_API_CALL getString() const;
	// 	Access internal string as a C string
		literal_t* UF_API_CALL c_str();
		const literal_t* UF_API_CALL c_str() const;
	// 	Append
		void UF_API_CALL operator +=( char ch );
		void UF_API_CALL operator +=( wchar_t ch );
		void UF_API_CALL operator +=( uint8_t ch );
		void UF_API_CALL operator +=( uint16_t ch );
		void UF_API_CALL operator +=( uint32_t ch );
	// 	Truncate
		uf::String& UF_API_CALL operator --();
		uf::String UF_API_CALL operator --(int);
		uf::String& UF_API_CALL operator -=( std::size_t i );
	// 	Convert to standard string
		UF_API_CALL operator uf::stl::string();
		UF_API_CALL operator uf::stl::string() const;
	// 	Convert to standard wide string
		UF_API_CALL operator std::wstring();
		UF_API_CALL operator std::wstring() const;
	// 	Write to an output stream
		friend std::ostream& operator <<( std::ostream& os, const uf::String& string ) {
			auto* pointer = string.getString().c_str();
			uf::stl::string str = (const char*) pointer;
			return os << str;
		}
	};
}