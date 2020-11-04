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

UF_OBJECT_REGISTER_BEGIN(Object)
	UF_OBJECT_REGISTER_BEHAVIOR(EntityBehavior)
	UF_OBJECT_REGISTER_BEHAVIOR(ObjectBehavior)
UF_OBJECT_REGISTER_END()
void uf::Object::queueHook( const std::string& name, const std::string& payload, double timeout ) {
	if ( !uf::Object::timer.running() ) uf::Object::timer.start();
	float start = uf::Object::timer.elapsed().asDouble();
	uf::Serializer queue;
	queue["name"] = name;
	queue["payload"] = uf::Serializer{payload};
	queue["timeout"] = start + timeout;
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	metadata["system"]["hooks"]["queue"].append(queue);
}
std::string uf::Object::formatHookName( const std::string& n ) {
	std::unordered_map<std::string, std::string> formats = {
		{"%UID%", std::to_string(this->getUid())},
		{"%P-UID%", std::to_string(this->getParent().getUid())},
	};
	std::string name = n;
	for ( auto& pair : formats ) {
		name = uf::string::replace( name, pair.first, pair.second );
	}
	return name;
}
std::vector<std::string> uf::Object::callHook( const std::string& name, const std::string& payload ) {
	return uf::hooks.call( this->formatHookName( name ), payload );
}
std::size_t uf::Object::addHook( const std::string& name, const uf::HookHandler::Readable::function_t& callback ) {
	std::string parsed = this->formatHookName( name );
	std::size_t id = uf::hooks.addHook( parsed, callback );
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	metadata["system"]["hooks"]["alloc"][parsed].append(id);
	return id;
}

bool uf::Object::load( const std::string& f, bool inheritRoot ) {
	uf::Serializer json;
	std::string root = "";
	if ( inheritRoot && this->hasParent() ) {
		auto& parent = this->getParent<uf::Object>();
		uf::Serializer& metadata = parent.getComponent<uf::Serializer>();
		root = metadata["system"]["root"].asString();
	}
	std::string filename = grabURI( f, root );
	if ( !json.readFromFile( filename ) ) {
		uf::iostream << "Error @ " << __FILE__ << ":" << __LINE__ << ": failed to open `" + filename + "`" << "\n";
		return false;
	}
	json["root"] = uf::io::directory(filename);
	json["source"] = filename; // uf::io::filename(filename);
	json["hot reload"]["mtime"] = uf::io::mtime(filename) + 10;
	return this->load(json);
}

bool uf::Object::reload( bool hard ) {
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	if ( !metadata["system"]["source"].isString() ) return false;
	uf::Serializer json;
	uf::Serializer payload;
	std::string filename = metadata["system"]["source"].asString(); //grabURI( metadata["system"]["source"].asString(), metadata["system"]["root"].asString() ); // uf::io::sanitize(metadata["system"]["source"].asString(), metadata["system"]["root"].asString());
	if ( !json.readFromFile( filename ) ) {
		uf::iostream << "Error @ " << __FILE__ << ":" << __LINE__ << ": failed to open `" + filename + "`" << "\n";
		uf::iostream << this << ": " << this->getName() << ": " << this->getUid() << ": " << metadata << "\n";
		return false;
	}
	if ( hard ) return this->load(filename);

	payload["old"] = metadata;
	for ( auto it = json["metadata"].begin(); it != json["metadata"].end(); ++it ) {
		metadata[it.key().asString()] = json["metadata"][it.key().asString()];
	}
	payload["new"] = metadata;
	std::cout << "Updated metadata for " << this->getName() << ": " << this->getUid() << std::endl;

	this->queueHook("object:Reload.%UID%", payload);
	return true;
}

