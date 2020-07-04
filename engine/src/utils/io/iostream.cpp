#define _X_OPEN_SOURCE_EXTENDED

#include <uf/utils/io/iostream.h>
#include <uf/ext/ncurses/ncurses.h>
#include <ncursesw/ncurses.h>
#include <uf/spec/terminal/terminal.h>

#include <sstream>
#include <iostream>

#include <vector>

namespace {
	struct {
		int character;
		std::string temporary;
		struct {
			std::string buffer;
			std::vector<std::string> history;
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
	void addStr( const std::string& str ) {
		for ( auto x : str ) addCh(x);
	}
	void addUStr( const uf::String& str ) {
		for ( auto x : str.getString() ) addCh(x);
	}
};
#ifndef UF_USE_NCURSES
	bool uf::IoStream::ncurses = false;
#else
	bool uf::IoStream::ncurses = true;
#endif
uf::IoStream uf::iostream;

UF_API_CALL uf::IoStream::IoStream() {
	spec::terminal.setLocale();
}
uf::IoStream::~IoStream() {
}

void UF_API_CALL uf::IoStream::initialize() {
	if ( !uf::IoStream::ncurses ) return;
	ext::ncurses.initialize();
	if ( ext::ncurses.hasColors() ) {
		ext::ncurses.startColor();

		this->m_registeredColors = {
			{"Red", 	{1, COLOR_RED,		COLOR_BLACK, "COLOR_RED"		} },
			{"Green", 	{2, COLOR_GREEN,	COLOR_BLACK, "COLOR_GREEN"		} },
			{"Yellow", 	{3, COLOR_YELLOW,	COLOR_BLACK, "COLOR_YELLOW"		} },
			{"Blue", 	{4, COLOR_BLUE,		COLOR_BLACK, "COLOR_BLUE"		} },
			{"Cyan", 	{5, COLOR_CYAN,		COLOR_BLACK, "COLOR_CYAN"		} },
			{"Magenta", {6, COLOR_MAGENTA,	COLOR_BLACK, "COLOR_MAGENTA"	} },
			{"White", 	{7, COLOR_WHITE,	COLOR_BLACK, "COLOR_WHITE"		} },
		};
		for ( auto& pair : this->m_registeredColors ) {
			auto& color = pair.second;
			ext::ncurses.initPair( color.id, color.foreground, color.background );
		}
		this->setColor("White");
	} else {
		uf::iostream << "Color not supported!" << "\n";
		this->readChar();
		this->terminate();
	}
}
void UF_API_CALL uf::IoStream::terminate() {
	if (uf::IoStream::ncurses) ext::ncurses.terminate();
}
void UF_API_CALL uf::IoStream::clear(bool all) {
	if ( !ext::ncurses.initialized() ) this->initialize();
	if ( !uf::IoStream::ncurses ) {
		if ( all ) {
			spec::terminal.clear();
		}
	}
	if ( all ) {
		ext::ncurses.move(0,0);
		ext::ncurses.clear(true);
		return;
	}
	
	ext::ncurses.getYX(::info.cursor.row, ::info.cursor.column);
	ext::ncurses.move(::info.cursor.row, 0);
	ext::ncurses.clear();
}
std::string uf::IoStream::getBuffer() {
	return ::info.output.buffer;
}
std::vector<std::string> uf::IoStream::getHistory() {
	return ::info.output.history;
}
void UF_API_CALL uf::IoStream::back() {
	if ( !ext::ncurses.initialized() ) return;
	if ( !uf::IoStream::ncurses ) return;
/*
	ext::ncurses.getYX(::info.cursor.row, ::info.cursor.column);
	struct {
		int row = ::info.cursor.row, column = ::info.cursor.column;
	} target;
	if ( ::info.output.buffer.size() > 0 ) {
		::info.output.buffer = ::info.output.buffer.substr( 0, ::info.output.buffer.size() );
	} else if ( ::info.output.history.size() > 0 ) {
		::info.output.buffer = ::info.output.history.back();
		::info.output.history.erase( ::info.output.history.end() - 1 );
		ext::ncurses.move(::info.cursor.row, ::info.output.buffer.size(), ::info.cursor.row, ::info.cursor.column);
	}
*/
	if ( !::info.output.buffer.empty() ) {
		::info.output.buffer.pop_back();
		ext::ncurses.delChar();
	} else if ( !::info.output.history.empty() ) {
		::info.output.buffer = ::info.output.history.back();
		::info.output.history.pop_back();
		ext::ncurses.getYX(::info.cursor.row, ::info.cursor.column);
		ext::ncurses.move(::info.cursor.row - 1, ::info.output.buffer.size(), ::info.cursor.row, ::info.cursor.column);
	}
	ext::ncurses.refresh();
}
char UF_API_CALL uf::IoStream::readChar(const bool& loop) {
	if ( !ext::ncurses.initialized() ) this->initialize();
	if ( !uf::IoStream::ncurses ) {
		return std::cin.get();
	}
	while ( loop ) {
		::info.character = ext::ncurses.getCh();
		if ( ::info.character == ERR ) continue;
		if ( ::info.character > 0 && ::info.character < 128 ) return ::info.character;
	}
	return 0;
}
std::string UF_API_CALL uf::IoStream::readString(const bool& loop) {
	if ( !ext::ncurses.initialized() ) this->initialize();
	// static std::vector<std::string> history;

	if ( !uf::IoStream::ncurses ) {
		std::string in;
		std::getline(std::cin, in);
		return in;
	}
	/*std::string ::info.input.buffer;
	int ch;
	struct {
		int r = 0, c = 0;
		int r_max = 0, c_max = 0;
	} home;
	unsigned int cursor = 0;
	
	std::string ::info.temporary;
	int ::info.indices.history = -1;*/
	ext::ncurses.getYX(::info.cursor.row, ::info.cursor.column);
	ext::ncurses.getMaxYX(::info.window.rows, ::info.window.columns);
	::info.origin = ::info.cursor;

	// break on new line or carriage return
	while ( loop && (::info.character = ext::ncurses.getCh()) != '\n' && ::info.character != '\r' ) {
		// err
		if ( ::info.character == ERR ) {
			continue;
		// mouse
		} else if ( ::info.character == KEY_MOUSE ) {
		/*
			MEVENT event;
			if ( wgetmouse(&event) == OK ) {

			}
		*/
		// backspace
		} else if ( ::info.character == '\b' && ::info.input.buffer.length() > 0 ) {
			::info.input.buffer = ::info.input.buffer.substr( 0, ::info.input.buffer.length() - 1 );
			ext::ncurses.getYX(::info.cursor.row, ::info.cursor.column);
			ext::ncurses.move(::info.cursor.row, ::info.cursor.column-1, ::info.cursor.row, ::info.cursor.column);
			ext::ncurses.delChar();
			ext::ncurses.refresh();
			--::info.indices.buffer;
		// left/right arrow keys
		} else if ( ::info.character == KEY_LEFT || ::info.character == KEY_RIGHT ) {
			int dir = (::info.character == KEY_LEFT) ? -1 : 1;
			ext::ncurses.getYX(::info.cursor.row, ::info.cursor.column);
			if ( ::info.cursor.column < ::info.window.columns && ::info.cursor.column > ::info.origin.column ) {
				::info.cursor.column += dir;
				::info.indices.buffer += dir;
				ext::ncurses.move(-1, ::info.cursor.column);
			}
		// up/down arrow keys
		} else if ( ::info.character == KEY_UP || ::info.character == KEY_DOWN ) {
			if ( ::info.temporary == "" ) ::info.temporary = ::info.input.buffer;
			int dir = (::info.character == KEY_UP) ? -1 : 1;
			if ( ::info.indices.history - 1 >= 0 && ::info.indices.history + 1 < ::info.input.history.size() ) {
				ext::ncurses.move(::info.origin.row, ::info.origin.column, ::info.cursor.row, ::info.cursor.column);
				ext::ncurses.clear();
				::info.indices.history += dir;
				::info.input.buffer = *(::info.input.history.end() - ::info.indices.history - 1);
				::info.indices.buffer = ::info.input.buffer.size();
				uf::iostream << ::info.input.buffer;
			}
		/*	else if ( delta < 0 ) {
				ext::ncurses.move(home.r, home.c);
				ext::ncurses.clear();
				::info.input.buffer = ::info.temporary;
				::info.indices.buffer = ::info.input.buffer.size();
				uf::iostream << ::info.input.buffer;
			}
		*/
		// valid characters
		} else if ( ::info.character >= 32 && ::info.character <= 127 ) {
			char at = ::info.character;
			::info.temporary = "";
			if ( ::info.indices.buffer == ::info.input.buffer.length() ) {	
				::info.input.buffer += this->writeChar(at);
			} else {
				::info.input.buffer[::info.indices.buffer] = this->writeChar(at);
			}
			++::info.indices.buffer;
		}
	}
	uf::iostream << "\n";
	::info.input.history.push_back(::info.input.buffer);
	return ::info.input.buffer;
}
uf::String UF_API_CALL uf::IoStream::readUString(const bool& loop) {
	if ( !ext::ncurses.initialized() ) this->initialize();
	// static std::vector<std::string> history;

	if ( !uf::IoStream::ncurses ) {
		std::string in;
		std::getline(std::cin, in);
		return in;
	}
	/*std::string ::info.input.buffer;
	int ch;
	struct {
		int r = 0, c = 0;
		int r_max = 0, c_max = 0;
	} home;
	unsigned int cursor = 0;
	
	std::string ::info.temporary;
	int ::info.indices.history = -1;*/
	ext::ncurses.getYX(::info.cursor.row, ::info.cursor.column);
	ext::ncurses.getMaxYX(::info.window.rows, ::info.window.columns);
	::info.origin = ::info.cursor;

	// break on new line or carriage return
	while ( loop && (::info.character = ext::ncurses.getCh()) != '\n' && ::info.character != '\r' ) {
		// err
		if ( ::info.character == ERR ) {
			continue;
		// mouse
		} else if ( ::info.character == KEY_MOUSE ) {
		/*
			MEVENT event;
			if ( wgetmouse(&event) == OK ) {

			}
		*/
		// backspace
		} else if ( ::info.character == '\b' && ::info.input.buffer.length() > 0 ) {
			::info.input.buffer = ::info.input.buffer.substr( 0, ::info.input.buffer.length() - 1 );
			ext::ncurses.getYX(::info.cursor.row, ::info.cursor.column);
			ext::ncurses.move(::info.cursor.row, ::info.cursor.column-1, ::info.cursor.row, ::info.cursor.column);
			ext::ncurses.delChar();
			ext::ncurses.refresh();
			--::info.indices.buffer;
		// left/right arrow keys
		} else if ( ::info.character == KEY_LEFT || ::info.character == KEY_RIGHT ) {
			int dir = (::info.character == KEY_LEFT) ? -1 : 1;
			ext::ncurses.getYX(::info.cursor.row, ::info.cursor.column);
			if ( ::info.cursor.column < ::info.window.columns && ::info.cursor.column > ::info.origin.column ) {
				::info.cursor.column += dir;
				::info.indices.buffer += dir;
				ext::ncurses.move(-1, ::info.cursor.column);
			}
		// up/down arrow keys
		} else if ( ::info.character == KEY_UP || ::info.character == KEY_DOWN ) {
			if ( ::info.temporary == "" ) ::info.temporary = ::info.input.buffer;
			int dir = (::info.character == KEY_UP) ? -1 : 1;
			if ( ::info.indices.history - 1 >= 0 && ::info.indices.history + 1 < ::info.input.history.size() ) {
				ext::ncurses.move(::info.origin.row, ::info.origin.column, ::info.cursor.row, ::info.cursor.column);
				ext::ncurses.clear();
				::info.indices.history += dir;
				::info.input.buffer = *(::info.input.history.end() - ::info.indices.history - 1);
				::info.indices.buffer = ::info.input.buffer.size();
				uf::iostream << ::info.input.buffer;
			}
		/*	else if ( delta < 0 ) {
				ext::ncurses.move(home.r, home.c);
				ext::ncurses.clear();
				::info.input.buffer = ::info.temporary;
				::info.indices.buffer = ::info.input.buffer.size();
				uf::iostream << ::info.input.buffer;
			}
		*/
		// valid characters
		// UTF-8 char
		} else {
			int iterations = 0;
			char at = ::info.character;
			if (  32 <= ::info.character && ::info.character <= 127 ) iterations = 1;
			if ( 194 <= ::info.character && ::info.character <= 223 ) iterations = 2;
			if ( 224 <= ::info.character && ::info.character <= 239 ) iterations = 3;
			if ( 240 <= ::info.character && ::info.character <= 244 ) iterations = 4;
			for ( int i = 0; i < iterations; ++i, ++::info.indices.buffer ) {
				if ( at != ::info.character ) at = ext::ncurses.getCh(); 
				if ( ::info.indices.buffer == ::info.input.buffer.length() ) ::info.input.buffer += this->writeChar(at);
				else ::info.input.buffer[::info.indices.buffer] = this->writeChar(at);
				at = 0;
			}
		}
	/*
		// ANSI / 1-byte UTF-8
		} else if ( 32 <= ch && ch <= 127 ) {
			char at = ch;
			if ( cursor == ::info.input.buffer.length() ) {	
				::info.input.buffer += this->writeChar(at);
			} else {
				::info.input.buffer[cursor] = this->writeChar(at);
			}
			++cursor;
		// 2-byte UTF-8
		} else if ( 194 <= ch && ch <= 223 ) {
			char at = ch;
			for ( int i = 0; i < 2; ++i, ++cursor ) {
				if ( at != ch ) at = ext::ncurses.getCh(); 
				if ( cursor == ::info.input.buffer.length() ) ::info.input.buffer += this->writeChar(at);
				else ::info.input.buffer[cursor] = this->writeChar(at);
				at = 0;
			}
		}
	*/
	}
	uf::iostream << "\n";
	::info.input.history.push_back(::info.input.buffer);
	return ::info.input.buffer;
}
char UF_API_CALL uf::IoStream::writeChar( char ch ) {
	if ( !ext::ncurses.initialized() ) this->initialize();
	addCh(ch);
/*
	if ( ch == '\r' ) ch = '\n';
	if ( ch != '\n' ) {
		::info.output.buffer += ch;
	} else {
		this->writeString("new line: " + ::info.output.buffer + "\n");
		::info.output.history.push_back(::info.output.buffer);
		::info.output.buffer = "";
	}
*/
	if ( !uf::IoStream::ncurses ) {
		if ( ch == '\n' ) std::cout << std::endl;
		else std::cout << ch;
		return ch;
	}
	ext::ncurses.addChar(ch);
	ext::ncurses.refresh();
	return ch;
}
const std::string& UF_API_CALL uf::IoStream::writeString( const std::string& str ) {
	if ( !ext::ncurses.initialized() ) this->initialize();
	addStr(str);
/*
	std::size_t needle;
	std::string haystack = str;
	while ( (needle = haystack.find('\n')) != std::string::npos ) {
		::info.output.buffer += haystack.substr( 0, needle );
		::info.output.history.push_back(::info.output.buffer);
		::info.output.buffer = "";
		haystack = haystack.substr( needle + 1 );
	}
	::info.output.buffer += haystack;
*/
	if ( !uf::IoStream::ncurses ) {
		if ( str == "\n" ) std::cout << std::endl;
		else std::cout << str;
		return str;
	}
	ext::ncurses.addStr(str.c_str());
	ext::ncurses.refresh();
	return str;
}
const uf::String& UF_API_CALL uf::IoStream::writeUString( const uf::String& str ) {
	if ( !ext::ncurses.initialized() ) this->initialize();
	addUStr(str);
/*
	std::size_t needle;
	auto haystack = str.getString();
	while ( (needle = haystack.find('\n')) != std::string::npos ) {
		::info.output.buffer += std::string((const char*) haystack.substr( 0, needle ).c_str());
		::info.output.history.push_back(::info.output.buffer);
		::info.output.buffer = "";
		haystack = haystack.substr( needle + 1 );
	}
*/
	if ( !uf::IoStream::ncurses ) {
		std::cout << (const char*) str.getString().c_str();
		return str;
	}
	ext::ncurses.addStr((const char*) str.getString().c_str());
	ext::ncurses.refresh();
	return str;
}

void UF_API_CALL uf::IoStream::operator>> (bool& val) {
	std::stringstream ss;
	ss << this->readString();
	ss >> val;
}

void UF_API_CALL uf::IoStream::operator>> (char& val) {
	std::stringstream ss;
	ss << this->readString();
	ss >> val;
}
void UF_API_CALL uf::IoStream::operator>> (short& val) {
	std::stringstream ss;
	ss << this->readString();
	ss >> val;
}
void UF_API_CALL uf::IoStream::operator>> (unsigned short& val) {
	std::stringstream ss;
	ss << this->readString();
	ss >> val;
}
void UF_API_CALL uf::IoStream::operator>> (int& val) {
	std::stringstream ss;
	ss << this->readString();
	ss >> val;
}
void UF_API_CALL uf::IoStream::operator>> (unsigned int& val) {
	std::stringstream ss;
	ss << this->readString();
	ss >> val;
}
void UF_API_CALL uf::IoStream::operator>> (long& val) {
	std::stringstream ss;
	ss << this->readString();
	ss >> val;
}
void UF_API_CALL uf::IoStream::operator>> (unsigned long& val) {
	std::stringstream ss;
	ss << this->readString();
	ss >> val;
}
void UF_API_CALL uf::IoStream::operator>> (long long& val) {
	std::stringstream ss;
	ss << this->readString();
	ss >> val;
}
void UF_API_CALL uf::IoStream::operator>> (unsigned long long& val) {
	std::stringstream ss;
	ss << this->readString();
	ss >> val;
}
void UF_API_CALL uf::IoStream::operator>> (float& val) {
	std::stringstream ss;
	ss << this->readString();
	ss >> val;
}
void UF_API_CALL uf::IoStream::operator>> (double& val) {
	std::stringstream ss;
	ss << this->readString();
	ss >> val;
}
void UF_API_CALL uf::IoStream::operator>> (long double& val) {
	std::stringstream ss;
	ss << this->readString();
	ss >> val;
}
void UF_API_CALL uf::IoStream::operator>> (std::string& val) {
	val = this->readString();
}
void UF_API_CALL uf::IoStream::operator>> (uf::String& val) {
	val = this->readUString();
}
std::istream& UF_API_CALL uf::IoStream::operator>> ( std::istream& is ) {
	std::string input;
	is >> input;
	this->writeString(input);
	return is;
}
std::istream& UF_API_CALL operator>> ( std::istream& is, uf::IoStream& io ) {
	std::string input;
	is >> input;
	io.writeString(input);
	return is;
}



uf::IoStream& UF_API_CALL uf::IoStream::operator<< (const bool& val) {
	std::stringstream ss;
	ss << val;
	this->writeString(ss.str());
	return *this;
}
uf::IoStream& UF_API_CALL uf::IoStream::operator<< (const char& val) {
	std::stringstream ss;
	ss << val;
	this->writeString(ss.str());
	return *this;
}
uf::IoStream& UF_API_CALL uf::IoStream::operator<< (const short& val) {
	std::stringstream ss;
	ss << val;
	this->writeString(ss.str());
	return *this;
}
uf::IoStream& UF_API_CALL uf::IoStream::operator<< (const unsigned short& val) {
	std::stringstream ss;
	ss << val;
	this->writeString(ss.str());
	return *this;
}
uf::IoStream& UF_API_CALL uf::IoStream::operator<< (const int& val) {
	std::stringstream ss;
	ss << val;
	this->writeString(ss.str());
	return *this;
}
uf::IoStream& UF_API_CALL uf::IoStream::operator<< (const unsigned int& val) {
	std::stringstream ss;
	ss << val;
	this->writeString(ss.str());
	return *this;
}
uf::IoStream& UF_API_CALL uf::IoStream::operator<< (const long& val) {
	std::stringstream ss;
	ss << val;
	this->writeString(ss.str());
	return *this;
}
uf::IoStream& UF_API_CALL uf::IoStream::operator<< (const unsigned long& val) {
	std::stringstream ss;
	ss << val;
	this->writeString(ss.str());
	return *this;
}
uf::IoStream& UF_API_CALL uf::IoStream::operator<< (const long long& val) {
	std::stringstream ss;
	ss << val;
	this->writeString(ss.str());
	return *this;
}
uf::IoStream& UF_API_CALL uf::IoStream::operator<< (const unsigned long long& val) {
	std::stringstream ss;
	ss << val;
	this->writeString(ss.str());
	return *this;
}
uf::IoStream& UF_API_CALL uf::IoStream::operator<< (const float& val) {
	std::stringstream ss;
	ss << val;
	this->writeString(ss.str());
	return *this;
}
uf::IoStream& UF_API_CALL uf::IoStream::operator<< (const double& val) {
	std::stringstream ss;
	ss << val;
	this->writeString(ss.str());
	return *this;
}
uf::IoStream& UF_API_CALL uf::IoStream::operator<< (const long double& val) {
	std::stringstream ss;
	ss << val;
	this->writeString(ss.str());
	return *this;
}
uf::IoStream& UF_API_CALL uf::IoStream::operator<< (const std::string& val) {
	this->writeString(val);
	return *this;
}
uf::IoStream& UF_API_CALL uf::IoStream::operator<< (const uf::String& val) {
	this->writeUString(val);
	return *this;
}
uf::IoStream& UF_API_CALL uf::IoStream::operator<< (const char* cstr) {
	this->writeString(std::string(cstr));
	return *this;
}
uf::IoStream& UF_API_CALL uf::IoStream::operator<< (void* ptr) {
	std::stringstream ss;
	ss << ptr;
	this->writeString(ss.str());
	return *this;
}
uf::IoStream& UF_API_CALL uf::IoStream::operator<< ( std::ostream& os ) {
	std::stringstream str;
	str << os.rdbuf();
	this->writeString( str.str() );
	return *this;
}
std::ostream& UF_API_CALL operator<< ( std::ostream& os, uf::IoStream& io ) {
	std::string output = io.readString();
	os << output;
	return os;
}

std::string uf::IoStream::getColor() {
	return this->m_currentColor;
}
void UF_API_CALL uf::IoStream::setColor( const std::string& str ) {
	if ( !uf::IoStream::ncurses ) return;
	if ( !ext::ncurses.initialized() ) this->initialize();

	if ( this->m_registeredColors.count(str) < 1 ) return;

	if ( ::info.color.last != 0 ) ext::ncurses.attrOff(COLOR_PAIR(::info.color.last));
	short id = this->m_registeredColors.at(str).id;
	ext::ncurses.attrOn(COLOR_PAIR(id));
	::info.color.last = id;

	this->m_currentColor = str;
}

// manip via stream manipulator
UF_API_CALL uf::IoStream::Color::Manip1::Manip1( uf::IoStream& _io ) : io(_io) {}
uf::IoStream::Color::Manip1 UF_API_CALL uf::IoStream::operator<< ( const uf::IoStream::Color& color) {
	return uf::IoStream::Color::Manip1(*this);
}
uf::IoStream& UF_API_CALL uf::IoStream::Color::Manip1::operator<<( const std::string& name ) {
	this->io.setColor(name);
	return this->io;
}

// manip via function call
UF_API_CALL uf::IoStream::Color::Manip2::Manip2( const std::string& _name ) : name(_name) {}
uf::IoStream::Color::Manip2 UF_API_CALL uf::IoStream::Color::operator()( const std::string& name ) const {
	return uf::IoStream::Color::Manip2(name);
}
uf::IoStream& UF_API_CALL uf::IoStream::Color::operator()( uf::IoStream& io, const std::string& name ) const {
	io.setColor(name);
	return io;
}

uf::IoStream& UF_API_CALL uf::IoStream::operator<< (const uf::IoStream::Color::Manip2& color) {
	this->setColor(color.name);
	return *this;
}