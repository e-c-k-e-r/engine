#include <uf/ext/lua/lua.h>
#if UF_USE_LUA
#include <uf/engine/asset/asset.h>

namespace binds {
	void load( uf::Asset& asset, sol::variadic_args va ) {
		auto it = va.begin();
		std::string callback = "";
		std::string uri = "";
		std::string hash = "";
		std::string category = "";
		if ( it != va.end() ) callback = *(it++);
		if ( it != va.end() ) uri = *(it++);
		if ( it != va.end() ) hash = *(it++);
		if ( it != va.end() ) category = *(it++);
		if ( callback == "" ) asset.load( uri, hash, category );
		else asset.load( callback, uri, hash, category );
	}
	void cache( uf::Asset& asset, sol::variadic_args va ) {
		auto it = va.begin();
		std::string callback = "";
		std::string uri = "";
		std::string hash = "";
		std::string category = "";
		if ( it != va.end() ) callback = *(it++);
		if ( it != va.end() ) uri = *(it++);
		if ( it != va.end() ) hash = *(it++);
		if ( it != va.end() ) category = *(it++);
		if ( callback == "" )
			asset.cache( uri, hash, category );
		else
			asset.cache( callback, uri, hash, category );
	}
	std::string getOriginal( uf::Asset& asset, const std::string& n ) {
		return asset.getOriginal( n );
	}
}

UF_LUA_REGISTER_USERTYPE(uf::Asset,
	UF_LUA_REGISTER_USERTYPE_DEFINE( load, UF_LUA_C_FUN(::binds::load) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( cache, UF_LUA_C_FUN(::binds::cache) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( getOriginal, UF_LUA_C_FUN(::binds::getOriginal) )
	)
#endif