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
		uf::Object* objectPointer = graph.root.entity;
		objectPointer->process([&]( uf::Entity* entity ) {
			if ( !entity->hasComponent<uf::Graphic>() ) return;
			auto& graphic = entity->getComponent<uf::Graphic>();
			if ( !(graph.mode & ext::gltf::LoadMode::LOAD) ) {
				if ( graph.mode & ext::gltf::LoadMode::SEPARATE ) {
					{
						std::string filename = "/gltf.vert.spv";
						if ( graph.mode & ext::gltf::LoadMode::SKINNED ) {
							filename = "/gltf.skinned.vert.spv";
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
				} else {
					{
						std::string filename = "/gltf.instanced.vert.spv";
						if ( graph.mode & ext::gltf::LoadMode::SKINNED ) {
							filename = "/gltf.skinned.instanced.vert.spv";
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

			{
				auto& shader = graphic.material.getShader("vertex");
				struct SpecializationConstant {
					uint32_t passes = 6;
				};
				auto& specializationConstants = shader.specializationConstants.get<SpecializationConstant>();
				specializationConstants.passes = uf::renderer::settings::maxViews;

			
				ext::json::forEach( shader.metadata["specializationConstants"], [&]( ext::json::Value& sc ){
					std::string name = sc["name"].as<std::string>();
					if ( name == "PASSES" ) {
						sc["value"] = specializationConstants.passes;
					}
				});
			
			}
			{
				auto& shader = graphic.material.getShader("fragment");
				struct SpecializationConstant {
					uint32_t textures = 1;
				};
				auto& specializationConstants = shader.specializationConstants.get<SpecializationConstant>();
				specializationConstants.textures = graphic.material.textures.size();
			
				ext::json::forEach( shader.metadata["specializationConstants"], [&]( ext::json::Value& sc ){
					std::string name = sc["name"].as<std::string>();
					if ( name == "TEXTURES" ) {
						sc["value"] = specializationConstants.textures;
					}
				});
			
				for ( auto& binding : shader.descriptorSetLayoutBindings ) {
					if ( binding.descriptorCount > 1 )
						binding.descriptorCount = specializationConstants.textures;
				}
				{
					auto& definition = shader.metadata["definitions"]["uniforms"]["UBO"];
					size_t size = definition["size"].as<size_t>();
					size_t elements = definition["value"].size();
					if ( elements > 0 ) {
						definition["size"] = size / elements * specializationConstants.textures;
						ext::json::Value value = definition["value"][0];
						definition["value"] = ext::json::array();
						for ( size_t i = 0; i < specializationConstants.textures; ++i ) {
							definition["value"].emplace_back(value);
						}
					}
				}
			}
		});
		this->addChild(objectPointer->as<uf::Entity>());

		auto& transform = this->getComponent<pod::Transform<>>();
		objectPointer->getComponent<pod::Transform<>>().reference = &transform;
		
		objectPointer->initialize();
		objectPointer->process([&]( uf::Entity* entity ) {
			if ( !entity->hasComponent<uf::Graphic>() ) {
				if ( entity->getUid() == 0 ) entity->initialize();
				return;
			}
			auto& eMetadata = entity->getComponent<uf::Serializer>();
			eMetadata["textures"]["map"] = metadata["textures"]["map"];
		//	if ( graph.mode & ext::gltf::LoadMode::SEPARATE ) uf::instantiator::bind( "RenderBehavior", *entity );
			uf::instantiator::bind( "GltfBehavior", *entity );
			uf::instantiator::unbind( "RenderBehavior", *entity );
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
	/*
		auto& transform = this->getComponent<pod::Transform<>>();
		auto& node = graph.node->children.size() == 1 ? *graph.node->children[0] : *graph.node;
		pod::Matrix4f nodeMatrix = node.transform.model;
		pod::Node* currentParent = uf::graph::find(graph, node.parent);
		while ( currentParent ) {
			nodeMatrix = currentParent->transform.model * nodeMatrix;
			currentParent = uf::graph::find(graph, currentParent->parent);
		}
		transform.model = nodeMatrix;
	*/
	}
	/* Update uniforms */ if ( this->hasComponent<uf::Graphic>() ) {
		auto& scene = uf::scene::getCurrentScene();
		auto& metadata = this->getComponent<uf::Serializer>();
		auto& graphic = this->getComponent<uf::Graphic>();
		auto& controller = scene.getController();
		auto& camera = controller.getComponent<uf::Camera>();

		if ( !graphic.initialized ) return;
		if ( !graphic.material.hasShader("fragment") ) return;

		auto* objectWithGraph = this;
		while ( objectWithGraph != &scene ) {
			if ( objectWithGraph->hasComponent<pod::Graph>() ) break;
			objectWithGraph = &objectWithGraph->getParent().as<uf::Object>();
		}
		if ( !objectWithGraph->hasComponent<pod::Graph>() ) return;

		auto& graph = objectWithGraph->getComponent<pod::Graph>();
		if ( !(graph.mode & ext::gltf::LoadMode::SEPARATE) ) {
			auto& shader = graphic.material.getShader("vertex");
			std::vector<pod::Matrix4f> instances( graph.nodes.size() );
			for ( size_t i = 0; i < graph.nodes.size(); ++i ) {
				auto& node = graph.nodes[i];
				instances[i] = node.entity ? uf::transform::model( node.entity->getComponent<pod::Transform<>>() ) : uf::transform::model( node.transform );
			}
			auto& storageBuffer = *graphic.getStorageBuffer("Models");
			graphic.updateBuffer( (void*) instances.data(), instances.size() * sizeof(pod::Matrix4f), graph.instanceBufferIndex /*storageBuffer*/, false );
		}

		if ( graph.materials.empty() && graphic.hasStorage("Materials") ) {
			auto& shader = graphic.material.getShader("fragment");
			std::vector<pod::Material::Storage> materials( graph.materials.size() );
			for ( size_t i = 0; i < graph.materials.size(); ++i ) {
				materials[i] = graph.materials[i].storage;
				materials[i].indexMappedTarget = graph.mode & ext::gltf::LoadMode::ATLAS ? 0 : i;
				materials[i].factorMappedBlend = graph.mode & ext::gltf::LoadMode::ATLAS ? 1.0f : 0.0f;
			}
			{
				size_t texturesLen = graphic.material.textures.size();
				ext::json::forEach(metadata["textures"]["map"], [&]( const std::string& key, const ext::json::Value& mapping ){
					uint32_t from = std::stoi(key);
					if ( mapping["alpha cutoff"].is<float>() ) {
						materials[from].factorAlphaCutoff = mapping["alpha cutoff"].as<float>();
					}
					if ( mapping["to"].is<size_t>() ) {
						uint32_t to = mapping["to"].as<size_t>();
						float blend = 1.0f;
						if ( mapping["factor"].as<std::string>() == "sin(time)" ) {
							blend = sin(uf::physics::time::current)*0.5f+0.5f;
						} else if ( mapping["factor"].as<std::string>() == "cos(time)" ) {
							blend = cos(uf::physics::time::current)*0.5f+0.5f;
						} else if ( mapping["factor"].is<float>() ) {
							blend = mapping["factor"].as<float>();
						}
						if ( from >= texturesLen || to >= texturesLen ) return;
						materials[from].indexMappedTarget = to;
						materials[from].factorMappedBlend = blend;
					}
				});
			}
			auto& storageBuffer = *graphic.getStorageBuffer("Materials");
			graphic.updateBuffer( (void*) materials.data(), materials.size() * sizeof(pod::Material::Storage), graph.root.materialBufferIndex /*storageBuffer*/, false );
		}
	}
}

namespace {
	template<size_t N = uf::renderer::settings::maxViews>
	struct UniformDescriptor {
		alignas(16) pod::Matrix4f view[N];
		alignas(16) pod::Matrix4f projection[N];
	};
}

void uf::GltfBehavior::render( uf::Object& self ) {
	/* Update uniforms */ if ( this->hasComponent<uf::Graphic>() ) {
		auto& scene = uf::scene::getCurrentScene();
		auto& metadata = this->getComponent<uf::Serializer>();
		auto& graphic = this->getComponent<uf::Graphic>();
		auto& controller = scene.getController();
		auto& camera = controller.getComponent<uf::Camera>();
		auto& transform = this->getComponent<pod::Transform<>>();

		if ( !graphic.initialized ) return;
		if ( !graphic.material.hasShader("fragment") ) return;
		if ( !graphic.hasStorage("Materials") ) return;

		auto* objectWithGraph = this;
		while ( objectWithGraph != &scene ) {
			if ( objectWithGraph->hasComponent<pod::Graph>() ) break;
			objectWithGraph = &objectWithGraph->getParent().as<uf::Object>();
		}
		if ( !objectWithGraph->hasComponent<pod::Graph>() ) return;
		auto& graph = objectWithGraph->getComponent<pod::Graph>();
		if ( graph.materials.empty() ) return;
		
		// std::cout << "START" << std::endl;
		auto& shader = graphic.material.getShader("vertex");
		auto& uniform = shader.getUniform("UBO");
		if ( !(graph.mode & ext::gltf::LoadMode::SEPARATE) ) {
		#if UF_UNIFORMS_UPDATE_WITH_JSON
		//	auto uniforms = shader.getUniformJson("UBO");
			ext::json::Value uniforms;
			for ( std::size_t i = 0; i < uf::renderer::settings::maxViews; ++i ) {
				uniforms["view"][i] = uf::matrix::encode( camera.getView( i ) );
				uniforms["projection"][i] = uf::matrix::encode( camera.getProjection( i ) );
			}
			shader.updateUniform("UBO", uniforms );
		#else
			auto& uniforms = uniform.get<UniformDescriptor<>>();
			for ( std::size_t i = 0; i < uf::renderer::settings::maxViews; ++i ) {
				uniforms.view[i] = camera.getView( i );
				uniforms.projection[i] = camera.getProjection( i );
			}
			shader.updateUniform( "UBO", uniform );
		#endif
		} else {
		#if UF_UNIFORMS_UPDATE_WITH_JSON
		//	auto uniforms = shader.getUniformJson("UBO");
			ext::json::Value uniforms;
			uniforms["matrices"]["model"] = uf::matrix::encode( uf::transform::model( transform ) );
			for ( std::size_t i = 0; i < uf::renderer::settings::maxViews; ++i ) {
				uniforms["matrices"]["view"][i] = uf::matrix::encode( camera.getView( i ) );
				uniforms["matrices"]["projection"][i] = uf::matrix::encode( camera.getProjection( i ) );
			}
			if ( ext::json::isArray(metadata["color"]) ) {
				uniforms["color"][0] = metadata["color"][0];
				uniforms["color"][1] = metadata["color"][1];
				uniforms["color"][2] = metadata["color"][2];
				uniforms["color"][3] = metadata["color"][3];
			} else {
				uniforms["color"][0] = 1.0f;
				uniforms["color"][1] = 1.0f;
				uniforms["color"][2] = 1.0f;
				uniforms["color"][3] = 1.0f;
			}
			shader.updateUniform("UBO", uniforms );
		#else
			auto& uniforms = uniform.get<uf::MeshDescriptor<>>();
			uniforms.matrices.model = uf::transform::model( transform );
			for ( std::size_t i = 0; i < uf::renderer::settings::maxViews; ++i ) {
				uniforms.matrices.view[i] = camera.getView( i );
				uniforms.matrices.projection[i] = camera.getProjection( i );
			}
			if ( ext::json::isArray(metadata["color"]) ) {
				uniforms.color[0] = metadata["color"][0].as<float>();
				uniforms.color[1] = metadata["color"][1].as<float>();
				uniforms.color[2] = metadata["color"][2].as<float>();
				uniforms.color[3] = metadata["color"][3].as<float>();
			} else {
				uniforms.color = { 1, 1, 1, 1 };
			}
			shader.updateUniform( "UBO", uniform );
		#endif
		}
	}
}
#undef this