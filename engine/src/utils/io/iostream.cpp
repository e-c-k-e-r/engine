#define _X_OPEN_SOURCE_EXTENDED
#include <uf/utils/io/iostream.h>

#if UF_USE_NCURSES
#include <uf/ext/ncurses/ncurses.h>
#include <ncursesw/ncurses.h>
#endif
#include <uf/spec/terminal/terminal.h>

#include <sstream>
#include <iostream>

#include <uf/utils/memory/vector.h>

namespace {
	struct {
		int character;
		uf::stl::string temporary;
		struct {
			uf::stl::string buffer;
			uf::stl::vector<uf::stl::string> history;
		} input, output;
		struct {
			unsigned int buffer = 0;
			unsigned int history = -1;
		} indices;
		struct {
			int rows = 0;
			int columns = 0;
		} window;
		struct {
			int row = 0;
			int column = 0;
		} cursor, origin;
		struct {
			short last = 0;
		} color;
	} info;
	void addCh( char ch ) {
		if ( ch == '\r' ) ch = '\n';
		if ( ch == '\n' ) {
			::info.output.history.push_back(::info.output.buffer);
			::info.output.buffer = "";
			return;
		} else if ( ch < 32 || ch > 127 ) return;
		::info.output.buffer += ch;
	}
	void addStr( const uf::stl::string& str ) {
		for ( auto x : str ) addCh(x);
	}
	void addUStr( const uf::String& str ) {
		for ( auto x : str.getString() ) addCh(x);
	}
};
bool uf::IoStream::ncurses = false;
uf::IoStream uf::iostream;

UF_API_CALL uf::IoStream::IoStream() {
	spec::terminal.setLocale();
}
uf::IoStream::~IoStream() {
}

void UF_API_CALL uf::IoStream::initialize() {
}
void UF_API_CALL uf::IoStream::terminate() {
}
void UF_API_CALL uf::IoStream::clear(bool all) {
	spec::terminal.clear();
}
uf::stl::string uf::IoStream::getBuffer() {
	return ::info.output.buffer;
}
uf::stl::vector<uf::stl::string> uf::IoStream::getHistory() {
	return ::info.output.history;
}
void UF_API_CALL uf::IoStream::back() {
}
char UF_API_CALL uf::IoStream::readChar(const bool& loop) {
	auto ch = std::cin.get();
	
	addCh(ch);
//	::info.input.history.push_back( std::to_string(ch) );
	return ch;
}
uf::stl::string UF_API_CALL uf::IoStream::readString(const bool& loop) {
	uf::stl::string str;
	std::getline(std::cin, str);

	addStr(str);
//	::info.input.history.push_back( str );
	return str;
}
uf::String UF_API_CALL uf::IoStream::readUString(const bool& loop) {
	uf::stl::string str;
	std::getline(std::cin, str);

	addUStr(str);
//	::info.input.history.push_back( str );
	return str;
}
char UF_API_CALL uf::IoStream::writeChar( char ch ) {
	addCh(ch);

	if ( ch == '\n' ) std::cout << std::endl;
	else std::cout << ch;
//	::info.input.history.push_back( std::to_string(ch) );
	return ch;
}
const uf::stl::string& UF_API_CALL uf::IoStream::writeString( const uf::stl::string& str ) {
	addStr(str);
	
	if ( str == "\n" ) std::cout << std::endl;
	else std::cout << str;
//	::info.input.history.push_back( str );
	return str;
}
const uf::String& UF_API_CALL uf::IoStream::writeUString( const uf::String& str ) {
	addUStr(str);
	
	std::cout << (const char*) str.getString().c_str();
//	::info.input.history.push_back( str );
	return str;
}

void UF_API_CALL uf::IoStream::operator>> (bool& val) {
	uf::stl::stringstream ss;
	ss << this->readString();
	ss >> val;
}

