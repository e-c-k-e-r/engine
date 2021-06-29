#include <uf/utils/string/string.h>
#include <uf/utils/string/encoding.h>
//uf::String uf::locale::current;
uf::stl::string uf::locale::current;
UF_API_CALL uf::String::String() {
}
// 	ANSI literals/strings
UF_API_CALL uf::String::String( char ch ) {
//	this->m_string += ch;
	this->parse( ch );
}
UF_API_CALL uf::String::String( const char* c_str ) {
//	this->m_string = (const uint8_t*) c_str;
	this->parse( c_str );
}
UF_API_CALL uf::String::String( const std::basic_string<char>& str ) {
//	this->m_string = (const uint8_t*) str.c_str();
	this->parse( str );
}
// 	Wide literals/strings
UF_API_CALL uf::String::String( wchar_t ch ) {
	this->parse(ch);
}
UF_API_CALL uf::String::String( const wchar_t* c_str ) {
	this->parse(c_str);
}
UF_API_CALL uf::String::String( const std::basic_string<wchar_t>& str ) {
	this->parse(str);
}
// 	UTF-8 literals/strings
UF_API_CALL uf::String::String( uint8_t ch ) {
	this->parse(ch);
}
UF_API_CALL uf::String::String( const uint8_t* c_str ) {
	this->parse(c_str);
}
UF_API_CALL uf::String::String( const std::basic_string<uint8_t>& str ) {
	this->parse(str);
}
// 	UTF-16 literals/strings
UF_API_CALL uf::String::String( uint16_t ch ) {
	this->parse(ch);
}
UF_API_CALL uf::String::String( const uint16_t* c_str ) {
	this->parse(c_str);
}
UF_API_CALL uf::String::String( const std::basic_string<uint16_t>& str ) {
	this->parse(str);
}
// 	UTF-32 literals/strings
UF_API_CALL uf::String::String( uint32_t ch ) {
	this->parse(ch);
}
UF_API_CALL uf::String::String( const uint32_t* c_str ) {
	this->parse(c_str);
}
UF_API_CALL uf::String::String( const std::basic_string<uint32_t>& str ) {
	this->parse(str);
}

// 	Parses ANSI/ASCII
void UF_API_CALL uf::String::parse( char ch ) {
	this->m_string += ch;
/*
	uint32_t codepoint = uf::locale::Ansi::decode( ch, ch );
	uint8_t chars[4] = { 0, 0, 0, 0 };
	String::encoding_t::encode( codepoint, &chars[0] );
	this->m_string += uf::String::string_t(chars);
*/
}
void UF_API_CALL uf::String::parse( const char* c_str ) {
//	this->m_string = c_str;
	this->parse(std::basic_string<char>(c_str));
}
void UF_API_CALL uf::String::parse( const std::basic_string<char>& str ) {
//	this->m_string = str;
	for ( auto ch : str ) this->parse(ch);
}

// 	Wide literals/strings
void UF_API_CALL uf::String::parse( wchar_t ch ) {
	switch ( sizeof(wchar_t) * 8 ) {
		case 16: this->parse((uint16_t) ch); break;
		case 32: this->parse((uint32_t) ch); break;
	}
}
void UF_API_CALL uf::String::parse( const wchar_t* c_str ) {
	this->parse(std::basic_string<wchar_t>(c_str));
}
void UF_API_CALL uf::String::parse( const std::basic_string<wchar_t>& str ) {
	switch ( sizeof(wchar_t) * 8 ) {
		case 16:
			for ( auto ch : str ) this->parse((uint16_t)ch);
		break;
		case 32:
			for ( auto ch : str ) this->parse((uint32_t)ch);
		break;
	}
}

