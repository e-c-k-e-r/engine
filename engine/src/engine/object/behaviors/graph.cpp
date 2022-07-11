#include <uf/engine/object/behaviors/graph.h>

#include <uf/engine/object/object.h>
#include <uf/engine/asset/asset.h>
#include <uf/engine/scene/scene.h>
#include <uf/engine/graph/graph.h>
#include <uf/utils/time/time.h>
#include <uf/utils/math/transform.h>
#include <uf/utils/math/physics.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/thread/thread.h>
#include <uf/utils/mesh/mesh.h>
#include <uf/utils/string/hash.h>

UF_BEHAVIOR_REGISTER_CPP(uf::GraphBehavior)
UF_BEHAVIOR_TRAITS_CPP(uf::GraphBehavior, ticks = false, renders = false, multithread = false)
#define this (&self)
void uf::GraphBehavior::initialize( uf::Object& self ) {	
	auto& metadata = this->getComponent<uf::Serializer>();
	this->addHook( "animation:Set.%UID%", [&](ext::json::Value& json){
		uf::stl::string name = json["name"].as<uf::stl::string>();

		if ( !this->hasComponent<pod::Graph>() ) return;
		auto& graph = this->getComponent<pod::Graph>();
		uf::graph::animate( graph, name );
	});
	this->addHook( "asset:Load.%UID%", [&](pod::payloads::assetLoad& payload){
		if ( !uf::Asset::isExpected( payload, uf::Asset::Type::GRAPH ) ) return;

		auto& scene = uf::scene::getCurrentScene();
		auto& assetLoader = scene.getComponent<uf::Asset>();
		if ( !assetLoader.has<pod::Graph>(payload.filename) ) return;
		auto& graph = (this->getComponent<pod::Graph>() = std::move( assetLoader.get<pod::Graph>(payload.filename) ));
		assetLoader.remove<pod::Graph>(payload.filename);

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
		if ( !ext::json::isNull(graph.metadata["gamma"]) ) {
			sceneMetadataJson["light"]["gamma"] = graph.metadata["gamma"];
			shouldUpdate = true;	
		}
		if ( !ext::json::isNull(graph.metadata["brightnessThreshold"]) ) {
			sceneMetadataJson["light"]["brightnessThreshold"] = graph.metadata["brightnessThreshold"];
			shouldUpdate = true;	
		}
		if ( shouldUpdate ) scene.callHook("object:Deserialize.%UID%");

		// deferred shader loading
		auto& transform = this->getComponent<pod::Transform<>>();
		graph.root.entity->getComponent<pod::Transform<>>().reference = &transform;

		uf::graph::initialize( graph );

		if ( graph.metadata["renderer"]["skinned"].as<bool>() ) {
			if ( metadata["model"]["animation"].is<uf::stl::string>() ) {
				uf::graph::animate( graph, metadata["model"]["animation"].as<uf::stl::string>() );
			}
		}

		this->addChild(graph.root.entity->as<uf::Entity>());
	});
}
void uf::GraphBehavior::destroy( uf::Object& self ) {}
void uf::GraphBehavior::tick( uf::Object& self ) {
#if 0
	if ( this->hasComponent<pod::Graph>() ) {
		auto& graph = this->getComponent<pod::Graph>();
		uf::graph::update( graph );
	}
	/* Update animations */ if ( this->hasComponent<pod::Graph>() ) {
		auto& graph = this->getComponent<pod::Graph>();
	//	if ( graph.metadata["renderer"]["skinned"].as<bool>() ) uf::graph::update( graph );
	}

	if ( !this->hasComponent<uf::Graphic>() ) return;

	const auto& scene = uf::scene::getCurrentScene();
	const auto& metadata = this->getComponent<uf::Serializer>();
	const auto& graphic = this->getComponent<uf::Graphic>();
	const auto& controller = scene.getController();
	const auto& camera = controller.getComponent<uf::Camera>();
	const auto& transform = this->getComponent<pod::Transform<>>();

	if ( !graphic.initialized ) return;

	const auto* objectWithGraph = this;
	while ( objectWithGraph != &scene ) {
		if ( objectWithGraph->hasComponent<pod::Graph>() ) break;
		objectWithGraph = &objectWithGraph->getParent().as<uf::Object>();
	}
	if ( !objectWithGraph->hasComponent<pod::Graph>() ) return;
	auto& graph = objectWithGraph->getComponent<pod::Graph>();

#if UF_USE_OPENGL
	if ( graphic.material.hasShader("vertex") ) {
		auto& shader = graphic.material.getShader("vertex");
		auto& mesh = this->getComponent<pod::Graph::Mesh>();

		if ( !(graph.metadata["renderer"]["separate"].as<bool>()) ) {
			uf::stl::vector<pod::Matrix4f> instances( graph.nodes.size() );
			for ( size_t i = 0; i < graph.nodes.size(); ++i ) {
				auto& node = graph.nodes[i];
				instances[i] = uf::transform::model( node.entity ? node.entity->getComponent<pod::Transform<>>() : node.transform );
			}
			shader.updateBuffer( (const void*) instances.data(), instances.size() * sizeof(pod::Matrix4f), graph.buffers.instance );
			shader.execute( graphic, mesh.attributes.vertex.pointer );
		} else if ( graph.metadata["renderer"]["skinned"].as<bool>() ) {
			shader.execute( graphic, mesh.attributes.vertex.pointer );
		}
	}
#elif UF_USE_VULKAN
	if ( graphic.material.hasShader("vertex") ) {
		auto& shader = graphic.material.getShader("vertex");

		if ( !(graph.metadata["renderer"]["separate"].as<bool>()) ) {
			uf::stl::vector<pod::Matrix4f> instances( graph.nodes.size() );
			for ( size_t i = 0; i < graph.nodes.size(); ++i ) {
				auto& node = graph.nodes[i];
				instances[i] = uf::transform::model( node.entity ? node.entity->getComponent<pod::Transform<>>() : node.transform );
			}
			shader.updateBuffer( (const void*) instances.data(), instances.size() * sizeof(pod::Matrix4f), graph.buffers.instance /*storageBuffer*/ );
		} else {
			struct UniformDescriptor {
				pod::Matrix4f model;
			};
		#if UF_UNIFORMS_REUSE
			auto& uniform = shader.getUniform("UBO");
			auto& uniforms = uniform.get<UniformDescriptor>();
			uniforms = {
				.model = uf::transform::model( transform );
		//		.color = uf::vector::decode( metadata["color"], pod::Vector4f{1,1,1,1} ),
			};
			shader.updateUniform( "UBO", uniform );
		#else
			UniformDescriptor uniforms = {
				.model = uf::transform::model( transform ),
		//		.color = uf::vector::decode( metadata["color"], pod::Vector4f{1,1,1,1} ),
			};
			shader.updateBuffer( uniforms, shader.getUniformBuffer("UBO") );
		#endif
		}
	}
#if 0
	if ( graphic.material.hasShader("vertex") && !(graph.metadata["renderer"]["separate"].as<bool>()) ) {
		auto& shader = graphic.material.getShader("vertex");
		uf::stl::vector<pod::Matrix4f> instances( graph.nodes.size() );
		for ( size_t i = 0; i < graph.nodes.size(); ++i ) {
			auto& node = graph.nodes[i];
			instances[i] = node.entity ? uf::transform::model( node.entity->getComponent<pod::Transform<>>() ) : uf::transform::model( node.transform );
		}
		auto& storageBuffer = *graphic.getStorageBuffer("Models");
		shader.updateBuffer( (const void*) instances.data(), instances.size() * sizeof(pod::Matrix4f), graph.buffers.instance /*storageBuffer*/ );
	}
#endif
#endif
#endif
}
void uf::GraphBehavior::render( uf::Object& self ) {
#if 0
	/* Update uniforms */
	if ( !this->hasComponent<uf::Graphic>() ) return;

	const auto& scene = uf::scene::getCurrentScene();
	const auto& metadata = this->getComponent<uf::Serializer>();
	const auto& graphic = this->getComponent<uf::Graphic>();
	const auto& controller = scene.getController();
	const auto& camera = controller.getComponent<uf::Camera>();
	const auto& transform = this->getComponent<pod::Transform<>>();

	if ( !graphic.initialized ) return;

#if UF_USE_OPENGL
	const auto* objectWithGraph = this;
	while ( objectWithGraph != &scene ) {
		if ( objectWithGraph->hasComponent<pod::Graph>() ) break;
		objectWithGraph = &objectWithGraph->getParent().as<uf::Object>();
	}
	if ( !objectWithGraph->hasComponent<pod::Graph>() ) return;
	auto& graph = objectWithGraph->getComponent<pod::Graph>();
#if UF_UNIFORMS_REUSE
	auto uniforms = graphic.getUniform();
	pod::Uniform& uniform = *((pod::Uniform*) graphic.device->getBuffer(uniforms.buffer));
	uniform = {
		.modelView = !(graph.metadata["renderer"]["separate"].as<bool>()) ? camera.getView() : camera.getView() * uf::transform::model( transform ),
		.projection = camera.getProjection();
	};
	graphic.updateUniform( (const void*) &uniform, sizeof(uniform) );
#else
	pod::Uniform uniform = {
		.modelView = !(graph.metadata["renderer"]["separate"].as<bool>()) ? camera.getView() : camera.getView() * uf::transform::model( transform ),
		.projection = camera.getProjection(),
	};
	graphic.updateUniform( (const void*) &uniform, sizeof(uniform) );
#endif
#elif UF_USE_VULKAN
	if ( graphic.material.hasShader("vertex") ) {
		auto& shader = graphic.material.getShader("vertex");
	#if UF_UNIFORMS_REUSE
		auto& uniform = shader.getUniform("Camera");
		auto& uniforms = uniform.get<pod::Camera::Viewports>();
		uniforms = camera.data().viewport;
		shader.updateUniform("Camera", uniform);
	#else
		pod::Camera::Viewports uniforms = camera.data().viewport;
		shader.updateBuffer( uniforms, shader.getUniformBuffer("Camera") );
	#endif
	}
#endif
#endif
}
void uf::GraphBehavior::Metadata::serialize( uf::Object& self, uf::Serializer& serializer ) {}
void uf::GraphBehavior::Metadata::deserialize( uf::Object& self, uf::Serializer& serializer ) {}
#undef this