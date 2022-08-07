#include <uf/spec/terminal/terminal.h>

#if UF_ENV_UNKNOWN

void spec::Terminal::clear() {
	spec::uni::Terminal::clear();
}
void spec::Terminal::setLocale() {
	spec::uni::Terminal::setLocale();
}

#endif