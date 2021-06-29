#pragma once
#include "default.h"

namespace uf {
	namespace locale {
		class UF_API Utf32 : public Encoding {
		public:
			typedef uint32_t literal_t;
		protected:
			typedef literal_t Literal;
			Literal m_literal;
		public:
		// 	Converts from UTF-32 to default encoding (UTF-32) (so nothing)
			template<typename Input>
			static Input UF_API_CALL decode( Input begin, Input end, Encoding::Literal& output, Encoding::Literal fallback = 0 );
			template<typename Input>
			inline static Encoding::Literal UF_API_CALL decode( Input begin, Input end, Encoding::Literal fallback = 0 ) {
				return *begin++;
			}
		// 	Converts from default encoding (UTF-32) to UTF-32 (so nothing)
			template<typename Input, typename Output>
			static Output UF_API_CALL encode( Input input, Output output, Literal fallback = 0 );
		// 	Go to the next character in UTF-32's sequence
			template<typename Input>
			static Input UF_API_CALL next(Input begin, Input end);
		// 	Number of characters in UTF-32's sequence
			template<typename Input>
			static Input UF_API_CALL length(Input begin, Input end);
		// 	Encoding's name
			static uf::stl::string UF_API_CALL getName() { return "UTF-32"; }
		};
	}
}
/*
#include "encoding_constructor.h"
ENCODING_CONSTRUCTOR(Utf32: public Encoding, uint32_t, "UTF-32")
*/
#include "utf32.inl"