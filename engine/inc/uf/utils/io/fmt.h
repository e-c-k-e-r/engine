#pragma once

#if UF_USE_FMT
	#if UF_ENV_DREAMCAST
		#define FMT_HEADER_ONLY
	//	#include <fmt-1/format.h>

	//	#include <fmt-3/format.h>
	//	#include <fmt-8/format.h>
		#include <fmt-7/format.h>
	//	#include <fmt-7/core.h>
	#else
	//	#include <fmt/core.h>
	#endif
	#include <fmt/format.h>
#endif
#include <uf/utils/memory/string.h>

namespace uf {
#if UF_USE_FMT
	using namespace fmt;
#endif
	namespace io {
		uf::stl::string UF_API log( const uf::stl::string& category, const uf::stl::string& file, const uf::stl::string& function, size_t line, const uf::stl::string& message );
	}
}