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
		
		uf::Scene& scene = uf::scene::getCurrentScene();
		uf::Asset& assetLoader = scene.getComponent<uf::Asset>();
		uf::Object* objectPointer = NULL;
		try { objectPointer = assetLoader.get<uf::Object*>(filename); } catch ( ... ) {}
		if ( !objectPointer ) return "false";

		std::function<void(uf::Entity*)> filter = [&]( uf::Entity* entity ) {
			if ( !entity->hasComponent<uf::Graphic>() || !entity->hasComponent<ext::gltf::mesh_t>() ) return;
			auto& graphic = entity->getComponent<uf::Graphic>();
			auto& mesh = entity->getComponent<ext::gltf::mesh_t>();
			graphic.initialize();
			graphic.initializeGeometry( mesh );

			graphic.material.attachShader("./data/shaders/gltf.stereo.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
			graphic.material.attachShader("./data/shaders/gltf.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

			graphic.process = true;

			auto& shader = graphic.material.shaders.back();
			struct SpecializationConstant {
				uint32_t textures = 1;
			};
			auto* specializationConstants = (SpecializationConstant*) &shader.specializationConstants[0];
			specializationConstants->textures = graphic.material.textures.size();

			for ( auto& binding : shader.descriptorSetLayoutBindings ) {
				if ( binding.descriptorCount > 1 )
					binding.descriptorCount = specializationConstants->textures;
			}
		};
		objectPointer->process(filter);

		this->addChild(objectPointer->as<uf::Entity>());
		objectPointer->initialize();

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