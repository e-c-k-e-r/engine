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

		// deferred shader loading
		auto& transform = this->getComponent<pod::Transform<>>();
		graph.root.entity->getComponent<pod::Transform<>>().reference = &transform;

		uf::graph::initialize( graph );

		if ( graph.metadata["renderer"]["skinned"].as<bool>() ) {
			if ( metadata["graph"]["animation"].is<uf::stl::string>() ) {
				uf::graph::animate( graph, metadata["graph"]["animation"].as<uf::stl::string>() );
			}
		}

		this->addChild(graph.root.entity->as<uf::Entity>());
	});
}
void uf::GraphBehavior::destroy( uf::Object& self ) {}
void uf::GraphBehavior::tick( uf::Object& self ) {}
void uf::GraphBehavior::render( uf::Object& self ) {}
void uf::GraphBehavior::Metadata::serialize( uf::Object& self, uf::Serializer& serializer ) {}
void uf::GraphBehavior::Metadata::deserialize( uf::Object& self, uf::Serializer& serializer ) {}
#undef this