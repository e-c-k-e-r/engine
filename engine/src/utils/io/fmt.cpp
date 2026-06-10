#include <uf/utils/io/fmt.h>
#include <uf/utils/io/payloads.h>
#include <uf/utils/io/iostream.h>

#include <uf/utils/hook/hook.h>
#include <uf/utils/io/console.h>
#include <uf/utils/io/fmt.h>

#include <iostream>
#include <sstream>

/*
void uf::io::exception( const uf::stl::string& exception ) {
	std::abort(-1);
}
*/
uf::stl::string uf::io::log( const uf::stl::string& category, const uf::stl::string& file, const uf::stl::string& function, size_t line, const uf::stl::string& message ) {
	auto string = ::fmt::format("[{}] [{}:{}@{}]: {}", category, file, function, line, message);
	::fmt::print("{}\n", string);
	uf::iostream.pushHistory(string);
	std::cout.flush();

	uf::console::print( string );

	return string;
}