// 	UTF-8 literals/strings
void UF_API_CALL uf::String::parse( uint8_t ch ) {
//	this->m_string += ch;
	uint32_t codepoint = uf::locale::Utf8::decode( &ch, &ch + 1 );
	uint8_t chars[4] = { 0, 0, 0, 0 };
	String::encoding_t::encode( codepoint, &chars[0] );
	this->m_string += uf::String::string_t(chars);
}
void UF_API_CALL uf::String::parse( const uint8_t* c_str ) {
//	this->m_string = c_str;
	this->parse(std::basic_string<uint8_t>(c_str));
}
void UF_API_CALL uf::String::parse( const std::basic_string<uint8_t>& str ) {
//	this->m_string = str;
	for ( auto ch : str ) this->parse(ch);
}
// 	UTF-16 literals/strings
void UF_API_CALL uf::String::parse( uint16_t ch ) {
	uint32_t codepoint = uf::locale::Utf16::decode( &ch, &ch + 1 );
	uint8_t chars[4] = { 0, 0, 0, 0 };
	String::encoding_t::encode( codepoint, &chars[0] );
	this->m_string += uf::String::string_t(chars);
}
void UF_API_CALL uf::String::parse( const uint16_t* c_str ) {
	this->parse(std::basic_string<uint16_t>(c_str));
}
void UF_API_CALL uf::String::parse( const std::basic_string<uint16_t>& str ) {
	for ( auto ch : str ) this->parse(ch);
}
// 	UTF-32 literals/strings
void UF_API_CALL uf::String::parse( uint32_t ch ) {
	uint32_t codepoint = ch;
	uint8_t chars[4] = { 0, 0, 0, 0 };
	String::encoding_t::encode( codepoint, &chars[0] );
	this->m_string += uf::String::string_t(chars);
}
void UF_API_CALL uf::String::parse( const uint32_t* c_str ) {
	this->parse(std::basic_string<uint32_t>(c_str));
}
void UF_API_CALL uf::String::parse( const std::basic_string<uint32_t>& str ) {
	for ( auto ch : str ) this->parse(ch);
}

//
uf::String::string_t& UF_API_CALL uf::String::getString() {
	return this->m_string;
}
const uf::String::string_t& UF_API_CALL uf::String::getString() const {
	return this->m_string;
}
// 
uf::String::literal_t* UF_API_CALL uf::String::c_str() {
	return (uf::String::literal_t*) this->m_string.c_str();
}
const uf::String::literal_t* UF_API_CALL uf::String::c_str() const {
	return (const uf::String::literal_t*) this->m_string.c_str();
}
//
void UF_API_CALL uf::String::operator +=( char ch ) {
	this->parse(ch);
}
void UF_API_CALL uf::String::operator +=( wchar_t ch ) {
	this->parse(ch);
}
void UF_API_CALL uf::String::operator +=( uint8_t ch ) {
	this->parse(ch);
}
void UF_API_CALL uf::String::operator +=( uint16_t ch ) {
	this->parse(ch);
}
void UF_API_CALL uf::String::operator +=( uint32_t ch ) {
	this->parse(ch);
}
// Truncate
uf::String& UF_API_CALL uf::String::operator --() {
	this->m_string.pop_back();
	return *this;
}
uf::String UF_API_CALL uf::String::operator --(int) {
	auto before = this->m_string;
	this->m_string.pop_back();
	return before;
}
uf::String& UF_API_CALL uf::String::operator -=( std::size_t i ) {
	for ( std::size_t ii = 0; ii < i; ++ii ) this->m_string.pop_back();
	return *this;
}
// 
UF_API_CALL uf::String::operator uf::stl::string() {
	return uf::stl::string((char*) this->c_str());
}
UF_API_CALL uf::String::operator uf::stl::string() const {
	return uf::stl::string((const char*) this->c_str());
}
// 
UF_API_CALL uf::String::operator std::wstring() {
	switch ( sizeof(wchar_t) * 8 ) {
		case 16: {
			std::basic_string<uint16_t> w_str = this->translate<uf::locale::Utf16>();
			return std::wstring((wchar_t*) w_str.c_str());
		}
		case 32: {
			std::basic_string<uint32_t> w_str = this->translate<uf::locale::Utf32	>();
			return std::wstring((wchar_t*) w_str.c_str());
		}
	}
	std::wstring w_str;
	for ( auto c : this->m_string ) w_str += c;
	return w_str;
}
UF_API_CALL uf::String::operator std::wstring() const {
	switch ( sizeof(wchar_t) * 8 ) {
		case 16: {
			std::basic_string<uint16_t> w_str = this->translate<uf::locale::Utf16>();
			return std::wstring((wchar_t*) w_str.c_str());
		}
		case 32: {	
			std::basic_string<uint32_t> w_str = this->translate<uf::locale::Utf32>();
			return std::wstring((wchar_t*) w_str.c_str());
		}
	}
	std::wstring w_str;
	for ( auto c : this->m_string ) w_str += c;
	return w_str;
}