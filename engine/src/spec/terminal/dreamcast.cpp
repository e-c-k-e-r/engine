#include <uf/spec/terminal/terminal.h>

#ifdef UF_ENV_DREAMCAST

void UF_API_CALL spec::Terminal::clear() {
	spec::uni::Terminal::clear();
}
void UF_API_CALL spec::Terminal::setLocale() {
	spec::uni::Terminal::setLocale();
}

void UF_API_CALL spec::Terminal::hide() {
}
void UF_API_CALL spec::Terminal::show() {
}

#endif