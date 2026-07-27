#include <uf/utils/io/fmt.h>
#include <uf/utils/io/payloads.h>
#include <uf/utils/io/iostream.h>

#include <uf/utils/hook/hook.h>
#include <uf/utils/io/console.h>
#include <uf/utils/io/fmt.h>

#include <sstream>

/*
void uf::io::exception( const uf::stl::string& exception ) {
	std::abort(-1);
}
*/
uf::stl::string uf::io::log( const uf::stl::string& category, const uf::stl::string& file, const uf::stl::string& function, size_t line, const uf::stl::string& message ) {
	auto string = FMT_FORMAT("[{}] [{}:{}@{}]: {}", category, file, function, line, message);
	uf::iostream.pushHistory(string);
#if 1 || !UF_ENV_DREAMCAST
	::fmt::print("{}\n", string);
	fflush(stdout);
#endif

	uf::console::print( string );

	return string;
}