bool uf::Object::load( const uf::Serializer& _json ) {
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	uf::Serializer json = _json;
	// copy system table to base
	for ( auto it = json["system"].begin(); it != json["system"].end(); ++it ) {
		std::string key = it.key().asString();
		if ( json[key].isNull() )
			json[key] = json["system"][key];
	}
	json["hot reload"]["enabled"] = json["system"]["hot reload"]["enabled"];
	// Basic entity information
	{
		// Set name
		this->m_name = json["name"].isString() ? json["name"].asString() : json["type"].asString();
	}
	// Bind behaviors
	if ( json["type"].isString() ) {
		uf::instantiator::bind( json["type"].asString(), *this );
	}
	if ( json["defaults"]["render"].asBool() ) {
		uf::instantiator::bind( "RenderBehavior", *this );
	}
	if ( json["defaults"]["asset load"].asBool() ) {
		uf::instantiator::bind( "GltfBehavior", *this );
	}
	{
		uf::Serializer target;
		if ( metadata["system"]["behaviors"].isArray() ) {
			target = metadata["system"]["behaviors"];
		} else if ( json["behaviors"].isArray() ) {
			target = json["behaviors"];
		}
		for ( uint i = 0; i < target.size(); ++i ) {
			uf::instantiator::bind( target[i].asString(), *this );
		}
	}
	// Set transform
	{
		bool load = json["transform"].isObject();
		if ( this->hasComponent<pod::Transform<>>() ) load = false;
		pod::Transform<>& transform = this->getComponent<pod::Transform<>>();
		if ( transform.position.x == 0 && transform.position.y == 0 && transform.position.z == 0 ) {
			load = true;
		}
		if ( load ) {
			transform.position.x = json["transform"]["position"][0].asFloat();
			transform.position.y = json["transform"]["position"][1].asFloat();
			transform.position.z = json["transform"]["position"][2].asFloat();
			transform.orientation = uf::quaternion::identity();
			if ( json["transform"]["orientation"].isArray() ) {
				transform.orientation.x = json["transform"]["orientation"][0].asFloat();
				transform.orientation.y = json["transform"]["orientation"][1].asFloat();
				transform.orientation.z = json["transform"]["orientation"][2].asFloat();
				transform.orientation.w = json["transform"]["orientation"][3].asFloat();
			} else if ( json["transform"]["rotation"]["angle"].asFloat() != 0 ) {
				transform.orientation = uf::quaternion::axisAngle( {
					json["transform"]["rotation"]["axis"][0].asFloat(),
					json["transform"]["rotation"]["axis"][1].asFloat(),
					json["transform"]["rotation"]["axis"][2].asFloat()
				}, 
					json["transform"]["rotation"]["angle"].asFloat()
				);
			}
			if ( json["transform"]["scale"].isArray() ) {
				transform.scale = uf::vector::create( json["transform"]["scale"][0].asFloat(),json["transform"]["scale"][1].asFloat(),json["transform"]["scale"][2].asFloat() );
			}
			transform = uf::transform::reorient( transform );
		}
	}
	// Set movement
	{
		if ( json["physics"].isObject() && !this->hasComponent<pod::Physics>() ) {
			auto& physics = this->getComponent<pod::Physics>();
			if ( json["physics"]["linear"]["velocity"].isArray() )
				for ( uint j = 0; j < 3; ++j )
					physics.linear.velocity[j] = json["physics"]["linear"]["velocity"][j].asFloat();
			if ( json["physics"]["linear"]["acceleration"].isArray() )
				for ( uint j = 0; j < 3; ++j )
					physics.linear.acceleration[j] = json["physics"]["linear"]["acceleration"][j].asFloat();
			
			if ( json["physics"]["rotational"]["velocity"].isArray() )
				for ( uint j = 0; j < 4; ++j )
					physics.rotational.velocity[j] = json["physics"]["rotational"]["velocity"][j].asFloat();
			if ( json["physics"]["rotational"]["acceleration"].isArray() )
				for ( uint j = 0; j < 4; ++j )
					physics.rotational.acceleration[j] = json["physics"]["rotational"]["acceleration"][j].asFloat();
		}
	}

	uf::Scene& scene = uf::scene::getCurrentScene();
	uf::Asset& assetLoader = scene.getComponent<uf::Asset>();
	// initialize base entity, needed for asset hooks

	#define UF_OBJECT_LOAD_ASSET(...)\
		std::string canonical = "";\
		std::string f = target[i].isObject() ? target[i]["filename"].asString() : target[i].asString();\
		float delay = target[i].isObject() ? target[i]["delay"].asFloat() : 0;\
		std::string filename = grabURI( f, json["root"].asString() );\
		bool singleThreaded = target[i].isObject() ? target[i]["single threaded"].asBool() : false;\
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
		bool addBehavior = false;
		uf::Serializer target;
		if ( metadata["system"]["assets"].isArray() ) {
			target = metadata["system"]["assets"];
		} else if ( json["assets"].isArray() ) {
			target = json["assets"];
		} else if ( json["assets"].isObject() && !json["assets"]["audio"].isNull()  ) {
			target = json["assets"]["audio"];
		}
		for ( uint i = 0; i < target.size(); ++i ) {
			UF_OBJECT_LOAD_ASSET("ogg")
		}
	}

	// Texture
	{
		// find first valid texture in asset list
		bool addBehavior = false;
		uf::Serializer target;
		if ( metadata["system"]["assets"].isArray() ) {
			target = metadata["system"]["assets"];
		} else if ( json["assets"].isArray() ) {
			target = json["assets"];
		} else if ( json["assets"].isObject() && !json["assets"]["textures"].isNull()  ) {
			target = json["assets"]["textures"];
		}
		for ( uint i = 0; i < target.size(); ++i ) {
			UF_OBJECT_LOAD_ASSET("png")
		}
	}

	// gltf
	{
		// find first valid texture in asset list
		bool addBehavior = false;
		uf::Serializer target;
		if ( metadata["system"]["assets"].isArray() ) {
			target = metadata["system"]["assets"];
		} else if ( json["assets"].isArray() ) {
			target = json["assets"];
		} else if ( json["assets"].isObject() && !json["assets"]["models"].isNull()  ) {
			target = json["assets"]["models"];
		}
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
		uf::Serializer target;
		bool addBehavior = false;
		if ( metadata["system"]["assets"].isArray() ) {
			target = metadata["system"]["assets"];
		} else if ( json["assets"].isArray() ) {
			target = json["assets"];
		} else if ( json["assets"].isObject() && !json["assets"]["scripts"].isNull()  ) {
			target = json["assets"]["scripts"];
		}
		for ( uint i = 0; i < target.size(); ++i ) {
			UF_OBJECT_LOAD_ASSET("lua")
		}
		if ( addBehavior ) uf::instantiator::bind( "LuaBehavior", *this );
	}

	uf::Serializer hooks = metadata["system"]["hooks"];
	// Metadata
	if ( json["metadata"] != Json::nullValue ) {
		uf::Serializer& metadata = this->getComponent<uf::Serializer>();
		if ( json["metadata"].type() == Json::stringValue ) {
			std::string f = json["metadata"].asString();
			std::string filename = grabURI( json["metadata"].asString(), json["root"].asString() );
			if ( !metadata.readFromFile(filename) ) {
				uf::iostream << "Error @ " << __FILE__ << ":" << __LINE__ << ": failed to open `" + filename + "`" << "\n";
				return false;
			}
		} else {
			metadata = json["metadata"];
		}
	}
	metadata["system"] = json;
	metadata["system"].removeMember("metadata");
	metadata["system"]["hooks"] = hooks;
