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
#include <uf/utils/io/inputs.h>

UF_BEHAVIOR_REGISTER_CPP(uf::GraphBehavior)
UF_BEHAVIOR_TRAITS_CPP(uf::GraphBehavior, ticks = true, renders = false, multithread = false)
#define this (&self)
void uf::GraphBehavior::initialize( uf::Object& self ) {	
	auto& metadata = this->getComponent<uf::Serializer>();
	this->addHook( "animation:Set.%UID%", [&](ext::json::Value& json){
		auto name = json["name"].as<uf::stl::string>();
		auto loop = json["loop"].as<bool>();
		auto speed = json["speed"].as<float>();

		pod::payloads::QueueAnimationPayload payload;
		payload.name = name;
		payload.loop = loop;
		payload.speed = speed;
		this->callHook( "animation:Set.%UID%", payload );
	});
	this->addHook( "animation:Set.%UID%", [&](pod::payloads::QueueAnimationPayload& payload){

		if ( !this->hasComponent<pod::Graph>() ) return;
		auto& graph = this->getComponent<pod::Graph>();

		graph.settings.animations.loop = payload.loop;
		uf::graph::animate( graph, payload.name, payload.speed );
	});

	this->addHook( "asset:Load.%UID%", [&](pod::payloads::assetLoad& payload){
		if ( !uf::asset::isExpected( payload, uf::asset::Type::GRAPH ) ) return;
		if ( !uf::asset::has( payload ) ) uf::asset::load( payload );
		auto& graph = payload.asComponent ? this->getComponent<pod::Graph>() : uf::asset::get<pod::Graph>( payload );
		if ( !payload.asComponent ) {
		//	auto asset = uf::asset::release( payload.filename );
		//	this->moveComponent<pod::Graph>( asset );
			this->moveComponent<pod::Graph>( uf::asset::get( payload.filename ) );
			uf::asset::remove( payload.filename );
		}

		// deferred shader loading
		auto& transform = this->getComponent<pod::Transform<>>();
		auto& root = *graph.root.entity;
		root.getComponent<pod::Transform<>>().reference = &transform;

		uf::graph::initialize( graph );

		if ( graph.metadata["renderer"]["skinned"].as<bool>() ) {
			if ( metadata["graph"]["animation"].is<uf::stl::string>() ) {
				uf::graph::animate( graph, metadata["graph"]["animation"].as<uf::stl::string>(), metadata["graph"]["speed"].as<float>( 1.0f ) );
			}
		}

		this->addChild(root.as<uf::Entity>());
	});
}
void uf::GraphBehavior::destroy( uf::Object& self ) {}
void uf::GraphBehavior::tick( uf::Object& self ) {
	/* Test */ {
		TIMER(1, uf::inputs::kbm::states::T ) {
			UF_MSG_DEBUG("Regenerating graph graphics...");
			auto& graph = this->getComponent<pod::Graph>();
			uf::graph::reload( graph );
		}
	}
}
void uf::GraphBehavior::render( uf::Object& self ) {}
void uf::GraphBehavior::Metadata::serialize( uf::Object& self, uf::Serializer& serializer ) {}
void uf::GraphBehavior::Metadata::deserialize( uf::Object& self, uf::Serializer& serializer ) {}
#undef this