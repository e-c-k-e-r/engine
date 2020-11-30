#include <uf/engine/object/object.h>
#include <uf/engine/asset/asset.h>
#include <uf/engine/scene/scene.h>
#include <uf/utils/time/time.h>
#include <uf/utils/math/transform.h>
#include <uf/utils/math/physics.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/graphic/mesh.h>
#include <uf/ext/gltf/gltf.h>

uf::Timer<long long> uf::Object::timer(false);
namespace {
	uf::Object null;
}

UF_OBJECT_REGISTER_BEGIN(uf::Object)
	UF_OBJECT_REGISTER_BEHAVIOR(uf::EntityBehavior)
	UF_OBJECT_REGISTER_BEHAVIOR(uf::ObjectBehavior)
UF_OBJECT_REGISTER_END()
uf::Object::Object() UF_BEHAVIOR_ENTITY_CPP_ATTACH(uf::Object)
void uf::Object::queueHook( const std::string& name, const std::string& payload, double timeout ) {
	if ( !uf::Object::timer.running() ) uf::Object::timer.start();
	float start = uf::Object::timer.elapsed().asDouble();
	uf::Serializer queue;
	queue["name"] = name;
	queue["payload"] = uf::Serializer{payload};
	queue["timeout"] = start + timeout;
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	metadata["system"]["hooks"]["queue"].emplace_back(queue);
}
std::string uf::Object::formatHookName( const std::string& n, size_t uid, bool fetch ) {
	if ( fetch ) {
		uf::Object* object = (uf::Object*) uf::Entity::globalFindByUid( uid );
		if ( object ) return object->formatHookName( n );
	}
	std::unordered_map<std::string, std::string> formats = {
		{"%UID%", std::to_string(uid)},
	};
	std::string name = n;
	for ( auto& pair : formats ) {
		name = uf::string::replace( name, pair.first, pair.second );
	}
	return name;
}
std::string uf::Object::formatHookName( const std::string& n ) {
	size_t uid = this->getUid();
	size_t parent = uid;
	if ( this->hasParent() ) {
		parent = this->getParent().getUid();
	}
	std::unordered_map<std::string, std::string> formats = {
		{"%UID%", std::to_string(uid)},
		{"%P-UID%", std::to_string(parent)},
	};
	std::string name = n;
	for ( auto& pair : formats ) {
		name = uf::string::replace( name, pair.first, pair.second );
	}
	return name;
}
std::vector<std::string> uf::Object::callHook( const std::string& name, const std::string& payload ) {
	std::vector<std::string> strings;
	auto results = uf::hooks.call( this->formatHookName( name ), payload );
	for ( auto& result : results ) {
		if ( result.is<std::string>() ) {
			strings.emplace_back( result.as<std::string>() );
		}
	}
	return strings;
}
std::size_t uf::Object::addHook( const std::string& name, const uf::HookHandler::Readable::function_t& callback ) {
	std::string parsed = this->formatHookName( name );
	std::size_t id = uf::hooks.addHook( parsed, callback );
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	metadata["system"]["hooks"]["alloc"][parsed].emplace_back(id);
	return id;
}

bool uf::Object::load( const std::string& f, bool inheritRoot ) {
	uf::Serializer json;
	std::string root = "";
	if ( inheritRoot && this->hasParent() ) {
		auto& parent = this->getParent<uf::Object>();
		uf::Serializer& metadata = parent.getComponent<uf::Serializer>();
		root = metadata["system"]["root"].as<std::string>();
	}
	std::string filename = grabURI( f, root );
	if ( !json.readFromFile( filename ) ) {
	//	uf::iostream << "Error @ " << __FILE__ << ":" << __LINE__ << ": failed to open `" + filename + "`" << "\n";
		return false;
	}
	json["root"] = uf::io::directory(filename);
	json["source"] = filename; // uf::io::filename(filename);
	json["hot reload"]["mtime"] = uf::io::mtime(filename) + 10;
	return this->load(json);
}

