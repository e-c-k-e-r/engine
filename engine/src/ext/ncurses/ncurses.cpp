#include <uf/ext/ncurses/ncurses.h>

ext::Ncurses ext::ncurses;

#if defined(UF_USE_NCURSES)
#include <ncursesw/ncurses.h>
#include <uf/spec/terminal/terminal.h>
#include <locale.h>

UF_API_CALL ext::Ncurses::Ncurses() : 
	m_initialized(false),
	m_window(NULL),
	m_timeout(0)
{
	if ( this->m_initializeOnCtor ) this->initialize();
}

ext::Ncurses::~Ncurses() {
	this->terminate();
}

void UF_API_CALL ext::Ncurses::initialize() {
//	setlocale(LC_ALL, "");
	spec::terminal.setLocale();
	
	this->m_initialized = true;
	this->m_window = (void*) initscr();

	keypad((WINDOW*) this->m_window, true);
	scrollok((WINDOW*) this->m_window, true);
	mousemask(ALL_MOUSE_EVENTS, NULL);
	mouseinterval(0);
	timeout(this->m_timeout);
	nonl();
	noecho();
	cbreak();
}
bool UF_API_CALL ext::Ncurses::initialized() {
	return this->m_initialized;
}

void UF_API_CALL ext::Ncurses::terminate() {
	if ( !this->m_initialized ) return;
	if ( (WINDOW*) this->m_window != stdscr ) delwin((WINDOW*) this->m_window);
	endwin();
}

void UF_API_CALL ext::Ncurses::move(int row, int column) {
	if ( !this->m_initialized ) this->initialize();
	wmove((WINDOW*) this->m_window, row, column);
}
void UF_API_CALL ext::Ncurses::move(int toRow, int toColumn, int& row, int& column) {
	if ( !this->m_initialized ) this->initialize();
	this->move(-1 ? row : toRow, -1 ? column : toColumn);
	this->getYX(row, column);
}
void UF_API_CALL ext::Ncurses::getYX(int& row, int& column) { 
	if ( !this->m_initialized ) this->initialize();
	getyx((WINDOW*) this->m_window, row, column);
}
void UF_API_CALL ext::Ncurses::getMaxYX(int& row, int& column) {
	if ( !this->m_initialized ) this->initialize();
	getmaxyx((WINDOW*) this->m_window, row, column);
}
void UF_API_CALL ext::Ncurses::refresh() {
	if ( !this->m_initialized ) this->initialize();
	wrefresh((WINDOW*) this->m_window);
}
void UF_API_CALL ext::Ncurses::clrToEol() {
	if ( !this->m_initialized ) this->initialize();
//	if ( !this->m_initialized ) return;
	wclrtoeol((WINDOW*) this->m_window);
}
void UF_API_CALL ext::Ncurses::clrToBot() {
	if ( !this->m_initialized ) this->initialize();
//	if ( !this->m_initialized ) return;
	wclrtobot((WINDOW*) this->m_window);
}
 int UF_API_CALL ext::Ncurses::getCh() {
 	if ( !this->m_initialized ) this->initialize();
	return wgetch((WINDOW*) this->m_window);
}
void UF_API_CALL ext::Ncurses::delCh() {
	if ( !this->m_initialized ) this->initialize();
//	if ( !this->m_initialized ) return;
	wdelch((WINDOW*) this->m_window);
}
void UF_API_CALL ext::Ncurses::addCh( char c ) {
	if ( !this->m_initialized ) this->initialize();
//	if ( !this->m_initialized ) return;
	waddch((WINDOW*) this->m_window, c);
}
void UF_API_CALL ext::Ncurses::addStr(const char* c_str) {
	if ( !this->m_initialized ) this->initialize();
//	if ( !this->m_initialized ) return;
	waddstr((WINDOW*) this->m_window, c_str);
}
void UF_API_CALL ext::Ncurses::addStr(const uf::stl::string str) {
	if ( !this->m_initialized ) this->initialize();
//	if ( !this->m_initialized ) return;
	waddstr((WINDOW*) this->m_window, str.c_str());
}
void UF_API_CALL ext::Ncurses::attrOn(int att) {
	if ( !this->m_initialized ) this->initialize();
//	if ( !this->m_initialized ) return;
	wattron((WINDOW*) this->m_window, att);
}
void UF_API_CALL ext::Ncurses::attrOff(int att) {
	if ( !this->m_initialized ) this->initialize();
//	if ( !this->m_initialized ) return;
	wattroff((WINDOW*) this->m_window, att);
}
bool UF_API_CALL ext::Ncurses::hasColors() {
	if ( !this->m_initialized ) this->initialize();
	return has_colors();
}
bool UF_API_CALL ext::Ncurses::startColor() {
	if ( !this->m_initialized ) this->initialize();
	return start_color();
}
void UF_API_CALL ext::Ncurses::initPair( short id, short fore, short back ) {
	if ( !this->m_initialized ) this->initialize();
	init_pair(id, fore, back);
}
// Define dummy implementation
#else

UF_API_CALL ext::Ncurses::Ncurses() {}

ext::Ncurses::~Ncurses() {}

void UF_API_CALL ext::Ncurses::initialize() {}
bool UF_API_CALL ext::Ncurses::initialized() { return false; }

void UF_API_CALL ext::Ncurses::terminate() {}

void UF_API_CALL ext::Ncurses::move(int row, int column) {}
void UF_API_CALL ext::Ncurses::move(int toRow, int toColumn, int& row, int& column) {}
void UF_API_CALL ext::Ncurses::getYX(int& row, int& column) { 
}
void UF_API_CALL ext::Ncurses::getMaxYX(int& row, int& column) {}
void UF_API_CALL ext::Ncurses::refresh() {}
void UF_API_CALL ext::Ncurses::clrToEol() {}
void UF_API_CALL ext::Ncurses::clrToBot() {}
 int UF_API_CALL ext::Ncurses::getCh() { return 0; }
void UF_API_CALL ext::Ncurses::delCh() {}
void UF_API_CALL ext::Ncurses::addCh( char c ) {}
void UF_API_CALL ext::Ncurses::addStr(const char* c_str) {}
void UF_API_CALL ext::Ncurses::addStr(const uf::stl::string str) {}
void UF_API_CALL ext::Ncurses::attrOn(int att) {}
void UF_API_CALL ext::Ncurses::attrOff(int att) {}
bool UF_API_CALL ext::Ncurses::hasColors() { return false; }
bool UF_API_CALL ext::Ncurses::startColor() { return false; }
void UF_API_CALL ext::Ncurses::initPair( short id, short fore, short back ) {}

#endif