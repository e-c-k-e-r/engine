#pragma once

#include <uf/config.h>
#include "./allocator.h"

#include <string>

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
			class Traits = std::char_traits<CharT>,
			class Allocator = std::allocator<CharT> //uf::Allocator<CharT>
		>
		using basic_stringstream = std::basic_stringstream<CharT, Traits, Allocator>;
		using stringstream = uf::stl::basic_stringstream<char>;
	}
}