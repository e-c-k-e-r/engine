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
#define this (&self)
void uf::ObjectBehavior::initialize( uf::Object& self ) {
	auto& scene = uf::scene::getCurrentScene();
	auto& assetLoader = scene.getComponent<uf::Asset>();
	auto& metadata = this->getComponent<uf::Serializer>();

	// 
	{
		size_t assets = metadata["system"]["assets"].size();
		if ( metadata["system"]["load"]["ignore"].is<bool>() ) assets = 0;
		metadata["system"]["load"]["progress"] = 0;
		metadata["system"]["load"]["total"] = assets;
		if ( assets == 0 )  {
			auto& parent = this->getParent().as<uf::Object>();
			uf::Serializer payload;
			payload["uid"] = this->getUid();
			parent.callHook("asset:Parsed.%UID%", payload);
		}
	}

#if UF_USE_BULLET
	if ( ext::json::isObject(metadata["system"]["physics"]) ) {
		float mass = metadata["system"]["physics"]["mass"].as<float>();
		if ( metadata["system"]["physics"]["type"].as<std::string>() == "BoundingBox" ) {
			pod::Vector3f corner = uf::vector::decode( metadata["system"]["physics"]["corner"], pod::Vector3f{0.5, 0.5, 0.5} );
			ext::bullet::create( *this, corner, mass );
		} else if ( metadata["system"]["physics"]["type"].as<std::string>() == "Capsule" ) {
			float radius = metadata["system"]["physics"]["radius"].as<float>();
			float height = metadata["system"]["physics"]["height"].as<float>();
			ext::bullet::create( *this, radius, height, mass );
		} else {
			return;
		}

		auto& collider = this->getComponent<pod::Bullet>();
		if ( !ext::json::isNull( metadata["system"]["physics"]["gravity"] ) ) {
			collider.body->setGravity( btVector3(
				metadata["system"]["physics"]["gravity"][0].as<float>(),
				metadata["system"]["physics"]["gravity"][1].as<float>(),
				metadata["system"]["physics"]["gravity"][2].as<float>()
			) );
			if ( metadata["system"]["physics"]["shared"].is<bool>() ) {
				collider.shared = metadata["system"]["physics"]["shared"].as<bool>();
			}
		}
	}
#endif

	this->addHook( "object:TransformReferenceController.%UID%", [&](ext::json::Value& json){
		auto& transform = this->getComponent<pod::Transform<>>();
		auto& controller = scene.getController();
		if ( json["target"].as<std::string>() == "camera" ) {
			auto& camera = controller.getComponent<uf::Camera>();
			transform.reference = &camera.getTransform();
		} else {
			transform.reference = &controller.getComponent<pod::Transform<>>();
		}
	});
	this->addHook( "object:UpdateMetadata.%UID%", [&](ext::json::Value& json){	
		if ( ext::json::isNull( json ) ) return;
		if ( json["type"].as<std::string>() == "merge" ) {
			metadata.merge(json["value"], true);
		} else if ( json["type"].as<std::string>() == "import" ) {
			metadata.import(json["value"]);
		} else if ( json["path"].is<std::string>() ) {
			metadata.path(json["path"].as<std::string>()) = json["value"];
		} else {
			metadata.merge(json, true);
		}
	});
	this->addHook( "asset:QueueLoad.%UID%", [&](ext::json::Value& json){
		std::string filename = json["filename"].as<std::string>();
		std::string hash = json["hash"].as<std::string>();
		std::string category = json["category"].as<std::string>();
		std::string callback = this->formatHookName("asset:FinishedLoad.%UID%");
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
		std::string filename = json["filename"].as<std::string>();
		std::string category = json["category"].as<std::string>();
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
		auto& total = metadata["system"]["load"]["total"];
		auto& progress = metadata["system"]["load"]["progress"];
		progress = progress.as<int>() + portion;
		if ( progress.as<int>() == total.as<int>() ) {
			auto& parent = this->getParent().as<uf::Object>();

			uf::Serializer payload;
			payload["uid"] = this->getUid();
			parent.callHook("asset:Parsed.%UID%", payload);
		}
	});
}
void uf::ObjectBehavior::destroy( uf::Object& self ) {
#if UF_ENTITY_METADATA_USE_JSON
	auto& metadata = this->getComponent<uf::Serializer>();
	ext::json::forEach( metadata["system"]["hooks"]["bound"], [&]( const std::string& key, ext::json::Value& value ){
		ext::json::forEach( value, [&]( ext::json::Value& id ){
			uf::hooks.removeHook(key, id.as<size_t>());
		});
	});
/*
	for( auto it = metadata["system"]["hooks"]["bound"].begin() ; it != metadata["system"]["hooks"]["bound"].end() ; ++it ) {
	 	std::string name = it.key();
		for ( size_t i = 0; i < metadata["system"]["hooks"]["bound"][name].size(); ++i ) {
			size_t id = metadata["system"]["hooks"]["bound"][name][(int) i].as<size_t>();
			uf::hooks.removeHook(name, id);
		}
	}
*/
#else
	auto& metadata = this->getComponent<uf::ObjectBehavior::Metadata>();
	for ( auto pair : metadata.hooks.bound ) {
		for ( auto id : pair.second ) uf::hooks.removeHook(pair.first, id);
	}
#endif

	if ( this->hasComponent<uf::Audio>() ) {
		auto& audio = this->getComponent<uf::Audio>();
		audio.destroy();
	}
	if ( this->hasComponent<uf::SoundEmitter>() ) {
		auto& audio = this->getComponent<uf::SoundEmitter>();
		audio.cleanup(true);
	}
	if ( this->hasComponent<uf::Graphic>() ) {
		auto& graphic = this->getComponent<uf::Graphic>();
		graphic.destroy();
		uf::renderer::states::rebuild = true;
	}
	if ( this->hasComponent<uf::Atlas>() ) {
		auto& atlas = this->getComponent<uf::Atlas>();
		atlas.clear();
	}
}
void uf::ObjectBehavior::tick( uf::Object& self ) {
	// listen for metadata file changes
#if UF_ENTITY_METADATA_USE_JSON
	auto& metadataJson = this->getComponent<uf::Serializer>();
#if !UF_ENV_DREAMCAST
	if ( metadataJson["system"]["hot reload"]["enabled"].as<bool>() ) {
		size_t mtime = uf::io::mtime( metadataJson["system"]["source"].as<std::string>() );
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
				std::string name = member["name"].as<std::string>();
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