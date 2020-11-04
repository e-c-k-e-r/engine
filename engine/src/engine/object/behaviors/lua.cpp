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


UF_BEHAVIOR_REGISTER_CPP(LuaBehavior)
#define this (&self)
void uf::LuaBehavior::initialize( uf::Object& self ) {	
	this->addHook( "asset:Load.%UID%", [&](const std::string& event)->std::string{	
		uf::Serializer json = event;
		std::string filename = json["filename"].asString();
		
		if ( uf::io::extension(filename) != "lua" ) return "false";
		uf::Scene& scene = uf::scene::getCurrentScene();
		uf::Asset& assetLoader = scene.getComponent<uf::Asset>();
		const pod::LuaScript* assetPointer = NULL;
		try { assetPointer = &assetLoader.get<pod::LuaScript>(filename); } catch ( ... ) {}
		if ( !assetPointer ) return "false";
		pod::LuaScript script = *assetPointer;
		script.header = "local ent = entities.get("+ std::to_string((uint) this->getUid()) +")";

		ext::lua::run( script );

		return "true";
	});
}
void uf::LuaBehavior::destroy( uf::Object& self ) {

}
void uf::LuaBehavior::tick( uf::Object& self ) {

}
void uf::LuaBehavior::render( uf::Object& self ) {

}
#undef this