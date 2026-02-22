#include <uf/engine/object/object.h>
#include <uf/engine/asset/asset.h>
#include <uf/engine/scene/scene.h>
#include <uf/utils/time/time.h>
#include <uf/utils/audio/audio.h>
#include <uf/utils/math/transform.h>
#include <uf/utils/math/physics.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/mesh/mesh.h>
#include <uf/utils/renderer/renderer.h>
#include <uf/ext/gltf/gltf.h>
#include <uf/engine/graph/graph.h>

UF_BEHAVIOR_ENTITY_CPP_BEGIN(uf::Object)
UF_BEHAVIOR_TRAITS_CPP(uf::ObjectBehavior, ticks = true, renders = false, multithread = false) // segfaults @  engine/src/ext/lua/lua.cpp:298 `auto result = state.safe_script_file( s.file, s.env, sol::script_pass_on_error );`
#define this (&self)
void uf::ObjectBehavior::initialize( uf::Object& self ) {
#if UF_ENTITY_OBJECT_UNIFIED
	if ( !this->isValid() ) this->setUid();
#endif

	auto& scene = uf::scene::getCurrentScene();
//	auto& assetLoader = scene.getComponent<uf::asset>();
	auto& metadata = this->getComponent<uf::ObjectBehavior::Metadata>();
	auto& metadataJson = this->getComponent<uf::Serializer>();
	auto& transform = this->getComponent<pod::Transform<>>();

	// 
	{
		size_t assets = metadataJson["system"]["assets"].size();
		
		if ( metadata.system.load.ignore ) assets = 0;

		metadata.system.load.progress = 0;
		metadata.system.load.total = assets;

		if ( assets == 0 )  {
			auto& parent = this->getParent().as<uf::Object>();

			pod::payloads::assetLoad payload;
			payload.object = this->resolvable<>();
			parent.lazyCallHook("asset:Parsed.%UID%", payload);
		}
	}

	this->addHook( "object:TransformReferenceController.%UID%", [&](ext::json::Value& json){
		auto& transform = this->getComponent<pod::Transform<>>();
		auto& controller = scene.getController();
		if ( json["target"].as<uf::stl::string>() == "camera" ) {
			auto& camera = controller.getComponent<uf::Camera>();
			transform.reference = &camera.getTransform();
		} else {
			transform.reference = &controller.getComponent<pod::Transform<>>();
		}
	});
	this->addHook( "object:Reload.%UID%", [&](ext::json::Value& json){
		this->callHook("object:Deserialize.%UID%", json);
	});
	this->addHook( "object:Deserialize.%UID%", [&](ext::json::Value& json){	
		if ( ext::json::isNull( json ) ) return;

		if ( json["type"].as<uf::stl::string>() == "merge" ) metadataJson.merge(json["value"], true);
		else if ( json["type"].as<uf::stl::string>() == "import" ) metadataJson.import(json["value"]);
		else if ( json["path"].is<uf::stl::string>() ) metadataJson.path(json["path"].as<uf::stl::string>()) = json["value"];
	//	else metadataJson.merge(json, true);
	});
	this->addHook( "asset:QueueLoad.%UID%", [&](pod::payloads::assetLoad& payload){
		uf::stl::string callback = this->formatHookName("asset:FinishedLoad.%UID%");
	/*
		switch ( payload.type ) {
			case uf::asset::Type::AUDIO:
			case uf::asset::Type::IMAGE:
			case uf::asset::Type::LUA: {
				if ( payload.monoThreaded ) {
					if ( uf::asset::cache( payload ) != "" ) this->queueHook( callback, payload );
				} else {
					uf::asset::cache( callback, payload );
				}
			} break;

			case uf::asset::Type::GRAPH: {
				if ( payload.monoThreaded ) {
					if ( uf::asset::load( payload ) != "" ) this->queueHook( callback, payload );
				} else {
					uf::asset::load( callback, payload );
				}
			} break;
		}
	*/
		payload.object = this->resolvable<>();
		if ( payload.monoThreaded ) {
			if ( uf::asset::load( payload ) != "" ) this->queueHook( callback, payload );
		} else {
			uf::asset::load( callback, payload );
		}
	/*
		if ( payload.monoThreaded ) {
			if ( uf::asset::cache( payload ) != "" ) this->queueHook( callback, payload );
		} else {
			uf::asset::cache( callback, payload );
		}
	*/
	});
	this->addHook( "asset:FinishedLoad.%UID%", [&](pod::payloads::assetLoad& payload){
		this->queueHook("asset:Load.%UID%", payload);
		this->queueHook("asset:Parsed.%UID%", payload);
	});	
	this->addHook( "asset:Load.%UID%", [&](pod::payloads::assetLoad& payload){
		if ( !uf::asset::isExpected( payload, uf::asset::Type::JSON ) ) return;

		uf::Serializer json;
		if ( !json.readFromFile(payload.filename) ) return;

		json["root"] = uf::io::directory(payload.filename);
		json["source"] = payload.filename;
		json["hot reload"]["mtime"] = uf::io::mtime( payload.filename ) + 10;

		if ( this->loadChildUid(json, payload.initialize) == -1 ) return;
	});
	this->addHook( "asset:Parsed.%UID%", [&](pod::payloads::assetLoad& payload){	
		int portion = 1;
		auto& total = metadata.system.load.total;
		metadata.system.load.progress += portion;
		if ( metadata.system.load.progress == metadata.system.load.total ) {
			auto& parent = this->getParent().as<uf::Object>();
			payload.object = this->resolvable<>();
			parent.lazyCallHook("asset:Parsed.%UID%", payload);
		}
	});

	if ( ext::json::isObject(metadataJson["physics"]) ) {
		auto& metadataJsonPhysics = metadataJson["physics"];
		auto type = metadataJsonPhysics["type"].as<uf::stl::string>();
		float mass = metadataJsonPhysics["mass"].as<float>();

		pod::Vector3f offset = uf::vector::decode( metadataJsonPhysics["offset"], pod::Vector3f{} );

		if ( type == "bounding box" || type == "aabb" ) {
			pod::Vector3f min = uf::vector::decode( metadataJsonPhysics["min"], pod::Vector3f{-0.5f, -0.5f, -0.5f} );
			pod::Vector3f max = uf::vector::decode( metadataJsonPhysics["max"], pod::Vector3f{0.5f, 0.5f, 0.5f} );

			UF_MSG_DEBUG("entity={}, min={}, max={}", uf::string::toString( *this ), uf::vector::toString( min ), uf::vector::toString( max ));
		#if UF_USE_REACTPHYSICS
			auto center = ( max + min ) * 0.5f;
			if ( metadataJsonPhysics["recenter"].as<bool>(true) ) offset = (center - transform.position);
		#endif
			
			uf::physics::impl::create( self, pod::AABB{ .min = min, .max = max }, mass, offset );
		} else if ( type == "plane" ) {
			pod::Vector3f direction = uf::vector::decode( metadataJsonPhysics["direction"], pod::Vector3f{} );
			float o = metadataJsonPhysics["offset"].as<float>();

			uf::physics::impl::create( self, pod::Plane{ direction, o }, mass, offset );
		} else if ( type == "sphere" ) {
			float radius = metadataJsonPhysics["radius"].as<float>();
			
			uf::physics::impl::create( self, pod::Sphere{ radius }, mass, offset );
		} else if ( type == "capsule" ) {
			float radius = metadataJsonPhysics["radius"].as<float>();
			float halfHeight = metadataJsonPhysics["height"].as<float>() * 0.5f;
			
			uf::physics::impl::create( self, pod::Capsule{ radius, halfHeight }, mass, offset );
		}

		if ( this->hasComponent<pod::PhysicsBody>() ) {
			auto& physicsBody = this->getComponent<pod::PhysicsBody>();
			
			auto gravity = uf::vector::decode( metadataJsonPhysics["gravity"], physicsBody.gravity );
			auto category = metadataJsonPhysics["category"].as<uf::stl::string>("ALL");
			auto mask = metadataJsonPhysics["mask"].as<uf::stl::string>("ALL");

		#if UF_USE_REACTPHYSICS
			physicsBody.mass = mass;
			physicsBody.gravity = gravity;

			physicsBody.material.restitution = metadataJsonPhysics["restitution"].as(physicsBody.material.restitution);
			physicsBody.material.staticFriction = metadataJsonPhysics["friction"].as(physicsBody.material.staticFriction);
			physicsBody.inertiaTensor = uf::vector::decode( metadataJsonPhysics["inertia"], physicsBody.inertiaTensor );
		#else
			uf::physics::impl::setColliderCategory( physicsBody, category );
			uf::physics::impl::setColliderMask( physicsBody, mask );
			uf::physics::impl::setGravity( physicsBody, gravity );
		#endif

			physicsBody.velocity = uf::vector::decode( metadataJsonPhysics["velocity"], physicsBody.velocity );
			physicsBody.angularVelocity = uf::vector::decode( metadataJsonPhysics["angularVelocity"], physicsBody.angularVelocity );
			
			if ( metadataJsonPhysics["inertia"].is<bool>() && !metadataJsonPhysics["inertia"].as<bool>() ) {
				physicsBody.inertiaTensor = { FLT_MAX, FLT_MAX, FLT_MAX };
				physicsBody.inverseInertiaTensor = { 0.0f, 0.0f, 0.0f };
			}
		}
	}

	UF_BEHAVIOR_METADATA_BIND_SERIALIZER_HOOKS(metadata, metadataJson);
}
void uf::ObjectBehavior::destroy( uf::Object& self ) {
	auto& metadata = this->getComponent<uf::ObjectBehavior::Metadata>();
	for ( auto pair : metadata.hooks.bound ) {
		for ( auto id : pair.second ) {
			uf::hooks.removeHook(pair.first, id);
		}
	}

	if ( this->hasComponent<uf::Audio>() ) {
		auto& audio = this->getComponent<uf::Audio>();
		audio.destroy();
	//	this->deleteComponent<uf::Audio>();
	}
	if ( this->hasComponent<uf::SoundEmitter>() ) {
		auto& audio = this->getComponent<uf::SoundEmitter>();
		audio.cleanup(true);
	//	this->deleteComponent<uf::SoundEmitter>();
	}
	if ( this->hasComponent<uf::MappedSoundEmitter>() ) {
		auto& audio = this->getComponent<uf::MappedSoundEmitter>();
		audio.cleanup(true);
	//	this->deleteComponent<uf::MappedSoundEmitter>();
	}
	if ( this->hasComponent<uf::Graphic>() ) {
		auto& graphic = this->getComponent<uf::Graphic>();
		graphic.destroy();
	//	this->deleteComponent<uf::Graphic>();
	}
	if ( this->hasComponent<pod::Graph>() ) {
		auto& graph = this->getComponent<pod::Graph>();
	//	uf::graph::destroy( graph );
	//	this->deleteComponent<pod::Graph>();
	}
	if ( this->hasComponent<uf::Atlas>() ) {
		auto& atlas = this->getComponent<uf::Atlas>();
		atlas.clear();
	//	this->deleteComponent<uf::Atlas>();
	}
	#if UF_USE_REACTPHYSICS
	if ( this->hasComponent<pod::PhysicsBody>() ) {
		auto& physicsBody = this->getComponent<pod::PhysicsBody>();
		uf::physics::impl::detach( physicsBody );
	//	this->deleteComponent<pod::PhysicsBody>();
	}
	#endif
	if ( this->hasComponent<uf::renderer::RenderTargetRenderMode>() ) {
		auto& renderMode = this->getComponent<uf::renderer::RenderTargetRenderMode>();
		if ( uf::renderer::settings::experimental::registerRenderMode ) {
			uf::renderer::removeRenderMode( &renderMode, false );
		} else {
			renderMode.destroy();
		//	this->deleteComponent<uf::renderer::RenderTargetRenderMode>();
		}
	}
	if ( this->hasComponent<uf::renderer::DeferredRenderMode>() ) {
		auto& renderMode = this->getComponent<uf::renderer::DeferredRenderMode>();
		if ( uf::renderer::settings::experimental::registerRenderMode ) {
			uf::renderer::removeRenderMode( &renderMode, false );
		} else {
			renderMode.destroy();
		//	this->deleteComponent<uf::renderer::DeferredRenderMode>();
		}
	}

#if UF_ENTITY_OBJECT_UNIFIED
	for ( uf::Entity* kv : this->getChildren() ) kv->destroy();
	if ( this->hasParent() ) this->getParent().removeChild(*this);
	this->unsetUid();
#endif
}
void uf::ObjectBehavior::tick( uf::Object& self ) {
	auto& transform = this->getComponent<pod::Transform<>>();
	auto flattened = uf::transform::flatten( transform );
	// update audios
	if ( this->hasComponent<uf::Audio>() ) {
		auto& audio = this->getComponent<uf::Audio>();
		audio.update( flattened.position, flattened.orientation );
	}
	if ( this->hasComponent<uf::SoundEmitter>() ) {
		auto& audio = this->getComponent<uf::SoundEmitter>();
		audio.update( flattened.position, flattened.orientation );
		audio.cleanup();
	}
	if ( this->hasComponent<uf::MappedSoundEmitter>() ) {
		auto& audio = this->getComponent<uf::MappedSoundEmitter>();
		audio.update( flattened.position, flattened.orientation );
		audio.cleanup();
	}
	if ( this->hasComponent<pod::Graph>() ) {
		auto& graph = this->getComponent<pod::Graph>();
		uf::graph::update( graph );
	}

	auto& metadata = this->getComponent<uf::ObjectBehavior::Metadata>();
#if UF_ENTITY_METADATA_USE_JSON
	metadata.deserialize(self, metadataJson);
#endif
	// listen for metadata file changes
#if !UF_ENV_DREAMCAST
	if ( metadata.system.hotReload.enabled ) {
		size_t mtime = uf::io::mtime( metadata.system.filename );
		if ( metadata.system.hotReload.mtime < mtime ) {
			metadata.system.hotReload.mtime = mtime;
			this->reload();
		}
	}
#endif

	if ( metadata.transform.trackParent && this->hasParent() ) {
		auto& parent = this->getParent();
		auto& transform = this->getComponent<pod::Transform<>>();
		auto& parentTransform = parent.getComponent<pod::Transform<>>();
		transform.position = uf::transform::flatten( parentTransform ).position + metadata.transform.initial.position;
	}

	auto& queue = metadata.hooks.queue;
//	if ( !queue.empty() )
	{
		decltype(metadata.hooks.queue) unprocessed;
		unprocessed.reserve( metadata.hooks.queue.size() );

		decltype(metadata.hooks.queue) executeQueue;
		executeQueue.reserve( metadata.hooks.queue.size() );

		for ( auto& q : queue ) {
			if ( q.timeout < uf::time::current ) executeQueue.emplace_back(q);
			else unprocessed.emplace_back(q);
		}
		for ( auto& q : executeQueue ) {
			if ( q.type == 1 ) {
				this->callHook( q.name, q.userdata );
			}
			else if ( q.type == -1 ) this->callHook( q.name, q.json );
			else this->callHook( q.name );
		}
		queue = std::move(unprocessed);
	}

#if UF_ENTITY_METADATA_USE_JSON
	metadata.serialize(self, metadataJson);
#endif
}
void uf::ObjectBehavior::render( uf::Object& self ) {}
#undef this

void uf::ObjectBehavior::Metadata::serialize( uf::Object& self, uf::Serializer& serializer ) {
	if ( /*this->*/transform.trackParent ) serializer["system"]["transform"]["track"] = "parent";
}
void uf::ObjectBehavior::Metadata::deserialize( uf::Object& self, uf::Serializer& serializer ) {
	if ( !transform.trackParent ) /*this->*/transform.initial = self.getComponent<pod::Transform<>>();
	/*this->*/transform.trackParent = serializer["system"]["transform"]["track"].as<uf::stl::string>(/*this->*/transform.trackParent ? "parent" : "") == "parent";
}
UF_BEHAVIOR_ENTITY_CPP_END(Object)