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
#include <uf/utils/string/hash.h>

UF_BEHAVIOR_REGISTER_CPP(uf::GltfBehavior)
#define this (&self)
void uf::GltfBehavior::initialize( uf::Object& self ) {	
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	metadata["textures"]["additional"] = Json::Value(Json::arrayValue);
	// Default load: GLTF model
	this->addHook( "asset:Load.%UID%", [&](const std::string& event)->std::string{	
		uf::Serializer json = event;
		std::string filename = json["filename"].as<std::string>();
		if ( uf::io::extension(filename) != "png" ) return "false";
		auto& vector = metadata["textures"]["additional"];
		vector[vector.size()] = filename;
		return "true";
	});
	this->addHook( "asset:Load.%UID%", [&](const std::string& event)->std::string{	
		uf::Serializer json = event;
		std::string filename = json["filename"].as<std::string>();

		if ( uf::io::extension(filename) != "gltf" && uf::io::extension(filename) != "glb" ) return "false";
		
		uf::Scene& scene = uf::scene::getCurrentScene();
		uf::Asset& assetLoader = scene.getComponent<uf::Asset>();
		uf::Object* objectPointer = NULL;
		try { objectPointer = assetLoader.get<uf::Object*>(filename); } catch ( ... ) {}
		if ( !objectPointer ) return "false";
		objectPointer->process([&]( uf::Entity* entity ) {
			if ( !entity->hasComponent<uf::Graphic>() || !entity->hasComponent<ext::gltf::mesh_t>() ) return;
			auto& graphic = entity->getComponent<uf::Graphic>();
			auto& mesh = entity->getComponent<ext::gltf::mesh_t>();
			graphic.initialize();
			graphic.initializeGeometry( mesh );

			{
				std::string filename = "/gltf.stereo.vert.spv";
				if ( metadata["system"]["renderer"]["shaders"]["vertex"].is<std::string>() )
					filename = metadata["system"]["renderer"]["shaders"]["vertex"].as<std::string>();
				filename = this->grabURI( filename, metadata["system"]["root"].as<std::string>() );
				graphic.material.attachShader(filename, VK_SHADER_STAGE_VERTEX_BIT);
			}
			{
				std::string filename = "/gltf.frag.spv";
				if ( metadata["system"]["renderer"]["shaders"]["fragment"].is<std::string>() ) 
					filename = metadata["system"]["renderer"]["shaders"]["fragment"].as<std::string>();
				filename = this->grabURI( filename, metadata["system"]["root"].as<std::string>() );
				graphic.material.attachShader(filename, VK_SHADER_STAGE_FRAGMENT_BIT);
			}

			for ( int i = 0; i < metadata["textures"]["additional"].size(); ++i ) {
				std::string filename = metadata["textures"]["additional"][i].as<std::string>();
				auto& scene = uf::scene::getCurrentScene();
				auto& assetLoader = scene.getComponent<uf::Asset>();
				const uf::Image* imagePointer = NULL;
				try { imagePointer = &assetLoader.get<uf::Image>(filename); } catch ( ... ) {}
				if ( !imagePointer ) continue;
				uf::Image image = *imagePointer;
				auto& texture = graphic.material.textures.emplace_back();
				texture.loadFromImage( image );
			}

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
		});
		this->addChild(objectPointer->as<uf::Entity>());
		objectPointer->initialize();
		objectPointer->process([&]( uf::Entity* entity ) {
			if ( !entity->hasComponent<uf::Graphic>() || !entity->hasComponent<ext::gltf::mesh_t>() ) return;
			auto& eMetadata = entity->getComponent<uf::Serializer>();
			eMetadata["textures"]["map"] = metadata["textures"]["map"];
			uf::instantiator::bind( "GltfBehavior", *entity );
		});
		return "true";
	});
}
void uf::GltfBehavior::destroy( uf::Object& self ) {

}
void uf::GltfBehavior::tick( uf::Object& self ) {
	/* Update uniforms */ if ( this->hasComponent<uf::Graphic>() ) {
		auto& metadata = this->getComponent<uf::Serializer>();
		auto& graphic = this->getComponent<uf::Graphic>();	
		if ( !graphic.initialized ) return;
		auto& shader = graphic.material.shaders.back();
		if ( shader.uniforms.empty() ) return;
		auto& userdata = shader.uniforms.front();
		struct UniformDescriptor {
		//	alignas(4) uint32_t mappings;
			struct Mapping {
				alignas(4) uint32_t target;
				alignas(4) float blend;
				uint32_t padding[2];
			} map;
		};
		size_t uniforms_len = userdata.data().len;
		uint8_t* uniforms_buffer = (uint8_t*) (void*) userdata;
		UniformDescriptor* uniforms = (UniformDescriptor*) uniforms_buffer;
		UniformDescriptor::Mapping* mappings = (UniformDescriptor::Mapping*) &uniforms_buffer[sizeof(UniformDescriptor) - sizeof(UniformDescriptor::Mapping)];

		size_t textures = graphic.material.textures.size();
		// uniforms->mappings = textures;
		for ( size_t i = 0; i < textures; ++i ) {
			mappings[i].target = i;
			mappings[i].blend = 0.0f;
		}
		for ( auto it = metadata["textures"]["map"].begin(); it != metadata["textures"]["map"].end(); ++it ) {
			std::string key = it.key().as<std::string>();
			uint32_t from = std::stoi(key);
			uint32_t to = metadata["textures"]["map"][key][0].as<size_t>();
			float blend = 1.0f;
			if ( metadata["textures"]["map"][key][1].as<std::string>() == "sin(time)" ) {
				blend = sin(uf::physics::time::current)*0.5f+0.5f;
			} else if ( metadata["textures"]["map"][key][1].as<std::string>() == "cos(time)" ) {
				blend = cos(uf::physics::time::current)*0.5f+0.5f;
			} else if ( metadata["textures"]["map"][key][1].is<double>() ) {
				blend = metadata["textures"]["map"][key][1].as<float>();
			}
			if ( from >= textures || to >= textures ) continue;
			mappings[from].target = to;
			mappings[from].blend = blend;
		}
		shader.updateBuffer( (void*) uniforms_buffer, uniforms_len, 0, false );
	};
}
void uf::GltfBehavior::render( uf::Object& self ) {

}
#undef this