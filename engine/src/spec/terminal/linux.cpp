#include <uf/spec/terminal/terminal.h>

#ifdef UF_ENV_LINUX

#include <iostream>
#include <locale>
#include <unistd.h>
#include <termios.h>
#include <cstdio>

void spec::Terminal::clear() {
	spec::uni::Terminal::clear();
	std::cout << "\033[2J\033[H" << std::flush;
}

void spec::Terminal::setLocale() {
	spec::uni::Terminal::setLocale();
	std::setlocale(LC_ALL, "en_US.UTF-8");
}

void spec::Terminal::hide() {
/*
	if (isatty(fileno(stdin))) {
		struct termios tty;
		if (tcgetattr(STDIN_FILENO, &tty) == 0) {
			tty.c_lflag &= ~ECHO;
			tcsetattr(STDIN_FILENO, TCSANOW, &tty);
		}
	}
*/
}

void spec::Terminal::show() {
/*
	if (isatty(fileno(stdin))) {
		struct termios tty;
		if (tcgetattr(STDIN_FILENO, &tty) == 0) {
			tty.c_lflag |= ECHO;
			tcsetattr(STDIN_FILENO, TCSANOW, &tty);
		}
	}
*/
}



#endif