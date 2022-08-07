#include <uf/utils/io/fmt.h>
#include <uf/utils/io/payloads.h>

#include <uf/utils/hook/hook.h>
#if UF_USE_IMGUI
	#include <uf/ext/imgui/imgui.h>
#else
	#include <iostream>
	#include <sstream>
#endif


uf::stl::string uf::io::log( const uf::stl::string& category, const uf::stl::string& file, const uf::stl::string& function, size_t line, const uf::stl::string& message ) {
#if UF_USE_FMT
	auto string = ::fmt::format("[{}] [{}:{}@{}]: {}", category, file, function, line, message);
	::fmt::print("{}\n", string);
#else
	std::stringstream ss;
	ss << "[" << category << "] [" << file << ":" << function << "@" << line << "]: " << message << "\n";
	auto string = ss.str();
	std::cout << string;
#endif
	std::cout.flush();

#if UF_USE_IMGUI
	ext::imgui::log( string );
#endif

	return string;
}