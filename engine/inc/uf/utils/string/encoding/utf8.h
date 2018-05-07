#pragma once
#include "default.h"

namespace uf {
	namespace locale {
		class UF_API Utf8 : public Encoding {
		public:
			typedef uint8_t literal_t;
		protected:
			typedef literal_t Literal;
			Literal m_literal;
		public:
		// 	Converts from UTF-8 to UTF-32
			template<typename Input>
			static Input UF_API_CALL decode( Input begin, Input end, Encoding::Literal& output, Encoding::Literal fallback = 0 );
			template<typename Input>
			inline static Encoding::Literal UF_API_CALL decode( Input begin, Input end, Encoding::Literal fallback = 0 ) {
				Encoding::Literal literal;
				Utf8::decode(begin, end, literal, fallback);
				return literal;
			}
		// 	Converts from UTF-32 to UTF-8
			template<typename Input, typename Output>
			static Output UF_API_CALL encode( Input input, Output output, Literal fallback = 0 );
		// 	Go to the next character in UTF-8's sequence
			template<typename Input>
			static Input UF_API_CALL next(Input begin, Input end);
		// 	Number of characters in UTF-8's sequence
			template<typename Input>
			static Input UF_API_CALL length(Input begin, Input end);
		// 	Encoding's name
			static std::string UF_API_CALL getName() { return "UTF-8"; }
		};
	}
}
/*
#include "encoding_constructor.h"
ENCODING_CONSTRUCTOR(Utf8: public Encoding, uint8_t, "UTF-8")
*/
#include "utf8.inl"