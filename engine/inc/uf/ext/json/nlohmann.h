#pragma once

#include <uf/config.h>

#include <nlohmann/json.hpp>
#include <nlohmann/fifo_map.hpp>

#define UF_JSON_NLOHMANN_ORDERED 1

namespace ext {
	namespace json {
		class UF_API Value;

	#if UF_JSON_NLOHMANN_ORDERED
		template<class K, class V, class dummy_compare, class A>
		using fifo_map = nlohmann::fifo_map<K, V, nlohmann::fifo_map_compare<K>, A>;
		typedef nlohmann::basic_json<fifo_map> base_value;
	#else
		typedef nlohmann::json base_value;
	#endif

		typedef Value Document;
		class UF_API Value : public base_value {
		public:
			Value& operator=( const Value& );

			template<typename... Args> Value& operator[]( Args... args );
			template<typename... Args> const Value& operator[]( Args... args ) const;
			template<typename... Args> Value& operator=( Args... args );

			template<typename T> inline bool is( bool = false ) const = delete;
			template<typename T> inline T as() const;
			template<typename T> inline T as(const T& fallback) const;
		};

		inline bool isTable( const Value& v ) { return v.is_array() || v.is_object(); }
		inline bool isObject( const Value& v ) { return v.is_object(); }
		inline bool isArray( const Value& v ) { return v.is_array(); }
		inline bool isNull( const Value& v ) { return v.is_null(); }

		inline Value array() { return Value::array(); }
		inline Value object() { return Value::object(); }
		inline Value null() { return Value(); }
		
		std::vector<std::string> UF_API keys( const Value& v );
	}
}
/*
namespace pod {
	template<typename T, size_t N> struct UF_API Vector;
	template<typename T, size_t R, size_t C> struct UF_API Matrix;
	template<typename T> struct UF_API Transform;
}

template<typename T, size_t N> ext::json::Value& ext::json::Value::operator=( const pod::Vector<T,N>& v ) {
	ext::json::base_value::operator=( ext::json::encode(v) );
	return *this;
}
template<typename T, size_t R, size_t C = R> ext::json::Value& ext::json::Value::operator=( const pod::Matrix<T,R,C>& m ) {
	ext::json::base_value::operator=( ext::json::encode(m) );
	return *this;
}
template<typename T> ext::json::Value& ext::json::Value::operator=( const pod::Vector<T,R,C>& t ) {
	ext::json::base_value::operator=( ext::json::encode(t) );
	return *this;
}
*/
template<typename... Args> ext::json::Value& ext::json::Value::operator[]( Args... args ) {
	return (ext::json::Value&) ext::json::base_value::operator[](args...);
}
template<typename... Args> const ext::json::Value& ext::json::Value::operator[]( Args... args ) const {
	return (const ext::json::Value&) ext::json::base_value::operator[](args...);
}
template<typename... Args> ext::json::Value& ext::json::Value::operator=( Args... args ) {
	ext::json::base_value::operator=(args...);
	return *this;
}

template<> inline bool ext::json::Value::is<bool>(bool strict) const { return is_boolean(); }
template<> inline bool ext::json::Value::is<int8_t>(bool strict) const { return strict ? is_number_integer() : is_number(); }
template<> inline bool ext::json::Value::is<int16_t>(bool strict) const { return strict ? is_number_integer() : is_number(); }
template<> inline bool ext::json::Value::is<int32_t>(bool strict) const { return strict ? is_number_integer() : is_number(); }
template<> inline bool ext::json::Value::is<int64_t>(bool strict) const { return strict ? is_number_integer() : is_number(); }
template<> inline bool ext::json::Value::is<uint8_t>(bool strict) const { return strict ? is_number_unsigned() : is_number(); }
template<> inline bool ext::json::Value::is<uint16_t>(bool strict) const { return strict ? is_number_unsigned() : is_number(); }
template<> inline bool ext::json::Value::is<uint32_t>(bool strict) const { return strict ? is_number_unsigned() : is_number(); }
template<> inline bool ext::json::Value::is<uint64_t>(bool strict) const { return strict ? is_number_unsigned() : is_number(); }
template<> inline bool ext::json::Value::is<float>(bool strict) const { return strict ? is_number_float() : is_number(); }
template<> inline bool ext::json::Value::is<double>(bool strict) const { return strict ? is_number_float() : is_number(); }
template<> inline bool ext::json::Value::is<std::string>(bool strict) const { return is_string(); }

#if UF_ENV_DREAMCAST
	template<> inline bool ext::json::Value::is<int>(bool strict) const { return strict ? is_number_integer() : is_number(); }
	template<> inline bool ext::json::Value::is<unsigned int>(bool strict) const { return strict ? is_number_unsigned() : is_number(); }
#endif
template<typename T> inline T ext::json::Value::as() const {
	if ( !is<T>() ) return T();
	return get<T>();
}
template<typename T> inline T ext::json::Value::as( const T& fallback ) const {
	if ( !is<T>() ) return fallback;
	return get<T>();
}
template<> inline bool ext::json::Value::as<bool>() const {
	// explicitly a bool
	if ( is<bool>() ) return get<bool>();
	// implicit conversion
	if ( is_null() ) return false; // always return falce
	if ( is_number() ) return get<int>(); // implicit conversion to bool
	if ( is_string() ) return get<std::string>() != ""; // true if not empty
	if ( is_object() ) return !ext::json::keys( *this ).empty(); // true if not empty
	if ( is_array() ) return size() > 0; // true if not empty
	return false;
}