#pragma once
#include "default.h"

namespace uf {
	namespace locale {
		class UF_API Utf16 : public Encoding {
		public:
			typedef uint16_t literal_t;
		protected:
			typedef literal_t Literal;
			Literal m_literal;
		public:
		// 	Converts from UTF-16 to UTF-32
			template<typename Input>
			static Input UF_API_CALL decode( Input begin, Input end, Encoding::Literal& output, Encoding::Literal fallback = 0 );
			template<typename Input>
			inline static Encoding::Literal UF_API_CALL decode( Input begin, Input end, Encoding::Literal fallback = 0 ) {
				Encoding::Literal literal;
				Utf16::decode(begin, end, literal, fallback);
				return literal;
			}
		// 	Converts from UTF-32 to UTF-16
			template<typename Input, typename Output>
			static Output UF_API_CALL encode( Input input, Output output, Literal fallback = 0 );
		// 	Go to the next character in UTF-16's sequence
			template<typename Input>
			static Input UF_API_CALL next(Input begin, Input end);
		// 	Number of characters in UTF-16's sequence
			template<typename Input>
			static Input UF_API_CALL length(Input begin, Input end);
		// 	Encoding's name
			static std::string UF_API_CALL getName() { return "UTF-16"; }
		};
	}
}
/*
#include "encoding_constructor.h"
ENCODING_CONSTRUCTOR(Utf16: public Encoding, uint16_t, "UTF-16")
*/
#include "utf16.inl"