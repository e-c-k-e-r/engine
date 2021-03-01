#include <uf/spec/terminal/terminal.h>

#if UF_ENV_UNKNOWN

void UF_API_CALL spec::Terminal::clear() {
	spec::uni::Terminal::clear();
}
void UF_API_CALL spec::Terminal::setLocale() {
	spec::uni::Terminal::setLocale();
}

#endif