#pragma once

#include <uf/config.h>
#include "./allocator.h"

#include <string>
#include <string_view>

// strings with custom allocators really do not play nice with existing libraries
namespace uf {
	namespace stl {
		template<
			class CharT,
    		class Traits = std::char_traits<CharT>,
    		class Allocator = std::allocator<CharT> //uf::Allocator<CharT>
		>
		using basic_string = std::basic_string<CharT, Traits, Allocator>;
		using string = uf::stl::basic_string<char>;

		template<
            class CharT,
            class Traits = std::char_traits<CharT>
        >
        using basic_string_view = std::basic_string_view<CharT, Traits>;
        using string_view = uf::stl::basic_string_view<char>;

		template<
			class CharT,
			class Traits = std::char_traits<CharT>,
			class Allocator = std::allocator<CharT> //uf::Allocator<CharT>
		>
		using basic_stringstream = std::basic_stringstream<CharT, Traits, Allocator>;
		using stringstream = uf::stl::basic_stringstream<char>;
	}
}