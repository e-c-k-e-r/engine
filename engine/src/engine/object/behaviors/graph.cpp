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
#include <uf/utils/debug/draw.h>

UF_BEHAVIOR_REGISTER_CPP(uf::GraphBehavior)
UF_BEHAVIOR_TRAITS_CPP(uf::GraphBehavior, ticks = true, renders = false, thread = "")
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
			auto userdata = uf::asset::release( payload.filename );
			this->moveComponent<pod::Graph>( userdata );
		}

		// bind graph's root entity to self if different
		if ( graph.root.entity && graph.root.entity != this ) {
			//UF_MSG_DEBUG("binding root transform to self");
			auto& transform = this->getComponent<pod::Transform<>>();
			auto& root = *graph.root.entity;
			root.getComponent<pod::Transform<>>().reference = &transform;
			this->addChild(root.as<uf::Entity>());
		}

		uf::graph::initialize( graph );

		if ( graph.metadata["renderer"]["skinned"].as<bool>() ) {
			if ( ext::json::isObject( metadata["graph"]["animations"] ) ) {
				uf::graph::animate( graph, metadata["graph"]["animations"]["animation"].as<uf::stl::string>(), metadata["graph"]["animations"]["speed"].as<float>( 1.0f ) );
			}
		}
	});
}
void uf::GraphBehavior::destroy( uf::Object& self ) {}
void uf::GraphBehavior::tick( uf::Object& self ) {
	if ( !this->hasComponent<pod::Graph>() ) return;
	auto& graph = this->getComponent<pod::Graph>();
	if ( !graph.root.entity || !graph.root.entity->isValid() ) return;
	if ( !graph.metadata["debug"]["draw"]["armature"].as<bool>(false) ) return;
	auto& transform = this->getComponent<pod::Transform<>>();
	auto& storage = uf::graph::getStorage( graph );
	for ( auto& node : graph.nodes ) {
		if ( node.skin < 0 || node.mesh < 0 ) continue;
		auto bones = uf::graph::collectBones( graph, node );
		auto bounds = uf::graph::obbFromSkin( graph, node );
		auto& skinName = graph.skins[node.skin];
		auto& skin = storage.skins[skinName];
		for ( auto bone : bones ) {
			bone.start = uf::transform::apply( transform, bone.start );
			bone.end = uf::transform::apply( transform, bone.end );

			uf::debug::drawLine( bone.start, bone.end, pod::Vector4f{ 0, 1, 0, 1 } );
		}
		for ( auto i = 0; i < skin.joints.size(); ++i ) {
			auto nodeID = skin.joints[i];
			auto& obb = bounds[i];
			auto& bone = bones[nodeID];
			if ( obb.extent.x < 0 ) continue;
			auto bindMatrix = uf::matrix::inverse( skin.inverseBindMatrices[i] );
			auto modelMatrix = uf::transform::model( transform );
			auto finalMatrix = modelMatrix * bindMatrix;
			auto t = uf::transform::fromMatrix( finalMatrix );
			uf::debug::drawShape( obb, t );
		}
	}
}
void uf::GraphBehavior::render( uf::Object& self ) {}
void uf::GraphBehavior::Metadata::serialize( uf::Object& self, uf::Serializer& serializer ) {}
void uf::GraphBehavior::Metadata::deserialize( uf::Object& self, uf::Serializer& serializer ) {}
#undef this