void UF_API_CALL uf::IoStream::operator>> (char& val) {
	uf::stl::stringstream ss;
	ss << this->readString();
	ss >> val;
}
void UF_API_CALL uf::IoStream::operator>> (short& val) {
	uf::stl::stringstream ss;
	ss << this->readString();
	ss >> val;
}
void UF_API_CALL uf::IoStream::operator>> (unsigned short& val) {
	uf::stl::stringstream ss;
	ss << this->readString();
	ss >> val;
}
void UF_API_CALL uf::IoStream::operator>> (int& val) {
	uf::stl::stringstream ss;
	ss << this->readString();
	ss >> val;
}
void UF_API_CALL uf::IoStream::operator>> (unsigned int& val) {
	uf::stl::stringstream ss;
	ss << this->readString();
	ss >> val;
}
void UF_API_CALL uf::IoStream::operator>> (long& val) {
	uf::stl::stringstream ss;
	ss << this->readString();
	ss >> val;
}
void UF_API_CALL uf::IoStream::operator>> (unsigned long& val) {
	uf::stl::stringstream ss;
	ss << this->readString();
	ss >> val;
}
void UF_API_CALL uf::IoStream::operator>> (long long& val) {
	uf::stl::stringstream ss;
	ss << this->readString();
	ss >> val;
}
void UF_API_CALL uf::IoStream::operator>> (unsigned long long& val) {
	uf::stl::stringstream ss;
	ss << this->readString();
	ss >> val;
}
void UF_API_CALL uf::IoStream::operator>> (float& val) {
	uf::stl::stringstream ss;
	ss << this->readString();
	ss >> val;
}
void UF_API_CALL uf::IoStream::operator>> (double& val) {
	uf::stl::stringstream ss;
	ss << this->readString();
	ss >> val;
}
void UF_API_CALL uf::IoStream::operator>> (long double& val) {
	uf::stl::stringstream ss;
	ss << this->readString();
	ss >> val;
}
void UF_API_CALL uf::IoStream::operator>> (uf::stl::string& val) {
	val = this->readString();
}
void UF_API_CALL uf::IoStream::operator>> (uf::String& val) {
	val = this->readUString();
}
std::istream& UF_API_CALL uf::IoStream::operator>> ( std::istream& is ) {
	uf::stl::string input;
	is >> input;
	this->writeString(input);
	return is;
}
std::istream& UF_API_CALL operator>> ( std::istream& is, uf::IoStream& io ) {
	uf::stl::string input;
	is >> input;
	io.writeString(input);
	return is;
}



