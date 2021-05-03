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
		if ( !assetLoader.has<pod::Graph>(filename) ) return;
		pod::Graph* graphPointer = &assetLoader.get<pod::Graph>(filename);
		auto& graph = this->getComponent<pod::Graph>();
		graph = std::move( *graphPointer );
		assetLoader.remove<pod::Graph>(filename);

		bool shouldUpdate = false;	

		auto& sceneMetadataJson = scene.getComponent<uf::Serializer>();
		if ( !ext::json::isNull(graph.metadata["ambient"]) ) {
			sceneMetadataJson["light"]["ambient"] = graph.metadata["ambient"];
			shouldUpdate = true;	
		}
		if ( !ext::json::isNull(graph.metadata["fog"]) ) {
			sceneMetadataJson["light"]["fog"] = graph.metadata["fog"];
			shouldUpdate = true;	
		}
		if ( shouldUpdate ) scene.callHook("object:UpdateMetadata.%UID%");

		// deferred shader loading
		auto& transform = this->getComponent<pod::Transform<>>();
		graph.root.entity->getComponent<pod::Transform<>>().reference = &transform;

		uf::graph::initialize( graph );

		if ( graph.metadata["flags"]["SKINNED"].as<bool>() ) {
			if ( metadata["model"]["animation"].is<std::string>() ) {
				uf::graph::animate( graph, metadata["model"]["animation"].as<std::string>() );
			}
		/*
			if ( metadata["model"]["print animations"].as<bool>() ) {
				uf::Serializer json = ext::json::array();
				for ( auto pair : graph.animations ) json.emplace_back( pair.first );
			}
		*/
		}

		this->addChild(graph.root.entity->as<uf::Entity>());
	});
}
void uf::GltfBehavior::destroy( uf::Object& self ) {
	if ( this->hasComponent<pod::Graph>() ) {
		auto& graph = this->getComponent<pod::Graph>();
		uf::graph::destroy( graph );
	}
}

