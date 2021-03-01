#pragma once

#define ENCODING_CONSTRUCTOR(CLASS_NAME, LITERAL_TYPE, ENCODING_NAME) \
namespace uf { \
	namespace locale { \
		// Internal encoding used is UTF-32 \
		class UF_API CLASS_NAME { \
		public: \
			typedef LITERAL_TYPE literal_t; \
		protected: \
			typedef literal_t Literal; \
			Literal m_literal; \
		public: \
		// 	Converts from [this encoding] to [arbitrary encoding] \
			template<typename Input> \
			static Input UF_API_CALL decode( Input begin, Input end, Encoding::Literal& output, Encoding::Literal fallback = 0 ); \
			template<typename Input> \
			static Encoding::Literal UF_API_CALL decode( Input begin, Input end, Encoding::Literal fallback = 0 ); \
		// 	Converts from [arbitrary encoding] to [this encoding] \
			template<typename Input, typename Output> \
			static Output UF_API_CALL encode( Input input, Output output, Output fallback = 0 ); \
		// 	Go to the next character in [this encoding]'s sequence \
			template<typename Input> \
			static Input UF_API_CALL next(Input begin, Input end); \
		// 	Number of characters in [this encoding]'s sequence \
			template<typename Input> \
			static Input UF_API_CALL length(Input begin, Input end); \
		// 	Encoding's name \
			static std::string name = ENCODING_NAME; \
		}; \
	} \
}