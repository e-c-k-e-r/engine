#include <uf/spec/terminal/terminal.h>
#include <uf/utils/io/iostream.h>
#include <uf/utils/string/locale.h>

#if !UF_ENV_DREAMCAST
#include <locale>
#include <clocale>
#endif


spec::Terminal spec::terminal;
spec::uni::Terminal& spec::Terminal::getUniversal() {
	return (spec::uni::Terminal&) *this;
}

void spec::uni::Terminal::clear() {
}
void spec::uni::Terminal::hide() {
}
void spec::uni::Terminal::show() {
}
void spec::uni::Terminal::setLocale() {
#if UF_USE_DEPRECATED_STRING
	const char* locales[4] = {
		"", "C.utf8", "C", "POSIX"
	};
	for ( unsigned int i = 0; i < 4; ++i ) {	
		const char*& name = locales[i];
		try {	
			std::setlocale(LC_ALL, name);
			std::locale::global(std::locale(name));
			uf::locale::current = name;
			break;
		} catch ( ... ) {
			UF_MSG_ERROR("Attempted to set invalid locale: {}", name);
			continue;
		}
	}
#endif
}