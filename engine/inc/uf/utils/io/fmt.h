#pragma once

#if UF_USE_FMT
	#if UF_ENV_DREAMCAST || UF_ENV_LINUX
		#define FMT_HEADER_ONLY
	#endif
	#include <fmt/core.h>
	#include <fmt/format.h>
	#include <fmt/chrono.h>
#endif
#include <uf/utils/memory/string.h>

namespace uf {
    template <typename... Args>
    inline uf::stl::string _fmt_format(fmt::format_string<Args...> fmt_str, Args&&... args) {
        fmt::memory_buffer buffer;
        fmt::format_to(std::back_inserter(buffer), fmt_str, std::forward<Args>(args)...);
        return uf::stl::string(buffer.data(), buffer.size());
    }
}

namespace uf {
#if UF_USE_FMT
	using namespace fmt;
#endif
	namespace io {
	//	void UF_API exception( const uf::stl::string& message );
		uf::stl::string UF_API log( const uf::stl::string& category, const uf::stl::string& file, const uf::stl::string& function, size_t line, const uf::stl::string& message );
	}
}