#include <uf/spec/terminal/terminal.h>

#ifdef UF_ENV_WINDOWS

void UF_API_CALL spec::Terminal::clear() {
	spec::uni::Terminal::clear();

	COORD topLeft  = { 0, 0 };
	HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO screen;
	DWORD written;

	GetConsoleScreenBufferInfo(console, &screen);
	FillConsoleOutputCharacterA(
		console, ' ', screen.dwSize.X * screen.dwSize.Y, topLeft, &written
	);
	FillConsoleOutputAttribute(
		console, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE,
		screen.dwSize.X * screen.dwSize.Y, topLeft, &written
	);
	SetConsoleCursorPosition(console, topLeft);
}
void UF_API_CALL spec::Terminal::setLocale() {
	spec::uni::Terminal::setLocale();

	SetConsoleOutputCP(CP_UTF8);
}

void UF_API_CALL spec::Terminal::hide() {
	ShowWindow( GetConsoleWindow(), SW_HIDE);
}
void UF_API_CALL spec::Terminal::show() {
	ShowWindow( GetConsoleWindow(), SW_RESTORE);
}



#endif