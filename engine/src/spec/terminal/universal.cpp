#include <uf/spec/terminal/terminal.h>
#include <uf/utils/io/iostream.h>
#include <uf/utils/string/locale.h>

#include <locale>
#include <clocale>


spec::Terminal spec::terminal;
spec::uni::Terminal& UF_API_CALL spec::Terminal::getUniversal() {
	return (spec::uni::Terminal&) *this;
}

void UF_API_CALL spec::uni::Terminal::clear() {
}
void UF_API_CALL spec::uni::Terminal::hide() {
}
void UF_API_CALL spec::uni::Terminal::show() {
}
void UF_API_CALL spec::uni::Terminal::setLocale() {
#if UF_ENV_DREAMCAST
#else
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
			std::cout << "Attempted to set invalid locale: " << name << std::endl;
			continue;
		}
	}
#endif
}