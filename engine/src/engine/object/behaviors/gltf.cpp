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
	this->addHook( "animation:Set.%UID%", [&](const std::string& event)->std::string{	
		uf::Serializer json = event;
		std::string name = json["name"].as<std::string>();

		if ( !this->hasComponent<pod::Graph>() ) return "false";
		auto& graph = this->getComponent<pod::Graph>();
		uf::graph::animate( graph, name );
		return "true";
	});
	this->addHook( "asset:Load.%UID%", [&](const std::string& event)->std::string{	
		uf::Serializer json = event;
		std::string filename = json["filename"].as<std::string>();

		if ( uf::io::extension(filename) != "gltf" && uf::io::extension(filename) != "glb" ) return "false";
		
		auto& scene = uf::scene::getCurrentScene();
		auto& assetLoader = scene.getComponent<uf::Asset>();

		{
			pod::Graph* graphPointer = NULL;
			try { graphPointer = &assetLoader.get<pod::Graph>(filename); } catch ( ... ) {}
			if ( !graphPointer ) return "false";
			auto& graph = this->getComponent<pod::Graph>();
			graph = std::move( *graphPointer );
			graphPointer = &graph;
		}
		auto& graph = this->getComponent<pod::Graph>();
		uf::Object* objectPointer = graph.entity;
		objectPointer->process([&]( uf::Entity* entity ) {
			if ( !entity->hasComponent<uf::Graphic>() ) return;
			auto& graphic = entity->getComponent<uf::Graphic>();
			if ( !(graph.mode & ext::gltf::LoadMode::DEFAULT_LOAD) ) {
				{
					std::string filename = "/gltf.stereo.vert.spv";
					if ( graph.mode & ext::gltf::LoadMode::SKINNED ) {
						filename = "/gltf.stereo.skinned.vert.spv";
					}
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
		auto& transform = this->getComponent<pod::Transform<>>();
		objectPointer->process([&]( uf::Entity* entity ) {
			{
				entity->getComponent<pod::Transform<>>() = transform;
			}
			if ( !entity->hasComponent<uf::Graphic>() || !entity->hasComponent<ext::gltf::mesh_t>() ) return;
			auto& eMetadata = entity->getComponent<uf::Serializer>();
			eMetadata["textures"]["map"] = metadata["textures"]["map"];
			uf::instantiator::bind( "GltfBehavior", *entity );
		});

		if ( graph.mode & ext::gltf::LoadMode::SKINNED ) {
			if ( metadata["model"]["animation"].is<std::string>() ) {
				uf::graph::animate( graph, metadata["model"]["animation"].as<std::string>() );
			}
			if ( metadata["model"]["print animations"].as<bool>() ) {
				uf::Serializer json = Json::Value(Json::arrayValue);
				for ( auto pair : graph.animations ) json.append( pair.first );
				uf::iostream << "Animations found: " << json << "\n";
			}
		}

		return "true";
	});
}
void uf::GltfBehavior::destroy( uf::Object& self ) {
	if ( this->hasComponent<pod::Graph>() ) {
		auto& graph = this->getComponent<pod::Graph>();
		uf::graph::destroy( graph );
	}
}
void uf::GltfBehavior::tick( uf::Object& self ) {
	/* Update animations */ if ( this->hasComponent<pod::Graph>() ) {
		auto& graph = this->getComponent<pod::Graph>();
		if ( graph.mode & ext::gltf::LoadMode::SKINNED ) {
			uf::graph::update( graph );
		}

		auto& transform = this->getComponent<pod::Transform<>>();
		auto& node = graph.node->children.size() == 1 ? *graph.node->children[0] : *graph.node;
		pod::Matrix4f nodeMatrix = node.transform.model;
		pod::Node*currentParent = node.parent;
		while ( currentParent ) {
			nodeMatrix = currentParent->transform.model * nodeMatrix;
			currentParent = currentParent->parent;
		}
		transform.model = nodeMatrix;
	}
	/* Update uniforms */ if ( this->hasComponent<uf::Graphic>() ) {
		auto& metadata = this->getComponent<uf::Serializer>();
		auto& graphic = this->getComponent<uf::Graphic>();	
		if ( !graphic.initialized ) return;
		auto& shader = graphic.material.shaders.back();
		if ( shader.uniforms.empty() ) return;
		shader.validate();

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
		uf::renderer::Buffer* bufferPointer = NULL;
		for ( auto& buffer : shader.buffers ) {
			if ( buffer.usageFlags & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT ) {
				 bufferPointer = &buffer;
			}
		}
		if ( bufferPointer ) shader.updateBuffer( (void*) uniforms_buffer, uniforms_len, *bufferPointer, false );
	};
}
void uf::GltfBehavior::render( uf::Object& self ) {

}
#undef this