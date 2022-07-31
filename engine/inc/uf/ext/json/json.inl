template<typename T>
uf::stl::vector<T> ext::json::vector( const ext::json::Value& value ) {
	uf::stl::vector<T> res;
	res.reserve( value.size() );
	ext::json::forEach( value, [&]( const ext::json::Value& value ){
		res.emplace_back( value.as<T>() );
	});
	return res;
}

template<typename T>
T& ext::json::encode( const ext::json::Value& json, T& output, const ext::json::EncodingSettings& settings ) {
//	ext::json::Value json = ext::json::reencode( _json, settings );

#define UF_JSON_PARSE_ENCODING(ENC)\
	if ( settings.encoding == #ENC ) {\
		auto buffer = nlohmann::json::to_ ## ENC( (const ext::json::base_value&) (json) );\
		output = T( buffer.begin(), buffer.end() );\
}

	if ( settings.encoding == ""  || settings.encoding == "json" ) {
		output = settings.pretty ? json.dump(1, '\t') : json.dump();
	}
	else UF_JSON_PARSE_ENCODING(bson)
	else UF_JSON_PARSE_ENCODING(cbor)
	else UF_JSON_PARSE_ENCODING(msgpack)
	else UF_JSON_PARSE_ENCODING(ubjson)
	else UF_JSON_PARSE_ENCODING(bjdata)
#if UF_USE_TOML
	else if ( settings.encoding == "toml" ) {
		uf::stl::string buffer = ext::toml::fromJson( json.dump() );
		output = T( buffer.begin(), buffer.end() );
	}
#endif
	else {
		// should probably default to json, not my problem
		UF_MSG_ERROR("invalid encoding requested: {}", settings.encoding);
	}
	return output;
#undef UF_JSON_PARSE_ENCODING
}
template<typename T>
ext::json::Value& ext::json::decode( ext::json::Value& json, const T& input, const DecodingSettings& settings ) {
#define UF_JSON_PARSE_ENCODING(ENC)\
	if ( settings.encoding == #ENC ) {\
		json = nlohmann::json::from_ ## ENC(input, strict, exceptions);\
	}

	constexpr bool comments = true;
	constexpr bool strict = true;
	constexpr bool exceptions = true;
#if UF_EXCEPTIONS
	try {
#endif
		if ( settings.encoding == "" || settings.encoding == "json" ) {
			json = nlohmann::json::parse(input, nullptr, exceptions, comments);
		}
		else UF_JSON_PARSE_ENCODING(bson)
		else UF_JSON_PARSE_ENCODING(cbor)
		else UF_JSON_PARSE_ENCODING(msgpack)
		else UF_JSON_PARSE_ENCODING(ubjson)
		else UF_JSON_PARSE_ENCODING(bjdata)
#if UF_USE_TOML
		else if ( settings.encoding == "toml" ) {
			T parsed = ext::toml::toJson( input );
			json = nlohmann::json::parse(parsed, nullptr, exceptions, comments);
		}
#endif
		else {
			// should probably default to json, not my problem
			UF_MSG_ERROR("invalid encoding requested: {}", settings.encoding);
		}
#if UF_EXCEPTIONS
	} catch ( nlohmann::json::parse_error& e ) {
		UF_MSG_ERROR("JSON error: {}", e.what());
	}
#endif
	return json;

#undef UF_JSON_PARSE_ENCODING
}