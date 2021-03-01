#include <uf/ext/lua/lua.h>
#if UF_USE_LUA
#include <uf/engine/asset/asset.h>

UF_LUA_REGISTER_USERTYPE(uf::Asset,
	UF_LUA_REGISTER_USERTYPE_DEFINE( load, []( uf::Asset& asset, sol::variadic_args va ) {
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
			asset.load( uri, hash, category );
		else
			asset.load( callback, uri, hash, category );
	}),
	UF_LUA_REGISTER_USERTYPE_DEFINE( cache, []( uf::Asset& asset, sol::variadic_args va ) {
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
	}),
	UF_LUA_REGISTER_USERTYPE_MEMBER( uf::Asset::getOriginal )
)
#endif