#include <uf/ext/lua/lua.h>

#include <uf/engine/asset/asset.h>

UF_LUA_REGISTER_USERTYPE(uf::Asset,
	UF_LUA_REGISTER_USERTYPE_DEFINE( load, []( uf::Asset& asset, const std::string& uri, const std::string& callback = "" ) {
		if ( callback == "" )
			asset.load( uri );
		else
			asset.load( uri, callback );
	}),
	UF_LUA_REGISTER_USERTYPE_DEFINE( cache, []( uf::Asset& asset, const std::string& uri, const std::string& callback = "" ) {
		if ( callback == "" )
			asset.cache( uri );
		else
			asset.cache( uri, callback );
	}),
	UF_LUA_REGISTER_USERTYPE_MEMBER( uf::Asset::getOriginal )
)