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
#include <uf/utils/math/physics.h>
#include <uf/ext/gltf/gltf.h>

UF_BEHAVIOR_ENTITY_CPP_BEGIN(uf::Object)
UF_BEHAVIOR_TRAITS_CPP(uf::ObjectBehavior, ticks = true, renders = false, multithread = false) // segfaults @  engine/src/ext/lua/lua.cpp:298 `auto result = state.safe_script_file( s.file, s.env, sol::script_pass_on_error );`
#define this (&self)
void uf::ObjectBehavior::initialize( uf::Object& self ) {
	auto& scene = uf::scene::getCurrentScene();
	auto& assetLoader = scene.getComponent<uf::Asset>();
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
			payload.uid = this->getUid();
			parent.callHook("asset:Parsed.%UID%", payload);
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
		if ( payload.monoThreaded ) {
			if ( assetLoader.load( payload ) != "" ) this->queueHook( callback, payload );
		} else {
			assetLoader.load( callback, payload );
		}
	});
	this->addHook( "asset:FinishedLoad.%UID%", [&](pod::payloads::assetLoad& payload){
		this->queueHook("asset:Load.%UID%", payload);
		this->queueHook("asset:Parsed.%UID%", payload);
	});	
	this->addHook( "asset:Load.%UID%", [&](pod::payloads::assetLoad& payload){
		if ( !uf::Asset::isExpected( payload, uf::Asset::Type::JSON ) ) return;

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

			payload.uid = this->getUid();
			parent.callHook("asset:Parsed.%UID%", payload);
		}
	});

	UF_BEHAVIOR_METADATA_BIND_SERIALIZER_HOOKS(metadata, metadataJson);

	if ( ext::json::isObject(metadataJson["physics"]) ) {
		auto& collider = this->getComponent<pod::PhysicsState>();
		collider.stats.flags = metadataJson["physics"]["flags"].as(collider.stats.flags);
		collider.stats.mass = metadataJson["physics"]["mass"].as(collider.stats.mass);
		collider.stats.restitution = metadataJson["physics"]["restitution"].as(collider.stats.restitution);
		collider.stats.friction = metadataJson["physics"]["friction"].as(collider.stats.friction);
		collider.stats.inertia = uf::vector::decode( metadataJson["physics"]["inertia"], collider.stats.inertia );
		collider.stats.gravity = uf::vector::decode( metadataJson["physics"]["gravity"], collider.stats.gravity );
	
		if ( metadataJson["physics"]["type"].as<uf::stl::string>() == "bounding box" ) {
			pod::Vector3f center = uf::vector::decode( metadataJson["physics"]["center"], pod::Vector3f{} );
			pod::Vector3f corner = uf::vector::decode( metadataJson["physics"]["corner"], pod::Vector3f{0.5, 0.5, 0.5} );

			if ( metadataJson["physics"]["recenter"].as<bool>(true) ) collider.transform.position = (center - transform.position);

			uf::physics::impl::create( *this, corner );
		} else if ( metadataJson["physics"]["type"].as<uf::stl::string>() == "capsule" ) {
			float radius = metadataJson["physics"]["radius"].as<float>();
			float height = metadataJson["physics"]["height"].as<float>();

			uf::physics::impl::create( *this, radius, height );
		}
	}
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
		uf::graph::destroy( graph );
	//	this->deleteComponent<pod::Graph>();
	}
	if ( this->hasComponent<uf::Atlas>() ) {
		auto& atlas = this->getComponent<uf::Atlas>();
		atlas.clear();
	//	this->deleteComponent<uf::Atlas>();
	}
	if ( this->hasComponent<pod::PhysicsState>() ) {
	//	auto& collider = this->getComponent<pod::PhysicsState>();
	//	uf::physics::impl::detach( collider );
		uf::physics::impl::destroy( *this );
	}
#if UF_USE_VULKAN
	if ( this->hasComponent<uf::renderer::RenderTargetRenderMode>() ) {
		auto& renderMode = this->getComponent<uf::renderer::RenderTargetRenderMode>();
		uf::renderer::removeRenderMode( &renderMode, false );
		renderMode.destroy();
	//	this->deleteComponent<uf::renderer::RenderTargetRenderMode>();
	}
#endif
}
void uf::ObjectBehavior::tick( uf::Object& self ) {
	// update audios
	if ( this->hasComponent<uf::Audio>() ) {
		auto& audio = this->getComponent<uf::Audio>();
		audio.update();
	}
	if ( this->hasComponent<uf::SoundEmitter>() ) {
		auto& audio = this->getComponent<uf::SoundEmitter>();
		audio.update();
	}
	if ( this->hasComponent<uf::MappedSoundEmitter>() ) {
		auto& audio = this->getComponent<uf::MappedSoundEmitter>();
		audio.update();
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

//	if ( !uf::Object::timer.running() ) uf::Object::timer.start();
//	double curTime = uf::Object::timer.elapsed().asDouble();
	auto curTime = uf::time::current;

	decltype(metadata.hooks.queue) unprocessed;
	unprocessed.reserve( metadata.hooks.queue.size() );

	decltype(metadata.hooks.queue) executeQueue;
	executeQueue.reserve( metadata.hooks.queue.size() );

	for ( auto& q : queue ) {
		if ( q.timeout < curTime ) executeQueue.emplace_back(q);
		else unprocessed.emplace_back(q);
	}
	for ( auto& q : executeQueue ) {
		if ( q.type == 1 ) this->callHook( q.name, q.userdata );
		else if ( q.type == -1 ) this->callHook( q.name, q.json );
		else this->callHook( q.name );
	}
	queue = std::move(unprocessed);

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