/*
	for ( auto it = json["system"].begin(); it != json["system"].end(); ++it ) {
		if ( metadata["system"][it.key().asString()].isNull() )
			metadata["system"][it.key().asString()] = json["system"][it.key().asString()];
	}
*/

	// check for children
	{
		uf::Serializer target;
		if ( metadata["system"]["assets"].isArray() ) {
			target = metadata["system"]["assets"];
		} else if ( metadata["system"]["assets"].isArray() ) {
			target = metadata["system"]["assets"];
		} else if ( metadata["system"]["assets"].isObject() && !metadata["system"]["assets"]["entities"].isNull()  ) {
			target = metadata["system"]["assets"]["entities"];
		}
		for ( uint i = 0; i < target.size(); ++i ) {
			std::string canonical = "";
			std::string f = target[i].isObject() ? target[i]["filename"].asString() : target[i].asString();
			float delay = target[i].isObject() ? target[i]["delay"].asFloat() : -1;
			std::string filename = grabURI( f, json["root"].asString() );
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
					uf::iostream << "Error @ " << __FILE__ << ":" << __LINE__ << ": failed to open `" + filename + "`" << "\n";
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
	if ( metadata["system"]["lights"].isArray() ) {
		uf::Serializer target = metadata["system"]["lights"];
		auto& pTransform = this->getComponent<pod::Transform<>>();
		for ( uint i = 0; i < target.size(); ++i ) {
			uf::Serializer json = target[i];
			std::vector<pod::Vector4f> orientations;
			if ( json["ignore"].asBool() ) continue;
			if ( json["transform"]["orientation"].isNull() && json["shadows"]["fov"].asFloat() == 0.0f ) {
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
				if ( json["transform"]["orientation"].isArray() ) {
					orientation = pTransform.orientation;
				} else {
					for ( uint j = 0; j < 4; ++j ) orientation[j] = json["transform"]["orientation"][j].asFloat();
				}
				orientations.push_back(orientation);
			}
			uf::Object* target = this;
			for ( auto& orientation : orientations ) {
				std::string fname = "/light.json";
				if ( json["system"]["filename"].isString() ) fname = json["system"]["filename"].asString();
				auto* light = target->loadChildPointer(fname, false);
				if ( target == this ) target = light;
				auto& metadata = light->getComponent<uf::Serializer>();
				auto& transform = light->getComponent<pod::Transform<>>();
				if ( json["transform"]["position"].isArray() ) {
					for ( uint j = 0; j < 3; ++j )
						transform.position[j] = json["transform"]["position"][j].asFloat();
				} else {
					transform.position = pTransform.position;
				}
				for ( uint j = 0; j < 4; ++j )
					transform.orientation[j] = orientation[j];

				if ( !json["color"].isNull() ) metadata["light"]["color"] = json["color"];
				if ( !json["radius"].isNull() ) metadata["light"]["radius"] = json["radius"];
				if ( !json["power"].isNull() ) metadata["light"]["power"] = json["power"];
				if ( !json["shadows"].isNull() ) metadata["light"]["shadows"] = json["shadows"];
				if ( !json["flicker"].isNull() ) metadata["light"]["flicker"] = json["flicker"];
				if ( !json["fade"].isNull() ) metadata["light"]["fade"] = json["fade"];

				if ( !json["system"]["track"].isNull() ) metadata["system"]["track"] = json["system"]["track"];

				light->initialize();
			}
		}
	}
	return true;
}
uf::Object& uf::Object::loadChild( const std::string& f, bool initialize ) {
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	uf::Serializer json;
	std::string filename = grabURI( f, metadata["system"]["root"].asString() );
	if ( !json.readFromFile(filename) ) {
		uf::iostream << "Error @ " << __FILE__ << ":" << __LINE__ << ": failed to open `" + filename + "`" << "\n";
		return ::null;
	}
	json["root"] = uf::io::directory(filename);
	json["source"] = filename; //uf::io::filename(filename);
	json["hot reload"]["mtime"] = uf::io::mtime( filename ) + 10;
	return this->loadChild(json, initialize);
}
uf::Object& uf::Object::loadChild( const uf::Serializer& json, bool initialize ) {
	std::string type = json["type"].asString();
	if ( json["ignore"].asBool() ) return ::null;

	uf::Entity& entity = uf::instantiator::instantiate(type);
	uf::Object& object = entity.as<uf::Object>();
	this->addChild(entity);
	if ( !object.load(json) ) {
		uf::iostream << "Error @ " << __FILE__ << ":" << __LINE__ << " loading `" << json << "!" << "\n";
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