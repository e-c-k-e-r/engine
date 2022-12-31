#include <uf/utils/io/fmt.h>
#include <uf/utils/io/payloads.h>
#include <uf/utils/io/iostream.h>

#include <uf/utils/hook/hook.h>
#include <uf/utils/io/console.h>

#include <iostream>
#include <sstream>

/*
void uf::io::exception( const uf::stl::string& exception ) {
	std::abort(-1);
}
*/
uf::stl::string uf::io::log( const uf::stl::string& category, const uf::stl::string& file, const uf::stl::string& function, size_t line, const uf::stl::string& message ) {
#if UF_USE_FMT
	auto string = ::fmt::format("[{}] [{}:{}@{}]: {}", category, file, function, line, message);
	::fmt::print("{}\n", string);
	uf::iostream.pushHistory(string);
#else
	std::stringstream ss;
	ss << "[" << category << "] [" << file << ":" << function << "@" << line << "]: " << message << "\n";
	auto string = ss.str();
	std::cout << string;
	uf::iostream.pushHistory(string);
#endif
	std::cout.flush();

	uf::console::print( string );

	return string;
}