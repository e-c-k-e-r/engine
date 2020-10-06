#include <uf/engine/object/behaviors/gltf.h>

#include <uf/engine/object/object.h>
#include <uf/engine/asset/asset.h>
#include <uf/engine/scene/scene.h>
#include <uf/utils/time/time.h>
#include <uf/utils/math/transform.h>
#include <uf/utils/math/physics.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/thread/thread.h>
#include <uf/utils/graphic/mesh.h>
#include <uf/ext/gltf/gltf.h>

UF_BEHAVIOR_REGISTER_CPP(GltfBehavior)
#define this (&self)
void uf::GltfBehavior::initialize( uf::Object& self ) {	
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	// Default load: GLTF model

	this->addHook( "asset:Load.%UID%", [&](const std::string& event)->std::string{	
		uf::Serializer json = event;
		std::string filename = json["filename"].asString();

		if ( uf::io::extension(filename) != "gltf" && uf::io::extension(filename) != "glb" ) return "false";
		int8_t LOAD_FLAGS = 0;
		if ( metadata["model"]["flags"]["GENERATE_NORMALS"].asBool() )
			LOAD_FLAGS |= ext::gltf::LoadMode::GENERATE_NORMALS; 	// 0x1 << 0;
		if ( metadata["model"]["flags"]["APPLY_TRANSFORMS"].asBool() )
			LOAD_FLAGS |= ext::gltf::LoadMode::APPLY_TRANSFORMS; 	// 0x1 << 1;
		if ( metadata["model"]["flags"]["SEPARATE_MESHES"].asBool() )
			LOAD_FLAGS |= ext::gltf::LoadMode::SEPARATE_MESHES; 	// 0x1 << 2;
		if ( metadata["model"]["flags"]["RENDER"].asBool() )
			LOAD_FLAGS |= ext::gltf::LoadMode::RENDER; 				// 0x1 << 3;
		if ( metadata["model"]["flags"]["COLLISION"].asBool() )
			LOAD_FLAGS |= ext::gltf::LoadMode::COLLISION; 			// 0x1 << 4;
		if ( metadata["model"]["flags"]["AABB"].asBool() )
			LOAD_FLAGS |= ext::gltf::LoadMode::AABB; 				// 0x1 << 5;
		if ( metadata["model"]["flags"]["THREADED"].asBool() )
			LOAD_FLAGS |= ext::gltf::LoadMode::THREADED; 			// 0x1 << 6;

		ext::gltf::load( *this, filename, LOAD_FLAGS );
		
		return "true";
	});
}
void uf::GltfBehavior::destroy( uf::Object& self ) {

}
void uf::GltfBehavior::tick( uf::Object& self ) {

}
void uf::GltfBehavior::render( uf::Object& self ) {

}
#undef this