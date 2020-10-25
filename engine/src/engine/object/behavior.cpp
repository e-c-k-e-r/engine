#include <uf/engine/object/object.h>
#include <uf/engine/asset/asset.h>
#include <uf/engine/scene/scene.h>
#include <uf/utils/time/time.h>
#include <uf/utils/math/transform.h>
#include <uf/utils/math/physics.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/graphic/mesh.h>
#include <uf/utils/renderer/renderer.h>
#include <uf/ext/gltf/gltf.h>

UF_BEHAVIOR_ENTITY_CPP_BEGIN(Object)
#define this (&self)
void uf::ObjectBehavior::initialize( uf::Object& self ) {
	auto& scene = uf::scene::getCurrentScene();
	auto& assetLoader = scene.getComponent<uf::Asset>();
	auto& metadata = this->getComponent<uf::Serializer>();
/*	
	size_t target = 0;
	for ( int i = 0; i < metadata["system"]["assets"].size(); ++i ) {
		std::string filename = metadata["system"]["assets"][i].isObject() ? metadata["system"]["assets"][i]["filename"].asString() : metadata["system"]["assets"][i].asString();
		if ( uf::io::extension(filename) != "json" ) ++target;
	}
	if ( target > 0 ) {
		metadata["system"]["load"]["progress"] = 0;
		metadata["system"]["load"]["total"] = target;
	}
*/

	// 
	{
		size_t assets = metadata["system"]["assets"].size();
		if ( metadata["system"]["load"]["ignore"].isBool() ) assets = 0;
		metadata["system"]["load"]["progress"] = 0;
		metadata["system"]["load"]["total"] = assets;
		if ( assets == 0 )  {
			auto& parent = this->getParent().as<uf::Object>();
			uf::Serializer payload;
			payload["uid"] = this->getUid();
			parent.callHook("asset:Parsed.%UID%", payload);
		}
	}

	this->addHook( "asset:QueueLoad.%UID%", [&](const std::string& event)->std::string{	
		uf::Serializer json = event;
		std::string filename = json["filename"].asString();
		std::string callback = "asset:FinishedLoad." + std::to_string(this->getUid());
		if ( json["single threaded"].asBool() ) {
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
		std::string filename = json["filename"].asString();
		bool initialize = json["initialize"].isNull() ? true : json["initialize"].asBool();
		
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
		progress = progress.asInt() + portion;
		if ( progress.asInt() == total.asInt() ) {
			auto& parent = this->getParent().as<uf::Object>();

			uf::Serializer payload;
			payload["uid"] = this->getUid();
			parent.callHook("asset:Parsed.%UID%", payload);
		}
	/*
		float portion = 1.0f / metadata["system"]["load"]["total"].asFloat();
		auto& progress = metadata["system"]["load"]["progress"];
		progress = progress.asFloat() + portion;
	*/
	/*
		if ( metadata["system"]["loaded"].asBool() ) return "false";
		if ( progress.asFloat() >= 1.0f ) {
			this->queueHook("system:Load.Finished.%UID%");
		}
	*/
		return "true";
	});
}
void uf::ObjectBehavior::destroy( uf::Object& self ) {
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	for( Json::Value::iterator it = metadata["system"]["hooks"]["alloc"].begin() ; it != metadata["system"]["hooks"]["alloc"].end() ; ++it ) {
	 	std::string name = it.key().asString();
		for ( size_t i = 0; i < metadata["system"]["hooks"]["alloc"][name].size(); ++i ) {
			size_t id = metadata["system"]["hooks"]["alloc"][name][(int) i].asUInt();
			uf::hooks.removeHook(name, id);
		}
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
	if ( metadata["system"]["hot reload"]["enabled"].asBool() ) {
		size_t mtime = uf::io::mtime( metadata["system"]["source"].asString() );
		if ( metadata["system"]["hot reload"]["mtime"].asUInt64() < mtime ) {
			std::cout << "File reload detected: " << ": " << metadata["system"]["source"].asString() << ", " << metadata["system"]["hot reload"]["mtime"] << " -> " << mtime << std::endl;
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
			std::string name = member["name"].asString();
			float timeout = member["timeout"].asFloat();
			if ( timeout < curTime ) {
				this->callHook( name, payload );
			} else {
				newQueue.append(member);
			}
		}
		if ( metadata.isObject() ) metadata["system"]["hooks"]["queue"] = newQueue;
	}
}
void uf::ObjectBehavior::render( uf::Object& self ) {
	auto& metadata = this->getComponent<uf::Serializer>();
/*
	if ( metadata["system"]["type"].isNull() || metadata["system"]["defaults"]["render"].asBool() ) {
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