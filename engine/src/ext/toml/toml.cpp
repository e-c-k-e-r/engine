#if UF_USE_TOML

#define TOML_HEADER_ONLY 0
#define TOML_ENABLE_FORMATTERS 1
#define TOML_IMPLEMENTATION

#if UF_EXCEPTIONS
	#define TOML_EXCEPTIONS 1
#elif UF_NO_EXCEPTIONS
	#define TOML_EXCEPTIONS 0
#else
	#define TOML_EXCEPTIONS 0
#endif
#include <toml++/toml.h>
#include <uf/ext/toml/toml.h>


#include <uf/ext/json/json.h>
#include <uf/utils/serialize/serializer.h>
using namespace std::string_view_literals;

TOML_NAMESPACE_START
{
	template<typename T>
	static void from_json(const ext::json::Value& j, value<T>& val) {
		assert(j.is<T>());
		*val = j.as<T>();
	}
	static void from_json(const ext::json::Value&, array&);

	template <typename T, typename Key>
	static void insert_from_json(table & tbl, Key && key, const ext::json::Value& val) {
		T v;
		from_json(val, v);

		tbl.insert_or_assign(static_cast<Key&&>(key), std::move(v));
	}

	template <typename T>
	static void insert_from_json(array & arr, const ext::json::Value& val) {
		T v;
		from_json(val, v);
		arr.push_back(std::move(v));
	}

	static void from_json(const ext::json::Value& j, table& tbl) {
		if ( j.is_null() ) return;
		
		if( !j.is_object() ) {
			UF_MSG_ERROR("ERROR: {}", j.dump());
			return;
		}

		ext::json::forEach(j, [&]( const uf::stl::string& k, const ext::json::Value& v ){
			if ( v.is<std::string>() ) insert_from_json<toml::value<std::string>>(tbl, k, v);
			else if ( v.is<double>(true) ) insert_from_json<toml::value<double>>(tbl, k, v);
			else if ( v.is<int64_t>() ) insert_from_json<toml::value<int64_t>>(tbl, k, v);
			else if ( v.is<bool>() ) insert_from_json<toml::value<bool>>(tbl, k, v);
			else if (v.is_array()) insert_from_json<toml::array>(tbl, k, v);
			else insert_from_json<toml::table>(tbl, k, v);
		});
	}

	static void from_json(const ext::json::Value& j, array& arr) {
		ext::json::forEach(j, [&]( const ext::json::Value& v ){
			if ( v.is<std::string>() )
				insert_from_json<toml::value<std::string>>(arr, v);
			else if ( v.is<double>(true) ) insert_from_json<toml::value<double>>(arr, v);
			else if ( v.is<int64_t>() ) insert_from_json<toml::value<int64_t>>(arr, v);
			else if ( v.is<bool>() ) insert_from_json<toml::value<bool>>(arr, v);
			else if (v.is_array()) insert_from_json<toml::array>(arr, v);
			else insert_from_json<toml::table>(arr, v);
		});
	}
}
TOML_NAMESPACE_END;

uf::stl::string ext::toml::fromJson( const uf::stl::string& source ) {
	ext::json::Value j;
	ext::json::decode( j, source );

#if UF_EXCEPTIONS
	try {
#endif
		::toml::table tbl;
		::toml::from_json( j, tbl );

		std::stringstream ss; ss << tbl;
		return ss.str();
#if UF_EXCEPTIONS
	} catch ( const ::toml::parse_error& e ) {
		UF_MSG_ERROR("TOML error: {}", e.what());
	}
	return "";
#endif
}
uf::stl::vector<uint8_t> ext::toml::fromJson( const uf::stl::vector<uint8_t>& source ) {
	ext::json::Value j;
	ext::json::decode( j, source );

#if UF_EXCEPTIONS
	try {
#endif
		::toml::table tbl;
		::toml::from_json( j, tbl );

		std::stringstream ss; ss << tbl;
		uf::stl::string str = ss.str();
		return uf::stl::vector<uint8_t>{ str.begin(), str.end() };
#if UF_EXCEPTIONS
	} catch ( const ::toml::parse_error& e ) {
		UF_MSG_ERROR("TOML error: {}", e.what());
	}
	return {};
#endif
}
uf::stl::string ext::toml::toJson( const uf::stl::string& source ) {
#if UF_EXCEPTIONS
	try {
#endif
	::toml::parse_result result = ::toml::parse( source );
	if ( !result ) {
		std::stringstream error; error << result.error();
		UF_MSG_ERROR("TOML error: {}", error.str());
		return "";
	}
	std::stringstream ss; ss << ::toml::json_formatter{ result };
	return ss.str();
#if UF_EXCEPTIONS
	} catch ( const ::toml::parse_error& e ) {
		UF_MSG_ERROR("TOML error: {}", e.what());
	}
	return "";
#endif
}
uf::stl::vector<uint8_t> ext::toml::toJson( const uf::stl::vector<uint8_t>& source ) {
#if UF_EXCEPTIONS
	try {
#endif
	std::string_view source_view( (const char*) source.data(), source.size() );
	::toml::parse_result result = ::toml::parse( source_view );
	if ( !result ) {
		std::stringstream error; error << result.error();
		UF_MSG_ERROR("TOML error: {}", error.str());
		return {};
	}
	std::stringstream ss; ss << ::toml::json_formatter{ result };
	uf::stl::string str = ss.str();
	return uf::stl::vector<uint8_t>{ str.begin(), str.end() };
#if UF_EXCEPTIONS
	} catch ( const ::toml::parse_error& e ) {
		UF_MSG_ERROR("TOML error: {}", e.what());
	}
	return {};
#endif
}
#endif