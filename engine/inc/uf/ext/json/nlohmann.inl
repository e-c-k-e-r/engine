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

template<typename T> ext::json::Value& ext::json::Value::emplace_back( const T& v ) {
	return (ext::json::Value&) ext::json::base_value::emplace_back(v);
}
/*
template<> ext::json::Value& ext::json::Value::emplace_back<ext::json::Value>( const ext::json::Value& value ) {
	return (ext::json::Value&) ext::json::base_value::emplace_back( (const ext::json::base_value&) value );
}
*/

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
template<> inline bool ext::json::Value::is<uf::stl::string>(bool strict) const { return is_string(); }

// template<> template<typename T, size_t N> inline bool ext::json::Value::is<pod::Vector<T,N>>(bool strict) const { return is_array() && size() == N; }
// template<> inline bool ext::json::Value::is<std::string>(bool strict) const { return is_string(); }

#if UF_ENV_DREAMCAST
	template<> inline bool ext::json::Value::is<int>(bool strict) const { return strict ? is_number_integer() : is_number(); }
	template<> inline bool ext::json::Value::is<unsigned int>(bool strict) const { return strict ? is_number_unsigned() : is_number(); }
#endif
template<typename T> inline T ext::json::Value::as() const {
	return !is<T>(false) ? T() : get<T>();
/*
	if ( !is<T>() ) return T();
	return get<T>();
*/
}
template<typename T> inline T ext::json::Value::as( const T& fallback ) const {
	return !is<T>(false) ? fallback : get<T>();
/*
	if ( !is<T>() ) return fallback;
	return get<T>();
*/
}
template<> inline bool ext::json::Value::as<bool>() const {
	// explicitly a bool
	if ( is<bool>() ) return get<bool>();
	// implicit conversion
	if ( is_null() ) return false; // always return falce
	if ( is_number() ) return get<int>(); // implicit conversion to bool
	if ( is_string() ) return get<uf::stl::string>() != ""; // true if not empty
	if ( is_object() ) return !ext::json::keys( *this ).empty(); // true if not empty
	if ( is_array() ) return size() > 0; // true if not empty
	return false;
}