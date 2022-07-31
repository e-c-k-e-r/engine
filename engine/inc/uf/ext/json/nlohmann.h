#pragma once

#include <uf/config.h>

#include <uf/utils/memory/vector.h>
#include <uf/utils/memory/string.h>

#if !UF_EXCEPTIONS
	#define JSON_NOEXCEPTION 1
#endif
// #define NDEBUG 1

#include <nlohmann/json.hpp>
#include <nlohmann/fifo_map.hpp>

#define UF_JSON_NLOHMANN_ORDERED 0
#define UF_JSON_NLOHMANN_FIFO_MAP 0

namespace uf {
	class UF_API Serializer;
}

namespace ext {
	namespace json {
		class UF_API Value;

	#if UF_JSON_NLOHMANN_ORDERED
		#if UF_JSON_NLOHMANN_FIFO_MAP
			template<class K, class V, class dummy_compare, class A>
			using fifo_map = nlohmann::fifo_map<K, V, nlohmann::fifo_map_compare<K>, A>;
			typedef nlohmann::basic_json<fifo_map> base_value;
		#else
			typedef nlohmann::ordered_json base_value;
		#endif
	#else
		typedef nlohmann::json base_value;
	#endif

		class UF_API Value : public base_value {
		public:
			Value& operator=( const uf::Serializer& );
			Value& operator=( const Value& );

			template<typename... Args> Value& operator[]( Args... args );
			template<typename... Args> const Value& operator[]( Args... args ) const;
			template<typename... Args> Value& operator=( Args... args );
		//	template<typename Arg> Value& operator=( Arg arg );

		//	template<typename... Args> Value& emplace_back( Args... args );
			template<typename T> Value& emplace_back( const T& v );
			Value& emplace_back( const ext::json::Value& = {} );
			Value& emplace_back( const uf::Serializer& );

			template<typename T> inline bool is( bool = false ) const = delete;
			template<typename T> inline T as() const;
			template<typename T> inline T as(const T& fallback) const;

			Value& reserve( size_t );
		};

		inline bool isTable( const Value& v ) { return v.is_array() || v.is_object(); }
		inline bool isObject( const Value& v ) { return v.is_object(); }
		inline bool isArray( const Value& v ) { return v.is_array(); }
		inline bool isNull( const Value& v ) { return v.is_null(); }

		inline Value array() { return Value/*::array*/(); } // nu-nlohmann json bitches about `[json.exception.type_error.302] type must be string, but is array`
		inline Value object() { return Value/*::object*/(); } // nu-nlohmann json bitches about `[json.exception.type_error.302] type must be string, but is array`
		inline Value null() { return Value(); }
		Value& UF_API reserve( Value& value, size_t size );
		
		uf::stl::vector<uf::stl::string> UF_API keys( const Value& v );
	}
}

#include "nlohmann.inl"