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
UF_BEHAVIOR_TRAITS_CPP(uf::ObjectBehavior, ticks = true, renders = false, thread = "") // segfaults @  engine/src/ext/lua/lua.cpp:298 `auto result = state.safe_script_file( s.file, s.env, sol::script_pass_on_error );`
#define this (&self)
void uf::ObjectBehavior::initialize( uf::Object& self ) {
	if ( !this->isValid() ) this->setUid();

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
		auto callback = this->formatHookName("asset:FinishedLoad.%UID%");
		payload.object = this->resolvable<>();
		if ( payload.async ) {
			uf::asset::load( callback, payload );
		} else {
			if ( uf::asset::load( payload ) != "" ) this->queueHook( callback, payload );
		}
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
		float mass = metadataJsonPhysics["mass"].as<float>(0.0f);

		bool recenter = metadataJsonPhysics["recenter"].as<bool>();
		auto offset = uf::vector::decode( metadataJsonPhysics["offset"], pod::Vector3f{} );
	//	if ( offset == pod::Vector3f{} ) recenter = true;

		auto& body = this->getComponent<pod::PhysicsBody>();
		if ( !body.world ) uf::physics::create( *this, mass, offset );
		if ( type == "bounding box" || type == "box" || type == "obb" ) {
			pod::Vector3f center = uf::vector::decode( metadataJsonPhysics["center"], pod::Vector3f{0.0f, 0.0f, 0.0f} );
			pod::Vector3f extent = uf::vector::decode( metadataJsonPhysics["extent"], pod::Vector3f{0.5f, 0.5f, 0.5f} );
			
			if ( recenter ) {
				offset = center;
				center = {};
			}
		#if OBB_EXTENT_CENTER
			uf::physics::initialize( body, pod::OBB{ .extent = extent, .center = center } );
		#else
			uf::physics::initialize( body, pod::OBB{ .center = center, .extent = extent } );
		#endif
		} else if ( type == "aabb" ) {
			pod::Vector3f min = uf::vector::decode( metadataJsonPhysics["min"], pod::Vector3f{-0.5f, -0.5f, -0.5f} );
			pod::Vector3f max = uf::vector::decode( metadataJsonPhysics["max"], pod::Vector3f{0.5f, 0.5f, 0.5f} );

			if ( recenter ) {
				pod::Vector3f center = (max + min) * 0.5f;
				pod::Vector3f extents = (max - min) * 0.5f;

				min = -extents;
				max =  extents;
				offset = center;
			}

			uf::physics::initialize( body, pod::AABB{ .min = min, .max = max } );
		} else if ( type == "plane" ) {
			pod::Vector3f direction = uf::vector::decode( metadataJsonPhysics["direction"], pod::Vector3f{} );
			float o = metadataJsonPhysics["offset"].as<float>();

			uf::physics::initialize( body, pod::Plane{ direction, o } );
		} else if ( type == "sphere" ) {
			float radius = metadataJsonPhysics["radius"].as<float>();
			
			uf::physics::initialize( body, pod::Sphere{ radius } );
		} else if ( type == "capsule" ) {
			float radius = metadataJsonPhysics["radius"].as<float>();
			auto up = uf::vector::decode( metadataJsonPhysics["up"], pod::Vector3f{0,1,0} );
			if ( metadataJsonPhysics["height"].is<float>() ) {
				up *= metadataJsonPhysics["height"].as<float>() * 0.5f;
			}
			
			uf::physics::initialize( body, pod::Capsule{ radius, up } );
		} else if ( type == "mesh" ) {
			// ...
		} else {
			UF_EXCEPTION("unregistered type: {}", type);
		}
		
		auto gravity = uf::vector::decode( metadataJsonPhysics["gravity"], body.gravity );
		if ( metadataJsonPhysics["category"].is<uf::stl::string>() ){
			uf::physics::setColliderCategory( body, metadataJsonPhysics["category"].as<uf::stl::string>() );
		}
		if ( metadataJsonPhysics["mask"].is<uf::stl::string>() ){
			uf::physics::setColliderMask( body, metadataJsonPhysics["mask"].as<uf::stl::string>() );
		}
		uf::physics::setGravity( body, gravity );

		body.velocity = uf::vector::decode( metadataJsonPhysics["velocity"], body.velocity );
		body.angularVelocity = uf::vector::decode( metadataJsonPhysics["angularVelocity"], body.angularVelocity );
		body.material.staticFriction = metadataJsonPhysics["friction"].as(body.material.staticFriction);
		body.material.restitution = metadataJsonPhysics["restitution"].as(body.material.restitution);
		if ( metadataJsonPhysics["inertia"].is<bool>() && !metadataJsonPhysics["inertia"].as<bool>() ) {
			body.inverseInertiaTensor = { 0.0f, 0.0f, 0.0f };
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

	if ( this->hasComponent<pod::AudioSource>() ) {
		auto& audio = this->getComponent<pod::AudioSource>();
		uf::audio::destroy( audio );
	//	this->deleteComponent<pod::AudioSource>();
	}
	if ( this->hasComponent<uf::AudioEmitter>() ) {
		auto& audio = this->getComponent<uf::AudioEmitter>();
		audio.cleanup(true);
	//	this->deleteComponent<uf::AudioEmitter>();
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

	for ( uf::Entity* kv : this->getChildren() ) kv->destroy();
	if ( this->hasParent() ) this->getParent().removeChild(*this);
	this->unsetUid();
}
void uf::ObjectBehavior::tick( uf::Object& self ) {
	auto& transform = this->getComponent<pod::Transform<>>();
	auto flattened = uf::transform::flatten( transform );
	// update audios
	if ( this->hasComponent<pod::AudioSource>() ) {
		auto& source = this->getComponent<pod::AudioSource>();
		uf::audio::update( source, flattened.position, flattened.orientation );
	}
	if ( this->hasComponent<uf::AudioEmitter>() ) {
		auto& audio = this->getComponent<uf::AudioEmitter>();
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
			if ( q.hash ) {
			#if UF_UF_HOOKS_HASH_KEYS
				if ( q.type == 1 ) {
					this->callHook( q.hash, static_cast<const pod::Hook::userdata_t&>(q.userdata) );
				}
				else if ( q.type == -1 ) this->callHook( q.hash, static_cast<const ext::json::Value&>(q.json) );
				else this->callHook( q.hash );
			#else
				UF_EXCEPTION("unimplemented");
			#endif
			} else {
				if ( q.type == 1 ) {
					this->callHook( q.name, static_cast<const pod::Hook::userdata_t&>(q.userdata) );
				}
				else if ( q.type == -1 ) this->callHook( q.name, static_cast<const ext::json::Value&>(q.json) );
				else this->callHook( q.name );
			}
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