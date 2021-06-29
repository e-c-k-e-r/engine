#pragma once

#include <uf/config.h>

#include <uf/utils/memory/vector.h>
#include <stdint.h>

namespace pod {
	template<typename T, typename U = uint16_t>
	struct RLE {
		typedef uf::stl::vector<pod::RLE<T,U>> string_t;
		typedef U length_t;
		typedef T value_t;
		U length;
		T value;
	};
}

namespace uf {
	namespace rle {
		template<typename T, typename U = uint16_t>
		typename pod::RLE<T,U>::string_t encode( const uf::stl::vector<T>& );
		template<typename T, typename U = uint16_t>
		uf::stl::vector<T> decode( const pod::RLE<T,U>& );
	}
}

#include "rle.inl"

/*
namespace pod {
	struct RLE {
		typedef uf::stl::vector<pod::RLE> string_t;
		uint16_t length;
		uint16_t value;
	};
}

namespace uf {
	namespace rle {
		UF_API pod::RLE::string_t encode( const uf::stl::vector<uint16_t>& );
		UF_API uf::stl::vector<uint16_t> decode( const pod::RLE::string_t& );

		UF_API pod::RLE::string_t wrap( const uf::stl::vector<uint16_t>& );
		UF_API uf::stl::vector<uint16_t> unwrap( const pod::RLE::string_t& );
	}
}
*/