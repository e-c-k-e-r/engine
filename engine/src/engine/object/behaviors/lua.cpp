#include <uf/engine/object/behaviors/lua.h>

#include <uf/engine/object/object.h>
#include <uf/engine/asset/asset.h>
#include <uf/engine/scene/scene.h>
#include <uf/utils/time/time.h>
#include <uf/utils/math/transform.h>
#include <uf/utils/math/physics.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/graphic/mesh.h>
#include <uf/engine/asset/asset.h>
#include <uf/ext/lua/lua.h>

UF_BEHAVIOR_REGISTER_CPP(uf::LuaBehavior)
UF_BEHAVIOR_TRAITS_CPP(uf::LuaBehavior, ticks = false, renders = false, multithread = false)
#define this (&self)
void uf::LuaBehavior::initialize( uf::Object& self ) {	
#if UF_USE_LUA
	this->addHook( "asset:Load.%UID%", [&](ext::json::Value& json){
		uf::stl::string filename = json["filename"].as<uf::stl::string>();
		uf::stl::string category = json["category"].as<uf::stl::string>();
		if ( category != "" && category != "scripts" ) return;
		if ( category == "" && uf::io::extension(filename) != "lua" ) return;
		uf::Scene& scene = uf::scene::getCurrentScene();
		uf::Asset& assetLoader = scene.getComponent<uf::Asset>();
		const pod::LuaScript* assetPointer = NULL;
		if ( !assetLoader.has<pod::LuaScript>(filename) ) return;
		assetPointer = &assetLoader.get<pod::LuaScript>(filename);
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
#undef this