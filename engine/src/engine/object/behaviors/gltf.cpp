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
	auto& metadata = this->getComponent<uf::Serializer>();
	this->addHook( "animation:Set.%UID%", [&](ext::json::Value& json){
		std::string name = json["name"].as<std::string>();

		if ( !this->hasComponent<pod::Graph>() ) return;
		auto& graph = this->getComponent<pod::Graph>();
		uf::graph::animate( graph, name );
	});
	this->addHook( "asset:Load.%UID%", [&](ext::json::Value& json){
		std::string filename = json["filename"].as<std::string>();
		std::string category = json["category"].as<std::string>();
		if ( category != "" && category != "models" ) return;
		if ( category == "" && uf::io::extension(filename) != "gltf" && uf::io::extension(filename) != "glb" && uf::io::extension(filename) != "graph" ) return;
		
		auto& scene = uf::scene::getCurrentScene();
		auto& assetLoader = scene.getComponent<uf::Asset>();

		{
			pod::Graph* graphPointer = NULL;
			if ( !assetLoader.has<pod::Graph>(filename) ) return;
			graphPointer = &assetLoader.get<pod::Graph>(filename);

			if ( !graphPointer ) return;
			auto& graph = this->getComponent<pod::Graph>();
			graph = std::move( *graphPointer );
			graphPointer = &graph;
		}
		auto& graph = this->getComponent<pod::Graph>();
		uf::Object* objectPointer = graph.root.entity;
		graph.root.entity->process([&]( uf::Entity* entity ) {
			if ( !entity->hasComponent<uf::Graphic>() ) return;
			auto& graphic = entity->getComponent<uf::Graphic>();
			if ( !(graph.mode & ext::gltf::LoadMode::LOAD) ) {
				if ( graph.mode & ext::gltf::LoadMode::SEPARATE ) {
					{
						std::string filename = "/gltf/vert.spv";
						if ( graph.mode & ext::gltf::LoadMode::SKINNED ) {
							filename = "/gltf/skinned.vert.spv";
						}
						if ( metadata["system"]["renderer"]["shaders"]["vertex"].is<std::string>() )
							filename = metadata["system"]["renderer"]["shaders"]["vertex"].as<std::string>();
						filename = this->grabURI( filename, metadata["system"]["root"].as<std::string>() );
						graphic.material.attachShader(filename, uf::renderer::enums::Shader::VERTEX);
					}
					{
						std::string filename = "/gltf/frag.spv";
						if ( metadata["system"]["renderer"]["shaders"]["fragment"].is<std::string>() ) 
							filename = metadata["system"]["renderer"]["shaders"]["fragment"].as<std::string>();
						filename = this->grabURI( filename, metadata["system"]["root"].as<std::string>() );
						graphic.material.attachShader(filename, uf::renderer::enums::Shader::FRAGMENT);
					}
				} else {
					{
						std::string filename = "/gltf/instanced.vert.spv";
						if ( graph.mode & ext::gltf::LoadMode::SKINNED ) {
							filename = "/gltf/skinned.instanced.vert.spv";
						}
						if ( metadata["system"]["renderer"]["shaders"]["vertex"].is<std::string>() )
							filename = metadata["system"]["renderer"]["shaders"]["vertex"].as<std::string>();
						filename = this->grabURI( filename, metadata["system"]["root"].as<std::string>() );
						graphic.material.attachShader(filename, uf::renderer::enums::Shader::VERTEX);
					}
					{
						std::string filename = "/gltf/frag.spv";
						if ( metadata["system"]["renderer"]["shaders"]["fragment"].is<std::string>() ) 
							filename = metadata["system"]["renderer"]["shaders"]["fragment"].as<std::string>();
						filename = this->grabURI( filename, metadata["system"]["root"].as<std::string>() );
						graphic.material.attachShader(filename, uf::renderer::enums::Shader::FRAGMENT);
					}
				}
			}
		#if UF_USE_VULKAN
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
		#endif
			graphic.process = true;
		});
		this->addChild(graph.root.entity->as<uf::Entity>());

		auto& transform = this->getComponent<pod::Transform<>>();
		graph.root.entity->getComponent<pod::Transform<>>().reference = &transform;
		
		graph.root.entity->initialize();
		graph.root.entity->process([&]( uf::Entity* entity ) {
			if ( !entity->hasComponent<uf::Graphic>() ) {
				if ( entity->getUid() == 0 ) entity->initialize();
				return;
			}
			uf::instantiator::bind( "GltfBehavior", *entity );
			uf::instantiator::unbind( "RenderBehavior", *entity );
			if ( entity->getUid() == 0 ) entity->initialize();
		});

		if ( graph.mode & ext::gltf::LoadMode::SKINNED ) {
			if ( metadata["model"]["animation"].is<std::string>() ) {
				uf::graph::animate( graph, metadata["model"]["animation"].as<std::string>() );
			}
			if ( metadata["model"]["print animations"].as<bool>() ) {
				uf::Serializer json = ext::json::array();
				for ( auto pair : graph.animations ) json.emplace_back( pair.first );
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
	/* Update animations */ if ( this->hasComponent<pod::Graph>() ) {
		auto& graph = this->getComponent<pod::Graph>();
		if ( graph.mode & ext::gltf::LoadMode::SKINNED ) uf::graph::update( graph );
	}
#if UF_USE_OPENGL
	/* Update uniforms */ if ( this->hasComponent<uf::Graphic>() ) {
		auto& scene = uf::scene::getCurrentScene();
		auto& metadata = this->getComponent<uf::Serializer>();
		auto& graphic = this->getComponent<uf::Graphic>();
		auto& controller = scene.getController();
		auto& camera = controller.getComponent<uf::Camera>();

		if ( !graphic.initialized ) return;
		if ( !graphic.material.hasShader("vertex") ) return;

		auto* objectWithGraph = this;
		while ( objectWithGraph != &scene ) {
			if ( objectWithGraph->hasComponent<pod::Graph>() ) break;
			objectWithGraph = &objectWithGraph->getParent().as<uf::Object>();
		}
		if ( !objectWithGraph->hasComponent<pod::Graph>() ) return;

		auto& shader = graphic.material.getShader("vertex");
		auto& graph = objectWithGraph->getComponent<pod::Graph>();
		auto& mesh = this->getComponent<ext::gltf::mesh_t>();

		if ( !(graph.mode & ext::gltf::LoadMode::SEPARATE) ) {
			std::vector<pod::Matrix4f> instances( graph.nodes.size() );
			for ( size_t i = 0; i < graph.nodes.size(); ++i ) {
				auto& node = graph.nodes[i];
				instances[i] = node.entity ? uf::transform::model( node.entity->getComponent<pod::Transform<>>() ) : uf::transform::model( node.transform );
			}

			graphic.updateBuffer( (void*) instances.data(), instances.size() * sizeof(pod::Matrix4f), graph.instanceBufferIndex );
			shader.execute( graphic, &mesh.vertices[0] );
		} else if ( graph.mode & ext::gltf::LoadMode::SKINNED ) {
			shader.execute( graphic, &mesh.vertices[0] );
		}
	}
#elif UF_USE_VULKAN
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
			graphic.updateBuffer( (void*) instances.data(), instances.size() * sizeof(pod::Matrix4f), graph.instanceBufferIndex /*storageBuffer*/ );
		}
	}
#endif
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

		auto* objectWithGraph = this;
		while ( objectWithGraph != &scene ) {
			if ( objectWithGraph->hasComponent<pod::Graph>() ) break;
			objectWithGraph = &objectWithGraph->getParent().as<uf::Object>();
		}
		if ( !objectWithGraph->hasComponent<pod::Graph>() ) return;
		auto& graph = objectWithGraph->getComponent<pod::Graph>();

#if UF_USE_OPENGL
		auto uniformBuffer = graphic.getUniform();
		pod::Uniform& uniform = *((pod::Uniform*) graphic.device->getBuffer(uniformBuffer.buffer));
		uniform.projection = camera.getProjection();
		if ( !(graph.mode & ext::gltf::LoadMode::SEPARATE) ) {
			uniform.modelView = camera.getView();
		} else {
			uniform.modelView = camera.getView() * uf::transform::model( transform );
		}
		graphic.updateUniform( (void*) &uniform, sizeof(uniform) );
#elif UF_USE_VULKAN
		if ( !graphic.material.hasShader("fragment") ) return;
		if ( !graphic.hasStorage("Materials") ) return;
		
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
#endif
	}
}
#undef this