#define UNIFORMS_UPDATE_IN_TICK 1
void uf::GltfBehavior::tick( uf::Object& self ) {
	/* Update animations */ if ( this->hasComponent<pod::Graph>() ) {
		auto& graph = this->getComponent<pod::Graph>();
		if ( graph.metadata["flags"]["SKINNED"].as<bool>() ) uf::graph::update( graph );
	}

	if ( !this->hasComponent<uf::Graphic>() ) return;

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

#if UNIFORMS_UPDATE_IN_TICK
#if UF_USE_OPENGL
	if ( graphic.material.hasShader("vertex") ) {
		auto& shader = graphic.material.getShader("vertex");
		auto& mesh = this->getComponent<ext::gltf::mesh_t>();

		if ( !(graph.metadata["flags"]["SEPARATE"].as<bool>()) ) {
			std::vector<pod::Matrix4f> instances( graph.nodes.size() );
			for ( size_t i = 0; i < graph.nodes.size(); ++i ) {
				auto& node = graph.nodes[i];
				instances[i] = node.entity ? uf::transform::model( node.entity->getComponent<pod::Transform<>>() ) : uf::transform::model( node.transform );
			}

			graphic.updateBuffer( (void*) instances.data(), instances.size() * sizeof(pod::Matrix4f), graph.instanceBufferIndex );
			shader.execute( graphic, &mesh.vertices[0] );
		} else if ( graph.metadata["flags"]["SKINNED"].as<bool>() ) {
			shader.execute( graphic, &mesh.vertices[0] );
		}
	}
#elif UF_USE_VULKAN
	if ( graphic.material.hasShader("vertex") && !(graph.metadata["flags"]["SEPARATE"].as<bool>()) ) {
		auto& shader = graphic.material.getShader("vertex");
		std::vector<pod::Matrix4f> instances( graph.nodes.size() );
		for ( size_t i = 0; i < graph.nodes.size(); ++i ) {
			auto& node = graph.nodes[i];
			instances[i] = node.entity ? uf::transform::model( node.entity->getComponent<pod::Transform<>>() ) : uf::transform::model( node.transform );
		}
		auto& storageBuffer = *graphic.getStorageBuffer("Models");
		graphic.updateBuffer( (void*) instances.data(), instances.size() * sizeof(pod::Matrix4f), graph.instanceBufferIndex /*storageBuffer*/ );
	}
#endif
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
	/* Update uniforms */
	if ( !this->hasComponent<uf::Graphic>() ) return;

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
	if ( graphic.material.hasShader("vertex") ) {
		auto uniformBuffer = graphic.getUniform();
		pod::Uniform& uniform = *((pod::Uniform*) graphic.device->getBuffer(uniformBuffer.buffer));
		uniform.projection = camera.getProjection();
		if ( !(graph.metadata["flags"]["SEPARATE"].as<bool>()) ) {
			uniform.modelView = camera.getView();
		} else {
			uniform.modelView = camera.getView() * uf::transform::model( transform );
		}
		graphic.updateUniform( (void*) &uniform, sizeof(uniform) );
	}
#elif UF_USE_VULKAN
	if ( graphic.material.hasShader("vertex") ) {
		auto& shader = graphic.material.getShader("vertex");
		auto& uniform = shader.getUniform("UBO");
	#if UF_UNIFORMS_UPDATE_WITH_JSON
		if ( !(graph.metadata["flags"]["SEPARATE"].as<bool>()) ) {
			ext::json::Value uniforms;
			for ( std::size_t i = 0; i < uf::renderer::settings::maxViews; ++i ) {
				uniforms["view"][i] = uf::matrix::encode( camera.getView( i ) );
				uniforms["projection"][i] = uf::matrix::encode( camera.getProjection( i ) );
			}
			shader.updateUniform("UBO", uniforms );
		} else {
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
		}
	#else
		if ( !(graph.metadata["flags"]["SEPARATE"].as<bool>()) ) {
			auto& uniforms = uniform.get<UniformDescriptor<>>();
			for ( std::size_t i = 0; i < uf::renderer::settings::maxViews; ++i ) {
				uniforms.view[i] = camera.getView( i );
				uniforms.projection[i] = camera.getProjection( i );
			}
			shader.updateUniform( "UBO", uniform );
		} else {
			auto& uniforms = uniform.get<uf::MeshDescriptor<>>();
			uniforms.matrices.model = uf::transform::model( transform );
			for ( std::size_t i = 0; i < uf::renderer::settings::maxViews; ++i ) {
				uniforms.matrices.view[i] = camera.getView( i );
				uniforms.matrices.projection[i] = camera.getProjection( i );
			}
			uniforms.color = uf::vector::decode( metadata["color"], pod::Vector4f{1,1,1,1} );
			shader.updateUniform( "UBO", uniform );
		}
	#endif
	}
#endif

#if !UNIFORMS_UPDATE_IN_TICK
	if ( uf::renderer::currentRenderMode == uf::renderer::renderModes.front() ) {
	#if UF_USE_OPENGL
		if ( graphic.material.hasShader("vertex") ) {
			auto& shader = graphic.material.getShader("vertex");
			auto& mesh = this->getComponent<ext::gltf::mesh_t>();

			if ( !(graph.metadata["flags"]["SEPARATE"].as<bool>()) ) {
				std::vector<pod::Matrix4f> instances( graph.nodes.size() );
				for ( size_t i = 0; i < graph.nodes.size(); ++i ) {
					auto& node = graph.nodes[i];
					instances[i] = node.entity ? uf::transform::model( node.entity->getComponent<pod::Transform<>>() ) : uf::transform::model( node.transform );
				}

				graphic.updateBuffer( (void*) instances.data(), instances.size() * sizeof(pod::Matrix4f), graph.instanceBufferIndex );
				shader.execute( graphic, &mesh.vertices[0] );
			} else if ( graph.metadata["flags"]["SKINNED"].as<bool>() ) {
				shader.execute( graphic, &mesh.vertices[0] );
			}
		}
	#elif UF_USE_VULKAN
		if ( graphic.material.hasShader("vertex") && !(graph.metadata["flags"]["SEPARATE"].as<bool>()) ) {
			auto& shader = graphic.material.getShader("vertex");
			std::vector<pod::Matrix4f> instances( graph.nodes.size() );
			for ( size_t i = 0; i < graph.nodes.size(); ++i ) {
				auto& node = graph.nodes[i];
				instances[i] = node.entity ? uf::transform::model( node.entity->getComponent<pod::Transform<>>() ) : uf::transform::model( node.transform );
			}
			auto& storageBuffer = *graphic.getStorageBuffer("Models");
			graphic.updateBuffer( (void*) instances.data(), instances.size() * sizeof(pod::Matrix4f), graph.instanceBufferIndex /*storageBuffer*/ );
		}
	#endif
	}
#endif
}
#undef this