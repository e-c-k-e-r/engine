#pragma once

#include <uf/config.h>
#include <uf/utils/memory/string.h>

namespace uf {
	namespace locale {
		class UF_API Literal {
		public:
			typedef uint8_t min_t;
			typedef uint32_t max_t;
			typedef uint32_t default_t;
		};

		// Internal encoding used is UTF-32
		class UF_API Encoding {
		public:
			typedef uf::locale::Literal::default_t literal_t;
		protected:
			typedef literal_t Literal;
			Literal m_literal;
		public:
		// 	Converts from [this encoding] to [arbitrary encoding]
			template<typename Input>
			static Input UF_API_CALL decode( Input begin, Input end, Literal& output, Literal fallback = 0 );
			template<typename Input>
			inline static Literal UF_API_CALL decode( Input begin, Input end, Literal fallback = 0 ) {
				return *begin++;
			}
		// 	Converts from [arbitrary encoding] to [this encoding]
			template<typename Input, typename Output>
			static Output UF_API_CALL encode( Input input, Output output, Literal fallback = 0 );
		// 	Go to the next character in [this encoding]'s sequence
			template<typename Input>
			static Input UF_API_CALL next(Input begin, Input end);
		// 	Number of characters in [this encoding]'s sequence
			template<typename Input>
			static Input UF_API_CALL length(Input begin, Input end);
		// 	Encoding's name
			static uf::stl::string UF_API_CALL getName() { return "UTF-32"; }
		};

	}
}
/*
#include "encoding_constructor.h"
ENCODING_CONSTRUCTOR(Encoding, uf::locale::Literal::default_t, "UTF-32")
*/
#include "default.inl"