bool uf::Object::reload( bool hard ) {
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	if ( !metadata["system"]["source"].is<std::string>() ) return false;
	uf::Serializer json;
	uf::Serializer payload;
	std::string filename = metadata["system"]["source"].as<std::string>(); //grabURI( metadata["system"]["source"].as<std::string>(), metadata["system"]["root"].as<std::string>() ); // uf::io::sanitize(metadata["system"]["source"].as<std::string>(), metadata["system"]["root"].as<std::string>());
	if ( !json.readFromFile( filename ) ) {
	////	uf::iostream << "Error @ " << __FILE__ << ":" << __LINE__ << ": failed to open `" + filename + "`" << "\n";
	//	uf::iostream << this << ": " << this->getName() << ": " << this->getUid() << ": " << metadata << "\n";
		return false;
	}
	if ( hard ) return this->load(filename);

	payload["old"] = metadata;
	for ( auto it = json["metadata"].begin(); it != json["metadata"].end(); ++it ) {
		metadata[it.key()] = json["metadata"][it.key()];
	}
	payload["new"] = metadata;
	std::cout << "Updated metadata for " << this->getName() << ": " << this->getUid() << std::endl;

	this->queueHook("object:Reload.%UID%", payload);
	return true;
}

bool uf::Object::load( const uf::Serializer& _json ) {
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	uf::Serializer json = _json;
	// import
	if ( json["import"].is<std::string>() || json["include"].is<std::string>() ) {
		uf::Serializer chain = json;
		std::string root = json["root"].as<std::string>();
		do {
			std::string filename = chain["import"].is<std::string>() ? chain["import"].as<std::string>() : chain["include"].as<std::string>();
			filename = grabURI( filename, root );
			chain.readFromFile( filename );
			// set new root
			root = uf::io::directory( filename );
			// merge table
			json.import( chain );
		/*
			json["import"] = Json::nullValue;
			json["include"] = Json::nullValue;
			chain.merge( json, true );
			json = chain;
		*/
		} while ( chain["import"].is<std::string>() || chain["include"].is<std::string>() );
		if ( !ext::json::isArray(_json["assets"]) || _json["assets"].size() == 0 ) json["root"] = root;
	}
	// copy system table to base
	for ( auto it = json["system"].begin(); it != json["system"].end(); ++it ) {
		std::string key = it.key();
		if ( ext::json::isNull( json[key] ) )
			json[key] = json["system"][key];
	}
	json["hot reload"]["enabled"] = json["system"]["hot reload"]["enabled"];
	// Basic entity information
	{
		// Set name
		this->m_name = json["name"].is<std::string>() ? json["name"].as<std::string>() : json["type"].as<std::string>();
	}
	// Bind behaviors
	if ( json["type"].is<std::string>() ) {
		uf::instantiator::bind( json["type"].as<std::string>(), *this );
	}
	if ( json["defaults"]["render"].as<bool>() ) {
		uf::instantiator::bind( "RenderBehavior", *this );
	}
	if ( json["defaults"]["asset load"].as<bool>() ) {
		uf::instantiator::bind( "GltfBehavior", *this );
	}
	{
		uf::Serializer target;
		if ( ext::json::isArray( metadata["system"]["behaviors"] ) ) {
			target = metadata["system"]["behaviors"];
		} else if ( ext::json::isArray( json["behaviors"] ) ) {
			target = json["behaviors"];
		}
		for ( uint i = 0; i < target.size(); ++i ) {
			uf::instantiator::bind( target[i].as<std::string>(), *this );
		}
	}
	// Set transform
	{
		bool load = ext::json::isObject( json["transform"] );
		if ( this->hasComponent<pod::Transform<>>() ) load = false;
		pod::Transform<>& transform = this->getComponent<pod::Transform<>>();
		if ( transform.position.x == 0 && transform.position.y == 0 && transform.position.z == 0 ) {
			load = true;
		}
		if ( load ) {
			transform.position.x = json["transform"]["position"][0].as<float>();
			transform.position.y = json["transform"]["position"][1].as<float>();
			transform.position.z = json["transform"]["position"][2].as<float>();
			transform.orientation = uf::quaternion::identity();
			if ( ext::json::isArray( json["transform"]["orientation"] ) ) {
				transform.orientation.x = json["transform"]["orientation"][0].as<float>();
				transform.orientation.y = json["transform"]["orientation"][1].as<float>();
				transform.orientation.z = json["transform"]["orientation"][2].as<float>();
				transform.orientation.w = json["transform"]["orientation"][3].as<float>();
			} else if ( json["transform"]["rotation"]["angle"].as<float>() != 0 ) {
				transform.orientation = uf::quaternion::axisAngle( {
					json["transform"]["rotation"]["axis"][0].as<float>(),
					json["transform"]["rotation"]["axis"][1].as<float>(),
					json["transform"]["rotation"]["axis"][2].as<float>()
				}, 
					json["transform"]["rotation"]["angle"].as<float>()
				);
			}
			if ( ext::json::isArray( json["transform"]["scale"] ) ) {
				transform.scale = uf::vector::create( json["transform"]["scale"][0].as<float>(),json["transform"]["scale"][1].as<float>(),json["transform"]["scale"][2].as<float>() );
			}
			transform = uf::transform::reorient( transform );
			if ( json["transform"]["reference"].as<std::string>() == "parent" ) {
				auto& parent = this->getParent().as<uf::Object>();
				transform.reference = &parent.getComponent<pod::Transform<>>();
			}
		}
	}
	// Set movement
	{
		if ( ext::json::isObject( json["physics"] ) && !this->hasComponent<pod::Physics>() ) {
			auto& physics = this->getComponent<pod::Physics>();
			if ( ext::json::isArray( json["physics"]["linear"]["velocity"] ) )
				for ( uint j = 0; j < 3; ++j )
					physics.linear.velocity[j] = json["physics"]["linear"]["velocity"][j].as<float>();
			if ( ext::json::isArray( json["physics"]["linear"]["acceleration"] ) )
				for ( uint j = 0; j < 3; ++j )
					physics.linear.acceleration[j] = json["physics"]["linear"]["acceleration"][j].as<float>();
			
			if ( ext::json::isArray( json["physics"]["rotational"]["velocity"] ) )
				for ( uint j = 0; j < 4; ++j )
					physics.rotational.velocity[j] = json["physics"]["rotational"]["velocity"][j].as<float>();
			if ( ext::json::isArray( json["physics"]["rotational"]["acceleration"] ) )
				for ( uint j = 0; j < 4; ++j )
					physics.rotational.acceleration[j] = json["physics"]["rotational"]["acceleration"][j].as<float>();
		}
	}

	uf::Scene& scene = uf::scene::getCurrentScene();
	uf::Asset& assetLoader = scene.getComponent<uf::Asset>();
	// initialize base entity, needed for asset hooks

	#define UF_OBJECT_LOAD_ASSET_HEADER(type)\
		bool addBehavior = false;\
		uf::Serializer target;\
		if ( ext::json::isObject( metadata["system"]["assets"] ) ) {\
			target = metadata["system"]["assets"];\
		} else if ( ext::json::isArray( json["assets"] ) ) {\
			target = json["assets"];\
		} else if ( ext::json::isObject( json["assets"] ) && !ext::json::isNull( json["assets"][#type] )  ) {\
			target = json["assets"][#type];\
		}

	#define UF_OBJECT_LOAD_ASSET(...)\
		std::string canonical = "";\
		std::string f = ext::json::isObject( target[i] ) ? target[i]["filename"].as<std::string>() : target[i].as<std::string>();\
		float delay = ext::json::isObject( target[i] ) ? target[i]["delay"].as<float>() : 0;\
		std::string filename = grabURI( f, json["root"].as<std::string>() );\
		bool singleThreaded = ext::json::isObject( target[i] ) ? target[i]["single threaded"].as<bool>() : false;\
		std::vector<std::string> allowedExtensions = {__VA_ARGS__};\
		if (  std::find( allowedExtensions.begin(), allowedExtensions.end(), uf::io::extension(filename) ) == allowedExtensions.end() ) continue;\
		uf::Serializer payload;\
		payload["filename"] = filename;\
		payload["single threaded"] = singleThreaded;\
		this->queueHook( "asset:QueueLoad.%UID%", payload, delay );\
		addBehavior = true;

	// Audio
	{
		// find first valid texture in asset list
		UF_OBJECT_LOAD_ASSET_HEADER(audio)
		for ( uint i = 0; i < target.size(); ++i ) {
			UF_OBJECT_LOAD_ASSET("ogg")
		}
	}

	// Texture
	{
		// find first valid texture in asset list
		UF_OBJECT_LOAD_ASSET_HEADER(textures)
		for ( uint i = 0; i < target.size(); ++i ) {
			UF_OBJECT_LOAD_ASSET("png")
		}
	}

	// gltf
	{
		// find first valid texture in asset list
		UF_OBJECT_LOAD_ASSET_HEADER(models)
		for ( uint i = 0; i < target.size(); ++i ) {
			UF_OBJECT_LOAD_ASSET("gltf", "glb")
			
			auto& aMetadata = assetLoader.getComponent<uf::Serializer>();
			aMetadata[filename] = json["metadata"]["model"];
		}
		if ( addBehavior ) uf::instantiator::bind( "GltfBehavior", *this );
	}
	// lua
	{
		// find first valid texture in asset list
		UF_OBJECT_LOAD_ASSET_HEADER(scripts)
		for ( uint i = 0; i < target.size(); ++i ) {
			UF_OBJECT_LOAD_ASSET("lua")
		}
		if ( addBehavior ) uf::instantiator::bind( "LuaBehavior", *this );
	}

	uf::Serializer hooks = metadata["system"]["hooks"];
	// Metadata
	if ( !ext::json::isNull( json["metadata"] ) ) {
		uf::Serializer& metadata = this->getComponent<uf::Serializer>();
		if ( json["metadata"].is<std::string>() ) {
			std::string f = json["metadata"].as<std::string>();
			std::string filename = grabURI( json["metadata"].as<std::string>(), json["root"].as<std::string>() );
			if ( !metadata.readFromFile(filename) ) {
			//	uf::iostream << "Error @ " << __FILE__ << ":" << __LINE__ << ": failed to open `" + filename + "`" << "\n";
				return false;
			}
		} else {
			metadata = json["metadata"];
		}
	}
	metadata["system"] = json;
	metadata["system"].erase("metadata");
	metadata["system"]["hooks"] = hooks;
/*
	for ( auto it = json["system"].begin(); it != json["system"].end(); ++it ) {
		if ( ext::json::isNull( metadata["system"][it.key()] ) )
			metadata["system"][it.key()] = json["system"][it.key()];
	}
*/

	// check for children
	{
		UF_OBJECT_LOAD_ASSET_HEADER(entities)
		for ( uint i = 0; i < target.size(); ++i ) {
			std::string canonical = "";
			std::string f = ext::json::isObject( target[i] ) ? target[i]["filename"].as<std::string>() : target[i].as<std::string>();
			float delay = ext::json::isObject( target[i] ) ? target[i]["delay"].as<float>() : -1;
			std::string filename = grabURI( f, json["root"].as<std::string>() );
			if ( uf::io::extension(filename) != "json" ) continue;
			if ( (canonical = assetLoader.load( filename )) == "" ) continue;
			if ( delay > -1 ) {
				uf::Serializer payload;
				payload["filename"] = canonical;
				this->queueHook( "asset:Load.%UID%", payload, delay );
				continue;
			}
			{
				uf::Serializer json;
				if ( !json.readFromFile(filename) ) {
				//	uf::iostream << "Error @ " << __FILE__ << ":" << __LINE__ << ": failed to open `" + filename + "`" << "\n";
					continue;
				}
				json["root"] = uf::io::directory(filename);
				json["source"] = filename; // uf::io::filename(filename)
				json["hot reload"]["mtime"] = uf::io::mtime( filename ) + 10;

				if ( this->loadChildUid(json) == -1 ) continue;
			}
		}
	}
	// Add lights
	if ( ext::json::isArray( metadata["system"]["lights"] ) ) {
		uf::Serializer target = metadata["system"]["lights"];
		auto& pTransform = this->getComponent<pod::Transform<>>();
		for ( uint i = 0; i < target.size(); ++i ) {
			uf::Serializer json = target[i];
			std::vector<pod::Vector4f> orientations;
			if ( json["ignore"].as<bool>() ) continue;
			if ( ext::json::isNull( json["transform"]["orientation"] ) && json["shadows"]["fov"].as<float>() == 0.0f ) {
				orientations.reserve(6);
				orientations.push_back({0, 0, 0, -1});
				orientations.push_back({0, 0.707107, 0, -0.707107});
				orientations.push_back({0, 1, 0, 0});
				orientations.push_back({0, 0.707107, 0, 0.707107});
				orientations.push_back({-0.707107, 0, 0, -0.707107});
				orientations.push_back({0.707107,  0, 0, -0.707107});
				json["shadows"]["fov"] = 90.0f;
			} else {
				pod::Vector4f orientation;
				if ( ext::json::isArray( json["transform"]["orientation"] ) ) {
					orientation = pTransform.orientation;
				} else {
					for ( uint j = 0; j < 4; ++j ) orientation[j] = json["transform"]["orientation"][j].as<float>();
				}
				orientations.push_back(orientation);
			}
			uf::Object* target = this;
			for ( auto& orientation : orientations ) {
				std::string fname = "/light.json";
				if ( json["system"]["filename"].is<std::string>() ) fname = json["system"]["filename"].as<std::string>();
				auto* light = target->loadChildPointer(fname, false);
				if ( target == this ) target = light;
				auto& metadata = light->getComponent<uf::Serializer>();
				auto& transform = light->getComponent<pod::Transform<>>();
				if ( ext::json::isArray( json["transform"]["position"] ) ) {
					for ( uint j = 0; j < 3; ++j )
						transform.position[j] = json["transform"]["position"][j].as<float>();
				} else {
					transform.position = pTransform.position;
				}
				for ( uint j = 0; j < 4; ++j )
					transform.orientation[j] = orientation[j];

				if ( !ext::json::isNull( json["color"] ) ) metadata["light"]["color"] = json["color"];
				if ( !ext::json::isNull( json["radius"] ) ) metadata["light"]["radius"] = json["radius"];
				if ( !ext::json::isNull( json["power"] ) ) metadata["light"]["power"] = json["power"];
				if ( !ext::json::isNull( json["shadows"] ) ) metadata["light"]["shadows"] = json["shadows"];
				if ( !ext::json::isNull( json["flicker"] ) ) metadata["light"]["flicker"] = json["flicker"];
				if ( !ext::json::isNull( json["fade"] ) ) metadata["light"]["fade"] = json["fade"];

				if ( !ext::json::isNull( json["system"]["track"] ) ) metadata["system"]["track"] = json["system"]["track"];

				light->initialize();
			}
		}
	}

	return true;
}
uf::Object& uf::Object::loadChild( const std::string& f, bool initialize ) {
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	uf::Serializer json;
	std::string filename = grabURI( f, metadata["system"]["root"].as<std::string>() );
	if ( !json.readFromFile(filename) ) {
	//	uf::iostream << "Error @ " << __FILE__ << ":" << __LINE__ << ": failed to open `" + filename + "`" << "\n";
		return ::null;
	}
	json["root"] = uf::io::directory(filename);
	json["source"] = filename; //uf::io::filename(filename);
	json["hot reload"]["mtime"] = uf::io::mtime( filename ) + 10;
	return this->loadChild(json, initialize);
}
uf::Object& uf::Object::loadChild( const uf::Serializer& _json, bool initialize ) {
	uf::Serializer json = _json;
	std::string type = json["type"].as<std::string>();
	if ( type == "" ) type = "Object";
	if ( json["ignore"].as<bool>() ) return ::null;

	uf::Entity& entity = uf::instantiator::instantiate(type);
	uf::Object& object = entity.as<uf::Object>();
	this->addChild(entity);
	if ( !object.load(json) ) {
	//	uf::iostream << "Error @ " << __FILE__ << ":" << __LINE__ << " loading `" << json << "!" << "\n";
		this->removeChild(entity);
		return ::null;
	}
	if ( initialize ) entity.initialize();
	return object;
}
uf::Object* uf::Object::loadChildPointer( const std::string& f, bool initialize ) {
	uf::Object* pointer = &this->loadChild(f, initialize);
	return pointer != &::null ? pointer : NULL;
}
uf::Object* uf::Object::loadChildPointer( const uf::Serializer& json, bool initialize ) {
	uf::Object* pointer = &this->loadChild(json, initialize);
	return pointer != &::null ? pointer : NULL;
}
std::size_t uf::Object::loadChildUid( const std::string& f, bool initialize ) {
	uf::Object* pointer = this->loadChildPointer(f, initialize);
	return pointer ? pointer->getUid() : -1;
}
std::size_t uf::Object::loadChildUid( const uf::Serializer& json, bool initialize ) {
	uf::Object* pointer = this->loadChildPointer(json, initialize);
	return pointer ? pointer->getUid() : -1;
}

std::string uf::Object::grabURI( const std::string& filename, const std::string& root  ) {
	return uf::io::resolveURI( filename, root );
}


#include <uf/utils/string/ext.h>
std::string uf::string::toString( const uf::Object& object ) {
	std::stringstream ss;
	ss << object.getName() << ": " << object.getUid();
	return ss.str();
}