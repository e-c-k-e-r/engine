#include <uf/spec/terminal/terminal.h>

#ifdef UF_ENV_DREAMCAST

void spec::Terminal::clear() {
	spec::uni::Terminal::clear();
}
void spec::Terminal::setLocale() {
	spec::uni::Terminal::setLocale();
}

void spec::Terminal::hide() {
}
void spec::Terminal::show() {
}

#endif