#pragma once

#if UF_USE_FMT
	#if UF_ENV_DREAMCAST
		#define FMT_HEADER_ONLY
	#endif
	#include <fmt/core.h>
	#include <fmt/format.h>
	#include <fmt/chrono.h>
#endif
#include <uf/utils/memory/string.h>

namespace uf {
#if UF_USE_FMT
	using namespace fmt;
#endif
	namespace io {
	//	void UF_API exception( const uf::stl::string& message );
		uf::stl::string UF_API log( const uf::stl::string& category, const uf::stl::string& file, const uf::stl::string& function, size_t line, const uf::stl::string& message );
	}
}