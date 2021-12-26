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
		if ( !uf::Asset::isExpected( payload, uf::Asset::Type::LUA ) ) return;

		uf::Scene& scene = uf::scene::getCurrentScene();
		uf::Asset& assetLoader = scene.getComponent<uf::Asset>();
		const pod::LuaScript* assetPointer = NULL;
		if ( !assetLoader.has<pod::LuaScript>(payload.filename) ) return;
		assetPointer = &assetLoader.get<pod::LuaScript>(payload.filename);
		if ( !assetPointer ) return;
		pod::LuaScript script = *assetPointer;
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