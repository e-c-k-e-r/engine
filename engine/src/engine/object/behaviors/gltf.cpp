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
	metadata["textures"]["additional"] = ext::json::array(); //Json::Value(Json::arrayValue);
	// Default load: GLTF model
	this->addHook( "asset:Load.%UID%", [&](ext::json::Value& json){
		std::string filename = json["filename"].as<std::string>();
		if ( uf::io::extension(filename) != "png" ) return;
		auto& vector = metadata["textures"]["additional"];
		vector[vector.size()] = filename;
	});
	this->addHook( "animation:Set.%UID%", [&](ext::json::Value& json){
		std::string name = json["name"].as<std::string>();

		if ( !this->hasComponent<pod::Graph>() ) return;
		auto& graph = this->getComponent<pod::Graph>();
		uf::graph::animate( graph, name );
	});
	this->addHook( "asset:Load.%UID%", [&](ext::json::Value& json){
		std::string filename = json["filename"].as<std::string>();

		if ( uf::io::extension(filename) != "gltf" && uf::io::extension(filename) != "glb" ) return;
		
		auto& scene = uf::scene::getCurrentScene();
		auto& assetLoader = scene.getComponent<uf::Asset>();

		{
			pod::Graph* graphPointer = NULL;
			try { graphPointer = &assetLoader.get<pod::Graph>(filename); } catch ( ... ) {}
			if ( !graphPointer ) return;
			auto& graph = this->getComponent<pod::Graph>();
			graph = std::move( *graphPointer );
			graphPointer = &graph;
		}
		auto& graph = this->getComponent<pod::Graph>();
		uf::Object* objectPointer = graph.entity;
		objectPointer->process([&]( uf::Entity* entity ) {
			if ( !entity->hasComponent<uf::Graphic>() ) return;
			auto& graphic = entity->getComponent<uf::Graphic>();
			if ( !(graph.mode & ext::gltf::LoadMode::LOAD) ) {
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

			auto& shader = graphic.material.getShader("fragment");
			struct SpecializationConstant {
				uint32_t textures = 1;
			};
			auto* specializationConstants = (SpecializationConstant*) &shader.specializationConstants[0];
			specializationConstants->textures = graphic.material.textures.size();

			for ( auto& binding : shader.descriptorSetLayoutBindings ) {
				if ( binding.descriptorCount > 1 )
					binding.descriptorCount = specializationConstants->textures;
			}
			{
				auto& definition = shader.metadata["definitions"]["uniforms"]["UBO"];
				size_t size = definition["size"].as<size_t>();
				size_t elements = definition["value"].size();
				if ( elements > 0 ) {
					definition["size"] = size / elements * specializationConstants->textures;
					ext::json::Value value = definition["value"][0];
					definition["value"] = ext::json::array();
					for ( size_t i = 0; i < specializationConstants->textures; ++i ) {
						definition["value"].emplace_back(value);
					}
				}
			}
		});
		this->addChild(objectPointer->as<uf::Entity>());
		objectPointer->initialize();
		auto& transform = this->getComponent<pod::Transform<>>();
		objectPointer->process([&]( uf::Entity* entity ) {
			// entity->getComponent<pod::Transform<>>() = transform;
			if ( !entity->hasComponent<uf::Graphic>() ) {
				if ( entity->getUid() == 0 ) entity->initialize();
				return;
			}
			auto& eMetadata = entity->getComponent<uf::Serializer>();
			eMetadata["textures"]["map"] = metadata["textures"]["map"];
			uf::instantiator::bind( "RenderBehavior", *entity );
			uf::instantiator::bind( "GltfBehavior", *entity );
			if ( entity->getUid() == 0 ) entity->initialize();
		});

		if ( graph.mode & ext::gltf::LoadMode::SKINNED ) {
			if ( metadata["model"]["animation"].is<std::string>() ) {
				uf::graph::animate( graph, metadata["model"]["animation"].as<std::string>() );
			}
			if ( metadata["model"]["print animations"].as<bool>() ) {
				uf::Serializer json = ext::json::array(); //Json::Value(Json::arrayValue);
				for ( auto pair : graph.animations ) json.emplace_back( pair.first );
				uf::iostream << "Animations found: " << json << "\n";
			}
		}
	});
}
void uf::GltfBehavior::destroy( uf::Object& self ) {
	if ( this->hasComponent<pod::Graph>() ) {
		auto& graph = this->getComponent<pod::Graph>();
		uf::graph::destroy( graph );
	}
}
void uf::GltfBehavior::tick( uf::Object& self ) {
	/* Animation change test */ 
	if ( this->hasComponent<pod::Graph>() ) {
		std::vector<std::string> animations = { "wank","walk","sit_wank","run","idle_wank","sit","idle" };
		bool anyNumber =
			uf::Window::isKeyPressed("1") ||
			uf::Window::isKeyPressed("2") ||
			uf::Window::isKeyPressed("3") ||
			uf::Window::isKeyPressed("4") ||
			uf::Window::isKeyPressed("5") ||
			uf::Window::isKeyPressed("6") ||
			uf::Window::isKeyPressed("7") ||
			uf::Window::isKeyPressed("8") ||
			uf::Window::isKeyPressed("9") ||
			uf::Window::isKeyPressed("0");
		TIMER(1, anyNumber && ) {
			auto& graph = this->getComponent<pod::Graph>();
			std::string target = "";
			if ( uf::Window::isKeyPressed("1") && animations.size() >= 1 ) target = animations[0];
			else if ( uf::Window::isKeyPressed("2") && animations.size() >= 2 ) target = animations[1];
			else if ( uf::Window::isKeyPressed("3") && animations.size() >= 3 ) target = animations[2];
			else if ( uf::Window::isKeyPressed("4") && animations.size() >= 4 ) target = animations[3];
			else if ( uf::Window::isKeyPressed("5") && animations.size() >= 5 ) target = animations[4];
			else if ( uf::Window::isKeyPressed("6") && animations.size() >= 6 ) target = animations[5];
			else if ( uf::Window::isKeyPressed("7") && animations.size() >= 7 ) target = animations[6];
			else if ( uf::Window::isKeyPressed("8") && animations.size() >= 8 ) target = animations[7];
			else if ( uf::Window::isKeyPressed("9") && animations.size() >= 9 ) target = animations[8];
			else if ( uf::Window::isKeyPressed("0") && animations.size() >= 10 ) target = animations[9];
			std::cout << "CHANGING ANIMATION TO " << target << std::endl;
			// graph.animationSettings.loop = false;
			uf::graph::animate( graph, target, 1 / 0.125f );
		}
	}

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
		auto& scene = uf::scene::getCurrentScene();
		auto& metadata = this->getComponent<uf::Serializer>();
		auto& graphic = this->getComponent<uf::Graphic>();

		if ( !graphic.initialized ) return;
		if ( !graphic.material.hasShader("fragment") ) return;
		if ( !graphic.hasStorage("Materials") ) return;
		if ( !ext::json::isObject(metadata["textures"]["map"]) ) return;

		auto* objectWithGraph = this;
		while ( objectWithGraph != &scene ) {
			if ( objectWithGraph->hasComponent<pod::Graph>() ) break;
			objectWithGraph = &objectWithGraph->getParent().as<uf::Object>();
		}
		if ( !objectWithGraph->hasComponent<pod::Graph>() ) return;
		auto& graph = objectWithGraph->getComponent<pod::Graph>();
		if ( graph.materials.empty() ) return;
		

		auto& shader = graphic.material.getShader("fragment");
		std::vector<pod::Material::Storage> materials( graph.materials.size() );
		for ( size_t i = 0; i < graph.materials.size(); ++i ) {
			materials[i] = graph.materials[i].storage;
			materials[i].indexMappedTarget = graph.mode & ext::gltf::LoadMode::ATLAS ? 0 : i;
			materials[i].factorMappedBlend = graph.mode & ext::gltf::LoadMode::ATLAS ? 1.0f : 0.0f;
		}
		size_t texturesLen = graphic.material.textures.size();
		ext::json::forEach(metadata["textures"]["map"], [&]( const std::string& key, const ext::json::Value& mapping ){
			uint32_t from = std::stoi(key);
			uint32_t to = mapping[0].as<size_t>();
			float blend = 1.0f;
			if ( mapping[1].as<std::string>() == "sin(time)" ) {
				blend = sin(uf::physics::time::current)*0.5f+0.5f;
			} else if ( mapping[1].as<std::string>() == "cos(time)" ) {
				blend = cos(uf::physics::time::current)*0.5f+0.5f;
			} else if ( mapping[1].is<float>() ) {
				blend = mapping[1].as<float>();
			}
			if ( from >= texturesLen || to >= texturesLen ) return;
			materials[from].indexMappedTarget = to;
			materials[from].factorMappedBlend = blend;
		});
		auto& storageBuffer = *graphic.getStorageBuffer("Materials");
		graphic.updateBuffer( (void*) materials.data(), materials.size() * sizeof(pod::Material::Storage), storageBuffer, false );
	/*
		struct UniformDescriptor {
			struct Mapping {
				alignas(4) uint32_t target;
				alignas(4) float blend;
				uint32_t padding[2];
			} map;
		};
		auto& uniform = shader.getUniform("UBO");

		uint8_t* uniformBuffer = (uint8_t*) (void*) uniform;
		UniformDescriptor* uniforms = (UniformDescriptor*) uniformBuffer;
		UniformDescriptor::Mapping* mappings = (UniformDescriptor::Mapping*) &(uniformBuffer)[sizeof(UniformDescriptor) - sizeof(UniformDescriptor::Mapping)];

		size_t textures = graphic.material.textures.size();
		for ( size_t i = 0; i < textures; ++i ) {
			mappings[i].target = i;
			mappings[i].blend = 0.0f;
		}
		for ( auto it = metadata["textures"]["map"].begin(); it != metadata["textures"]["map"].end(); ++it ) {
			std::string key = it.key();
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
		shader.updateUniform( "UBO" );
	*/
	}
}
void uf::GltfBehavior::render( uf::Object& self ) {

}
#undef this