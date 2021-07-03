#include <uf/engine/object/object.h>
#include <uf/engine/asset/asset.h>
#include <uf/engine/scene/scene.h>
#include <uf/utils/time/time.h>
#include <uf/utils/audio/audio.h>
#include <uf/utils/math/transform.h>
#include <uf/utils/math/physics.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/graphic/mesh.h>
#include <uf/utils/renderer/renderer.h>
#include <uf/ext/gltf/gltf.h>
#include <uf/ext/bullet/bullet.h>

UF_BEHAVIOR_ENTITY_CPP_BEGIN(uf::Object)
UF_BEHAVIOR_TRAITS_CPP(uf::ObjectBehavior, ticks = true, renders = false, multithread = false)
#define this (&self)
void uf::ObjectBehavior::initialize( uf::Object& self ) {
	auto& scene = uf::scene::getCurrentScene();
	auto& assetLoader = scene.getComponent<uf::Asset>();
	auto& metadataJson = this->getComponent<uf::Serializer>();

	// 
	{
		size_t assets = metadataJson["system"]["assets"].size();
		if ( metadataJson["system"]["load"]["ignore"].is<bool>() ) assets = 0;
		metadataJson["system"]["load"]["progress"] = 0;
		metadataJson["system"]["load"]["total"] = assets;
		if ( assets == 0 )  {
			auto& parent = this->getParent().as<uf::Object>();
			uf::Serializer payload;
			payload["uid"] = this->getUid();
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
	this->addHook( "object:UpdateMetadata.%UID%", [&](ext::json::Value& json){	
		if ( ext::json::isNull( json ) ) return;
		if ( json["type"].as<uf::stl::string>() == "merge" ) {
			metadataJson.merge(json["value"], true);
		} else if ( json["type"].as<uf::stl::string>() == "import" ) {
			metadataJson.import(json["value"]);
		} else if ( json["path"].is<uf::stl::string>() ) {
			metadataJson.path(json["path"].as<uf::stl::string>()) = json["value"];
		} else {
			metadataJson.merge(json, true);
		}
	});
	this->addHook( "asset:QueueLoad.%UID%", [&](ext::json::Value& json){
		uf::stl::string filename = json["filename"].as<uf::stl::string>();
		uf::stl::string hash = json["hash"].as<uf::stl::string>();
		uf::stl::string category = json["category"].as<uf::stl::string>();
		uf::stl::string callback = this->formatHookName("asset:FinishedLoad.%UID%");
		if ( json["single threaded"].as<bool>() ) {
			assetLoader.load( filename, hash, category );
			this->queueHook( callback, json );
		} else {
			assetLoader.load( callback, filename, hash, category );
		}
	});
	this->addHook( "asset:FinishedLoad.%UID%", [&](ext::json::Value& json){
		this->queueHook("asset:Load.%UID%", json);
		this->queueHook("asset:Parsed.%UID%", json);
	});	
	this->addHook( "asset:Load.%UID%", [&](ext::json::Value& json){
		uf::stl::string filename = json["filename"].as<uf::stl::string>();
		uf::stl::string category = json["category"].as<uf::stl::string>();
		bool initialize = ext::json::isNull( json["initialize"] ) ? true : json["initialize"].as<bool>();
		if ( category != "" && category != "entities" ) return;
		if ( category == "" && uf::io::extension(filename) != "json" ) return;
		{
			uf::Serializer json;
			if ( !json.readFromFile(filename) ) return;

			json["root"] = uf::io::directory(filename);
			json["source"] = filename;
			json["hot reload"]["mtime"] = uf::io::mtime( filename ) + 10;

			if ( this->loadChildUid(json, initialize) == -1 ) return;
		}
	});
	this->addHook( "asset:Parsed.%UID%", [&](ext::json::Value& json){	
		int portion = 1;
		auto& total = metadataJson["system"]["load"]["total"];
		auto& progress = metadataJson["system"]["load"]["progress"];
		progress = progress.as<int>() + portion;
		if ( progress.as<int>() == total.as<int>() ) {
			auto& parent = this->getParent().as<uf::Object>();

			uf::Serializer payload;
			payload["uid"] = this->getUid();
			parent.callHook("asset:Parsed.%UID%", payload);
		}
	});

	auto& metadata = this->getComponent<uf::ObjectBehavior::Metadata>();
	metadata.serialize = [&](){
		if ( metadata.transform.trackParent ) metadataJson["system"]["transform"]["track"] = "parent";
	};
	metadata.deserialize = [&](){
		metadata.transform.initial = this->getComponent<pod::Transform<>>();
		metadata.transform.trackParent = metadataJson["system"]["transform"]["track"].as<uf::stl::string>() == "parent";
	};
	this->addHook( "object:UpdateMetadata.%UID%", metadata.deserialize);
	metadata.deserialize();

#if UF_USE_BULLET
	if ( ext::json::isObject(metadataJson["system"]["physics"]) ) {
		float mass = metadataJson["system"]["physics"]["mass"].as<float>();
		auto& collider = this->getComponent<pod::Bullet>();
		if ( !ext::json::isNull( metadataJson["system"]["physics"]["inertia"] ) ) {
			collider.inertia = uf::vector::decode( metadataJson["system"]["physics"]["inertia"], pod::Vector3f{} );
		}
		if ( metadataJson["system"]["physics"]["type"].as<uf::stl::string>() == "BoundingBox" ) {
			pod::Vector3f corner = uf::vector::decode( metadataJson["system"]["physics"]["corner"], pod::Vector3f{0.5, 0.5, 0.5} );
			ext::bullet::create( *this, corner, mass );
		} else if ( metadataJson["system"]["physics"]["type"].as<uf::stl::string>() == "Capsule" ) {
			float radius = metadataJson["system"]["physics"]["radius"].as<float>();
			float height = metadataJson["system"]["physics"]["height"].as<float>();
			ext::bullet::create( *this, radius, height, mass );
		} else {
			return;
		}
		if ( !ext::json::isNull( metadataJson["system"]["physics"]["gravity"] ) ) {
			pod::Vector3f v = uf::vector::decode( metadataJson["system"]["physics"]["gravity"], pod::Vector3f{} );
			collider.body->setGravity( btVector3( v.x, v.y, v.z ) );
		}
		if ( metadataJson["system"]["physics"]["shared"].is<bool>() ) {
			collider.shared = metadataJson["system"]["physics"]["shared"].as<bool>();
		}
	}
#endif
}
void uf::ObjectBehavior::destroy( uf::Object& self ) {
#if UF_ENTITY_METADATA_USE_JSON
	auto& metadata = this->getComponent<uf::Serializer>();
	ext::json::forEach( metadata["system"]["hooks"]["bound"], [&]( const uf::stl::string& key, ext::json::Value& value ){
		ext::json::forEach( value, [&]( ext::json::Value& id ){
			uf::hooks.removeHook(key, id.as<size_t>());
		});
	});

#else
	auto& metadata = this->getComponent<uf::ObjectBehavior::Metadata>();
	for ( auto pair : metadata.hooks.bound ) {
		for ( auto id : pair.second ) uf::hooks.removeHook(pair.first, id);
	}
#endif

	if ( this->hasComponent<uf::Audio>() ) {
		auto& audio = this->getComponent<uf::Audio>();
		audio.destroy();
	//	this->deleteComponent<uf::Audio>();
	//	UF_MSG_DEBUG("Destroying audio: " << this->getName() << ": " << this->getUid());
	}
	if ( this->hasComponent<uf::SoundEmitter>() ) {
		auto& audio = this->getComponent<uf::SoundEmitter>();
		audio.cleanup(true);
	//	this->deleteComponent<uf::SoundEmitter>();
	//	UF_MSG_DEBUG("Destroying sound emitter: " << this->getName() << ": " << this->getUid());
	}
	if ( this->hasComponent<uf::MappedSoundEmitter>() ) {
		auto& audio = this->getComponent<uf::MappedSoundEmitter>();
		audio.cleanup(true);
	//	this->deleteComponent<uf::MappedSoundEmitter>();
	//	UF_MSG_DEBUG("Destroying sound emitter: " << this->getName() << ": " << this->getUid());
	}
	if ( this->hasComponent<uf::Graphic>() ) {
		auto& graphic = this->getComponent<uf::Graphic>();
		graphic.destroy();
	//	this->deleteComponent<uf::Graphic>();
	//	UF_MSG_DEBUG("Destroying graphic: " << this->getName() << ": " << this->getUid());
	}
	if ( this->hasComponent<pod::Graph>() ) {
		auto& graph = this->getComponent<pod::Graph>();
		uf::graph::destroy( graph );
	//	this->deleteComponent<pod::Graph>();
	//	UF_MSG_DEBUG("Destroying graph: " << this->getName() << ": " << this->getUid());
	}
	if ( this->hasComponent<uf::Atlas>() ) {
		auto& atlas = this->getComponent<uf::Atlas>();
		atlas.clear();
	//	this->deleteComponent<uf::Atlas>();
	//	UF_MSG_DEBUG("Destroying atlas: " << this->getName() << ": " << this->getUid());
	}
#if UF_USE_VULKAN
	if ( this->hasComponent<uf::renderer::RenderTargetRenderMode>() ) {
		auto& renderMode = this->getComponent<uf::renderer::RenderTargetRenderMode>();
		uf::renderer::removeRenderMode( &renderMode, false );
		renderMode.destroy();
	//	this->deleteComponent<uf::renderer::RenderTargetRenderMode>();
	//	UF_MSG_DEBUG("Destroying render mode: " << this->getName() << ": " << this->getUid());
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
	// listen for metadata file changes
#if UF_ENTITY_METADATA_USE_JSON
	auto& metadataJson = this->getComponent<uf::Serializer>();
#if !UF_ENV_DREAMCAST
	if ( metadataJson["system"]["hot reload"]["enabled"].as<bool>() ) {
		size_t mtime = uf::io::mtime( metadataJson["system"]["source"].as<uf::stl::string>() );
		if ( metadataJson["system"]["hot reload"]["mtime"].as<size_t>() < mtime ) {
			metadataJson["system"]["hot reload"]["mtime"] = mtime;
			this->reload();
		}
	}
#endif
	// Call queued hooks
	{
		auto& queue = metadataJson["system"]["hooks"]["queue"];
		if ( !uf::Object::timer.running() ) uf::Object::timer.start();
		float curTime = uf::Object::timer.elapsed().asDouble();
		uf::Serializer newQueue = ext::json::array();
		if ( !ext::json::isNull( queue ) ) {
			ext::json::forEach(queue, [&](ext::json::Value& member){
				if ( !ext::json::isObject(member) ) return;
				uf::Serializer payload = member["payload"];
				uf::stl::string name = member["name"].as<uf::stl::string>();
				float timeout = member["timeout"].as<float>();
				if ( timeout < curTime ) this->callHook( name, payload );
				else newQueue.emplace_back(member);
			});
		}
		if ( ext::json::isObject( metadataJson ) ) queue = newQueue;
	}
#else
	auto& metadata = this->getComponent<uf::ObjectBehavior::Metadata>();

	if ( metadata.hotReload.enabled ) {
		size_t mtime = uf::io::mtime( metadata.hotReload.source );
		if ( metadata.hotReload.mtime < mtime ) {
			metadata.hotReload.mtime = mtime;
			this->reload();
		}
	}

	if ( metadata.transform.trackParent && this->hasParent() ) {
		auto& parent = this->getParent();
		auto& transform = this->getComponent<pod::Transform<>>();
		auto& parentTransform = parent.getComponent<pod::Transform<>>();
		transform.position = uf::transform::flatten( parentTransform ).position + metadata.transform.initial.position;
	}

	auto& queue = metadata.hooks.queue;
	if ( !uf::Object::timer.running() ) uf::Object::timer.start();
	float curTime = uf::Object::timer.elapsed().asDouble();
#if 1
	decltype(metadata.hooks.queue) unprocessed;
	unprocessed.reserve( metadata.hooks.queue.size() );

	decltype(metadata.hooks.queue) executeQueue;
	executeQueue.reserve( metadata.hooks.queue.size() );

	for ( auto& q : queue ) if ( q.timeout < curTime ) executeQueue.emplace_back(q); else unprocessed.emplace_back(q);
	for ( auto& q : executeQueue ) this->callHook( q.name, q.payload );
	queue = std::move(unprocessed);
#else
	for ( auto it = queue.begin(); it != queue.end(); ) {
		auto& q = *it;
		if ( q.timeout < curTime ) { this->callHook( q.name, q.payload ); it = queue.erase( it ); }
		else ++it;
	}
#endif
#endif
}
void uf::ObjectBehavior::render( uf::Object& self ) {}
#undef this
UF_BEHAVIOR_ENTITY_CPP_END(Object)