uf::IoStream& UF_API_CALL uf::IoStream::operator<< (const bool& val) {
	uf::stl::stringstream ss;
	ss << val;
	this->writeString(ss.str());
	return *this;
}
uf::IoStream& UF_API_CALL uf::IoStream::operator<< (const char& val) {
	uf::stl::stringstream ss;
	ss << val;
	this->writeString(ss.str());
	return *this;
}
uf::IoStream& UF_API_CALL uf::IoStream::operator<< (const short& val) {
	uf::stl::stringstream ss;
	ss << val;
	this->writeString(ss.str());
	return *this;
}
uf::IoStream& UF_API_CALL uf::IoStream::operator<< (const unsigned short& val) {
	uf::stl::stringstream ss;
	ss << val;
	this->writeString(ss.str());
	return *this;
}
uf::IoStream& UF_API_CALL uf::IoStream::operator<< (const int& val) {
	uf::stl::stringstream ss;
	ss << val;
	this->writeString(ss.str());
	return *this;
}
uf::IoStream& UF_API_CALL uf::IoStream::operator<< (const unsigned int& val) {
	uf::stl::stringstream ss;
	ss << val;
	this->writeString(ss.str());
	return *this;
}
uf::IoStream& UF_API_CALL uf::IoStream::operator<< (const long& val) {
	uf::stl::stringstream ss;
	ss << val;
	this->writeString(ss.str());
	return *this;
}
uf::IoStream& UF_API_CALL uf::IoStream::operator<< (const unsigned long& val) {
	uf::stl::stringstream ss;
	ss << val;
	this->writeString(ss.str());
	return *this;
}
uf::IoStream& UF_API_CALL uf::IoStream::operator<< (const long long& val) {
	uf::stl::stringstream ss;
	ss << val;
	this->writeString(ss.str());
	return *this;
}
uf::IoStream& UF_API_CALL uf::IoStream::operator<< (const unsigned long long& val) {
	uf::stl::stringstream ss;
	ss << val;
	this->writeString(ss.str());
	return *this;
}
uf::IoStream& UF_API_CALL uf::IoStream::operator<< (const float& val) {
	uf::stl::stringstream ss;
	ss << val;
	this->writeString(ss.str());
	return *this;
}
uf::IoStream& UF_API_CALL uf::IoStream::operator<< (const double& val) {
	uf::stl::stringstream ss;
	ss << val;
	this->writeString(ss.str());
	return *this;
}
uf::IoStream& UF_API_CALL uf::IoStream::operator<< (const long double& val) {
	uf::stl::stringstream ss;
	ss << val;
	this->writeString(ss.str());
	return *this;
}
uf::IoStream& UF_API_CALL uf::IoStream::operator<< (const uf::stl::string& val) {
	this->writeString(val);
	return *this;
}
uf::IoStream& UF_API_CALL uf::IoStream::operator<< (const uf::String& val) {
	this->writeUString(val);
	return *this;
}
uf::IoStream& UF_API_CALL uf::IoStream::operator<< (const char* cstr) {
	this->writeString(uf::stl::string(cstr));
	return *this;
}
uf::IoStream& UF_API_CALL uf::IoStream::operator<< (const uf::Serializer& val) {
	this->writeString(val.serialize());
	return *this;
}
uf::IoStream& UF_API_CALL uf::IoStream::operator<< (void* ptr) {
	uf::stl::stringstream ss;
	ss << ptr;
	this->writeString(ss.str());
	return *this;
}
uf::IoStream& UF_API_CALL uf::IoStream::operator<< ( std::ostream& os ) {
	uf::stl::stringstream str;
	str << os.rdbuf();
	this->writeString( str.str() );
	return *this;
}
std::ostream& UF_API_CALL operator<< ( std::ostream& os, uf::IoStream& io ) {
	uf::stl::string output = io.readString();
	os << output;
	return os;
}

uf::stl::string uf::IoStream::getColor() {
	return this->m_currentColor;
}
void UF_API_CALL uf::IoStream::setColor( const uf::stl::string& str ) {
}

// manip via stream manipulator
UF_API_CALL uf::IoStream::Color::Manip1::Manip1( uf::IoStream& _io ) : io(_io) {}
uf::IoStream::Color::Manip1 UF_API_CALL uf::IoStream::operator<< ( const uf::IoStream::Color& color) {
	return uf::IoStream::Color::Manip1(*this);
}
uf::IoStream& UF_API_CALL uf::IoStream::Color::Manip1::operator<<( const uf::stl::string& name ) {
	this->io.setColor(name);
	return this->io;
}

// manip via function call
UF_API_CALL uf::IoStream::Color::Manip2::Manip2( const uf::stl::string& _name ) : name(_name) {}
uf::IoStream::Color::Manip2 UF_API_CALL uf::IoStream::Color::operator()( const uf::stl::string& name ) const {
	return uf::IoStream::Color::Manip2(name);
}
uf::IoStream& UF_API_CALL uf::IoStream::Color::operator()( uf::IoStream& io, const uf::stl::string& name ) const {
	io.setColor(name);
	return io;
}

uf::IoStream& UF_API_CALL uf::IoStream::operator<< (const uf::IoStream::Color::Manip2& color) {
	this->setColor(color.name);
	return *this;
}