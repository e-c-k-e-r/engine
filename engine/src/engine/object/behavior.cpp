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

	this->addHook( "object:UpdateMetadata.%UID%", [&](const std::string& event)->std::string{	
		uf::Serializer json = event;
		metadata.merge(json, true);
		return "true";
	/*
		std::string keyString = match[1].str();
		std::string valueString = match[2].str();
		auto keys = uf::string::split(keyString, ".");
		uf::Serializer value; value.deserialize(valueString);
		Json::Value* traversal = &::config;
		for ( auto& key : keys ) {
			traversal = &((*traversal)[key]);
		}
		*traversal = value;
	*/
	});
	this->addHook( "asset:QueueLoad.%UID%", [&](const std::string& event)->std::string{	
		uf::Serializer json = event;
		std::string filename = json["filename"].as<std::string>();
		std::string callback = this->formatHookName("asset:FinishedLoad.%UID%");
		if ( json["single threaded"].as<bool>() ) {
			assetLoader.load( filename );
			this->queueHook( callback, event );
		} else {
			assetLoader.load( filename, callback );
		}

		return "true";
	});
	this->addHook( "asset:FinishedLoad.%UID%", [&](const std::string& event)->std::string{
		this->queueHook("asset:Load.%UID%", event);
		this->queueHook("asset:Parsed.%UID%", event);
		return "true";
	});	
	this->addHook( "asset:Load.%UID%", [&](const std::string& event)->std::string{	
		uf::Serializer json = event;
		std::string filename = json["filename"].as<std::string>();
		bool initialize = ext::json::isNull( json["initialize"] ) ? true : json["initialize"].as<bool>();
		
		if ( uf::io::extension(filename) != "json" ) return "false";

		if ( !json.readFromFile(filename) ) {
			uf::iostream << "Error @ " << __FILE__ << ":" << __LINE__ << ": failed to open `" + filename + "`" << "\n";
			return "false";
		}
		json["root"] = uf::io::directory(filename);
		json["source"] = filename; // uf::io::filename(filename)
		json["hot reload"]["mtime"] = uf::io::mtime( filename ) + 10;

		if ( this->loadChildUid(json, initialize) == -1 ) return "false";

		return "true";
	});
	this->addHook( "asset:Parsed.%UID%", [&](const std::string& event)->std::string{	
		uf::Serializer json = event;
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
	/*
		float portion = 1.0f / metadata["system"]["load"]["total"].as<float>();
		auto& progress = metadata["system"]["load"]["progress"];
		progress = progress.as<float>() + portion;
	*/
	/*
		if ( metadata["system"]["loaded"].as<bool>() ) return "false";
		if ( progress.as<float>() >= 1.0f ) {
			this->queueHook("system:Load.Finished.%UID%");
		}
	*/
		return "true";
	});
}
void uf::ObjectBehavior::destroy( uf::Object& self ) {
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	for( Json::Value::iterator it = metadata["system"]["hooks"]["alloc"].begin() ; it != metadata["system"]["hooks"]["alloc"].end() ; ++it ) {
	 	std::string name = it.key().as<std::string>();
		for ( size_t i = 0; i < metadata["system"]["hooks"]["alloc"][name].size(); ++i ) {
			size_t id = metadata["system"]["hooks"]["alloc"][name][(int) i].as<size_t>();
			uf::hooks.removeHook(name, id);
		}
	}

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
}
void uf::ObjectBehavior::tick( uf::Object& self ) {
	// listen for metadata file changes
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	if ( metadata["system"]["hot reload"]["enabled"].as<bool>() ) {
		size_t mtime = uf::io::mtime( metadata["system"]["source"].as<std::string>() );
		if ( metadata["system"]["hot reload"]["mtime"].as<size_t>() < mtime ) {
			std::cout << "File reload detected: " << ": " << metadata["system"]["source"].as<std::string>() << ", " << metadata["system"]["hot reload"]["mtime"] << " -> " << mtime << std::endl;
			metadata["system"]["hot reload"]["mtime"] = mtime;
			this->reload();
			//this->queueHook("metadata:Reload.%UID%");
		}
	}

	// Call queued hooks
	{
		if ( !uf::Object::timer.running() ) uf::Object::timer.start();
		float curTime = uf::Object::timer.elapsed().asDouble();
		uf::Serializer newQueue = Json::Value(Json::arrayValue);
		for ( auto& member : metadata["system"]["hooks"]["queue"] ) {
			uf::Serializer payload = member["payload"];
			std::string name = member["name"].as<std::string>();
			float timeout = member["timeout"].as<float>();
			if ( timeout < curTime ) {
				this->callHook( name, payload );
			} else {
				newQueue.append(member);
			}
		}
		if ( ext::json::isObject( metadata ) ) metadata["system"]["hooks"]["queue"] = newQueue;
	}
}
void uf::ObjectBehavior::render( uf::Object& self ) {
	auto& metadata = this->getComponent<uf::Serializer>();
/*
	if ( ext::json::isNull( metadata["system"]["type"] ) || metadata["system"]["defaults"]["render"].as<bool>() ) {
		if ( this->hasComponent<uf::Graphic>() ) {
			auto& scene = uf::scene::getCurrentScene();
			auto& graphic = this->getComponent<uf::Graphic>();
			auto& transform = this->getComponent<pod::Transform<>>();
			auto& controller = scene.getController();
			auto& camera = controller.getComponent<uf::Camera>();		
			
			if ( !graphic.initialized ) return;

			auto& uniforms = graphic.material.shaders.front().uniforms.front().get<uf::StereoMeshDescriptor>();
			uniforms.matrices.model = uf::transform::model( transform );
			for ( std::size_t i = 0; i < 2; ++i ) {
				uniforms.matrices.view[i] = camera.getView( i );
				uniforms.matrices.projection[i] = camera.getProjection( i );
			}

			uniforms.color[0] = 1;
			uniforms.color[1] = 1;
			uniforms.color[2] = 1;
			uniforms.color[3] = 1;

			graphic.material.shaders.front().updateBuffer( uniforms, 0, false );
		};
	}
*/
}
#undef this
UF_BEHAVIOR_ENTITY_CPP_END(Object)