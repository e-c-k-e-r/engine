#pragma once

#include <uf/config.h>

#include <rapidjson/document.h>

namespace ext {
	namespace json {
		class UF_API Value;
		typedef rapidjson::Value base_value;
		struct UF_API Document;
	//	typedef rapidjson::Document Document
		class UF_API Value : public base_value {
		public:
			Value& operator=( const Value& );

			template<typename... Args> Value& operator[]( Args... args );
			template<typename... Args> const Value& operator[]( Args... args ) const;

			inline size_t size() const { return Size(); }
			
			inline decltype(auto) begin() { return Begin(); }
			inline decltype(auto) end() { return End(); }

			template<typename T> inline T get() const = delete;
			template<typename T> inline Value& operator=( T v ) = delete;
		};

		inline bool isTable( const Value& v ) { return v.IsArray() || v.IsObject(); }
		inline bool isObject( const Value& v ) { return v.IsObject(); }
		inline bool isArray( const Value& v ) { return v.IsArray(); }
		inline bool isNull( const Value& v ) { return v.IsNull(); }

		inline decltype(auto) array() {
			return ext::json::base_value(rapidjson::kArrayType).Move();
		}
		inline decltype(auto) object() {
			return ext::json::base_value(rapidjson::kObjectType).Move();
		}
		inline Value null() { return Value(); }
		
		std::vector<std::string> UF_API keys( const Value& v );
		static rapidjson::Document::AllocatorType allocator;

		struct Document : public rapidjson::Document {
		public:
			Document() : rapidjson::Document(&ext::json::allocator) {}

			template<typename... Args> Value& operator[]( Args... args );
			template<typename... Args> const Value& operator[]( Args... args ) const;
		};
	}
}

template<typename... Args> ext::json::Value& ext::json::Value::operator[]( Args... args ) {
	const std::string k(args...); 
	auto key = rapidjson::StringRef(k.c_str());
	auto it = ext::json::base_value::FindMember(key);
	if ( it != ext::json::base_value::MemberEnd() )
		return (ext::json::Value&) it->value;

	// member doesn't exist, make empty object
	ext::json::base_value::AddMember(key, ext::json::object(), ext::json::allocator);
	return this->operator[](args...);
}
template<typename... Args> const ext::json::Value& ext::json::Value::operator[]( Args... args ) const {
	const std::string k(args...); 
	auto key = rapidjson::StringRef(k.c_str());
	auto it = ext::json::base_value::FindMember(key);
	if ( it != ext::json::base_value::MemberEnd() )
		return (const ext::json::Value&) it->value;

	return ext::json::base_value();
}
template<> inline ext::json::Value& ext::json::Value::operator=<std::string>( std::string v ) {
	ext::json::base_value::SetString( v.c_str(), ext::json::allocator );
	return *this;
}
template<> inline ext::json::Value& ext::json::Value::operator=<bool>( bool v ) {
	ext::json::base_value::SetBool(v);
	return *this;
}
template<> inline ext::json::Value& ext::json::Value::operator=<int>( int v ) {
	ext::json::base_value::SetInt(v);
	return *this;
}
template<> inline ext::json::Value& ext::json::Value::operator=<size_t>( size_t v ) {
	ext::json::base_value::SetUint64(v);
	return *this;
}
template<> inline ext::json::Value& ext::json::Value::operator=<float>( float v ) {
	ext::json::base_value::SetFloat(v);
	return *this;
}
template<> inline ext::json::Value& ext::json::Value::operator=<double>( double v ) {
	ext::json::base_value::SetDouble(v);
	return *this;
}

template<typename... Args> ext::json::Value& ext::json::Document::operator[]( Args... args ) {
	const std::string k(args...); 
	auto key = rapidjson::StringRef(k.c_str());
	auto it = rapidjson::Document::FindMember(key);
	if ( it != rapidjson::Document::MemberEnd() )
		return (ext::json::Value&) it->value;

	// member doesn't exist, make empty object
	rapidjson::Document::AddMember(key, ext::json::object(), ext::json::allocator);
	return this->operator[](args...);
}
template<typename... Args> const ext::json::Value& ext::json::Document::operator[]( Args... args ) const {
	const std::string k(args...); 
	auto key = rapidjson::StringRef(k.c_str());
	auto it = rapidjson::Document::FindMember(key);
	if ( it != rapidjson::Document::MemberEnd() )
		return (const ext::json::Value&) it->value;

	return ext::json::base_value();
}

template<> inline bool ext::json::Value::is<bool>() const { return IsBool(); }
template<> inline bool ext::json::Value::is<int>() const { return IsInt64(); }
template<> inline bool ext::json::Value::is<size_t>() const { return IsUint64(); }
template<> inline bool ext::json::Value::is<float>() const { return IsDouble(); }
template<> inline bool ext::json::Value::is<double>() const { return IsDouble(); }
template<> inline bool ext::json::Value::is<std::string>() const { return IsString(); }

template<> inline bool ext::json::Value::get<bool>() const { return GetBool(); }
template<> inline int ext::json::Value::get<int>() const { return GetInt(); }
template<> inline size_t ext::json::Value::get<size_t>() const { return GetUint64(); }
template<> inline float ext::json::Value::get<float>() const { return GetDouble(); }
template<> inline double ext::json::Value::get<double>() const { return GetDouble(); }
template<> inline std::string ext::json::Value::get<std::string>() const { return GetString(); }

template<typename T> inline T ext::json::Value::as() const {
	if ( !is<T>() ) return T();
	return get<T>();
}
template<> inline bool ext::json::Value::as<bool>() const {
	if ( is<bool>() ) return get<bool>();
	if ( IsNull() ) return false;
	if ( IsNumber() ) return get<int>() != 0;
	if ( IsString() ) return get<std::string>() != "";
	if ( IsObject() ) return !ext::json::keys( *this ).empty();
	if ( IsArray() ) return size() > 0;
	return false;
}