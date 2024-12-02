#include <uf/engine/object/behaviors/lua.h>

#include <uf/engine/object/object.h>
#include <uf/engine/asset/asset.h>
#include <uf/engine/scene/scene.h>
#include <uf/utils/time/time.h>
#include <uf/utils/math/transform.h>
#include <uf/utils/math/physics.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/mesh/mesh.h>
#include <uf/engine/asset/asset.h>
#include <uf/ext/lua/lua.h>

UF_BEHAVIOR_REGISTER_CPP(uf::LuaBehavior)
UF_BEHAVIOR_TRAITS_CPP(uf::LuaBehavior, ticks = false, renders = false, multithread = false)
#define this (&self)
void uf::LuaBehavior::initialize( uf::Object& self ) {	
#if UF_USE_LUA
	if ( !ext::lua::enabled ) return;
	
	this->addHook( "asset:Load.%UID%", [&](pod::payloads::assetLoad& payload){
		if ( !uf::asset::isExpected( payload, uf::asset::Type::LUA ) ) return;
		if ( !uf::asset::has( payload ) ) uf::asset::load( payload );
		auto& script = uf::asset::get<pod::LuaScript>( payload );
		if ( !payload.asComponent ) {
		//	auto asset = uf::asset::release( payload.filename );
		//	this->moveComponent<pod::LuaScript>( asset );
			this->moveComponent<pod::LuaScript>( uf::asset::get( payload.filename ) );
			uf::asset::remove( payload.filename );
		}
	/*
		if ( !uf::asset::has(payload.filename) ) uf::asset::load( payload );
		auto& script = uf::asset::get<pod::LuaScript>(payload.filename);
	*/
	//	auto& script = this->getComponent<pod::LuaScript>();
		script.env["ent"] = &this->as<uf::Object>();
		ext::lua::run( script );
	});
#endif
}
void uf::LuaBehavior::destroy( uf::Object& self ) {}
void uf::LuaBehavior::tick( uf::Object& self ) {}
void uf::LuaBehavior::render( uf::Object& self ) {}
void uf::LuaBehavior::Metadata::serialize( uf::Object& self, uf::Serializer& serializer ) {}
void uf::LuaBehavior::Metadata::deserialize( uf::Object& self, uf::Serializer& serializer ) {}
#undef this