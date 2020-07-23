#include <uf/engine/object/object.h>
#include <uf/engine/asset/asset.h>
#include <uf/engine/scene/scene.h>
#include <uf/utils/time/time.h>
#include <uf/utils/math/transform.h>

namespace {
	uf::Timer<long long> timer(false);
}

UF_OBJECT_REGISTER_CPP(Object)
void uf::Object::initialize() {	
	uf::Entity::initialize();
}
void uf::Object::destroy() {
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	for( Json::Value::iterator it = metadata["system"]["hooks"]["alloc"].begin() ; it != metadata["system"]["hooks"]["alloc"].end() ; ++it ) {
	 	std::string name = it.key().asString();
		for ( size_t i = 0; i < metadata["system"]["hooks"]["alloc"][name].size(); ++i ) {
			size_t id = metadata["system"]["hooks"]["alloc"][name][(int) i].asUInt();
			uf::hooks.removeHook(name, id);
		}
	}
	
	uf::Entity::destroy();
}
void uf::Object::tick() {
	uf::Entity::tick();
	
	// Call queued hooks
	{
		if ( !timer.running() ) timer.start();
		float curTime = timer.elapsed().asDouble();
		uf::Serializer& metadata = this->getComponent<uf::Serializer>();
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
void uf::Object::render() {
	uf::Entity::render();
}

void uf::Object::queueHook( const std::string& name, const std::string& payload, double timeout ) {
	if ( !timer.running() ) timer.start();
	float start = timer.elapsed().asDouble();
	uf::Serializer queue;
	queue["name"] = name;
	queue["payload"] = uf::Serializer{payload};
	queue["timeout"] = start + timeout;
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	metadata["system"]["hooks"]["queue"].append(queue);
}
std::vector<std::string> uf::Object::callHook( const std::string& n, const std::string& payload ) {
	std::string name = uf::string::replace( n, "%UID%", std::to_string(this->getUid()) );
	return uf::hooks.call( name, payload );
}
std::size_t uf::Object::addHook( const std::string& name, const uf::HookHandler::Readable::function_t& callback ) {
	std::string parsed = uf::string::replace( name, "%UID%", std::to_string(this->getUid()) );
	std::size_t id = uf::hooks.addHook( parsed, callback );
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	metadata["system"]["hooks"]["alloc"][parsed].append(id);
	return id;
}

bool uf::Object::load( const std::string& filename ) {
	uf::Serializer json;
	std::string root = "./data/" + uf::string::directory(filename);
	if ( !json.readFromFile(root + uf::string::filename(filename)) ) {
		uf::iostream << "Error: failed to open `" + root + uf::string::filename(filename) + "`" << "\n";
		return false;
	}
	json["root"] = root;
	return this->load(json);
}
std::size_t uf::Object::loadChild( const std::string& filename, bool initialize ) {
	uf::Serializer json;
	std::string root = "./data/" + uf::string::directory(filename);
	if ( !json.readFromFile(root + uf::string::filename(filename)) ) {
		uf::iostream << "Error: failed to open `" + root + uf::string::filename(filename) + "`" << "\n";
		return -1;
	}
	json["root"] = root;
	return this->loadChild(json, initialize);
}
bool uf::Object::load( const uf::Serializer& json ) {
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	// Basic entity information
	{
		// Set name
		this->m_name = json["name"].asString();
		// Set transform
		bool load = json["transform"].isObject();
		if ( this->hasComponent<pod::Transform<>>() ) load = false;
		pod::Transform<>& transform = this->getComponent<pod::Transform<>>();
		if ( transform.position.x == 0 && transform.position.y == 0 && transform.position.z == 0 ) {
			load = true;
		}
		if ( load ) {
			float x = json["transform"]["position"][0].asFloat();
			float y = json["transform"]["position"][1].asFloat();
			float z = json["transform"]["position"][2].asFloat();
			
			transform.position = { x, y, z };
			transform.orientation = uf::quaternion::identity();
			if ( json["transform"]["rotation"]["angle"].asFloat() != 0 ) {
				transform.orientation = uf::quaternion::axisAngle( {
					json["transform"]["rotation"]["axis"][0].asFloat(),
					json["transform"]["rotation"]["axis"][1].asFloat(),
					json["transform"]["rotation"]["axis"][2].asFloat()
				}, 
					json["transform"]["rotation"]["angle"].asFloat()
				);
			}
			transform.scale = uf::vector::create( json["transform"]["scale"][0].asFloat(),json["transform"]["scale"][1].asFloat(),json["transform"]["scale"][2].asFloat() );
			transform = uf::transform::reorient( transform );
		}
	}

	uf::Scene& world = this->getRootParent<uf::Scene>();
	uf::Asset& assetLoader = world.getComponent<uf::Asset>();
	// initialize base entity, needed for asset hooks

	// Audio (singular)
	{
		// find first valid texture in asset list
		uf::Serializer target;
		if ( metadata["system"]["assets"].isArray() ) {
			target = metadata["system"]["assets"];
		} else if ( json["assets"].isArray() ) {
			target = json["assets"];
		} else if ( json["assets"].isObject() && !json["assets"]["audio"].isNull()  ) {
			target = json["assets"]["audio"];
		}
		for ( uint i = 0; i < target.size(); ++i ) {
			std::string filename = target[i].asString();
			if ( uf::string::extension(filename) != "ogg" ) continue;
			std::string canonical = "";
			if ( (canonical = assetLoader.load( filename )) != "" ) {
				uf::Serializer queue;
				queue["name"] = "asset:Load.%UID%";
				queue["payload"]["filename"] = canonical;
				metadata["system"]["hooks"]["queue"].append(queue);
			}
		}
	}

	// Texture (singular)
	{
		// find first valid texture in asset list
		uf::Serializer target;
		if ( metadata["system"]["assets"].isArray() ) {
			target = metadata["system"]["assets"];
		} else if ( json["assets"].isArray() ) {
			target = json["assets"];
		} else if ( json["assets"].isObject() && !json["assets"]["textures"].isNull()  ) {
			target = json["assets"]["textures"];
		}
		for ( uint i = 0; i < target.size(); ++i ) {
			std::string filename = target[i].asString();
			if ( uf::string::extension(filename) != "png" ) continue;
			std::string canonical = "";
			if ( (canonical = assetLoader.load( filename )) != "" ) {
				uf::Serializer queue;
				queue["name"] = "asset:Load.%UID%";
				queue["payload"]["filename"] = canonical;
				metadata["system"]["hooks"]["queue"].append(queue);
			//	uf::Serializer payload;
			//	payload["filename"] = canonical;
			//	this->callHook( "asset:Load.%UID%", payload );
			}
		}
	}

	uf::Serializer queue = metadata["system"]["hooks"]["queue"];

	// Metadata
	if ( json["metadata"] != Json::nullValue ) {
		uf::Serializer& metadata = this->getComponent<uf::Serializer>();
		if ( json["metadata"].type() == Json::stringValue ) {
			std::string filename = json["root"].asString() + "/" + json["metadata"].asString();
			if ( !metadata.readFromFile(filename) ) {
				uf::iostream << "Error: failed to open `" + filename + "`" << "\n";
				return false;
			}
		} else {
			metadata = json["metadata"];
		}
	}
	metadata["_config"] = json;
	metadata["_config"].removeMember("metadata");
	metadata["system"]["hooks"]["queue"] = queue;

	// check for children
	{
		uf::Serializer target;
		if ( metadata["system"]["assets"].isArray() ) {
			target = metadata["system"]["assets"];
		} else if ( metadata["_config"]["assets"].isArray() ) {
			target = metadata["_config"]["assets"];
		} else if ( metadata["_config"]["assets"].isObject() && !metadata["_config"]["assets"]["entities"].isNull()  ) {
			target = metadata["_config"]["assets"]["entities"];
		}
		for ( uint i = 0; i < target.size(); ++i ) {
			std::string filename = target[i].asString();
			if ( uf::string::extension(filename) != "json" ) continue;
			std::string root = "./data/entities/" + uf::string::directory(filename);
			filename = root + uf::string::filename(filename);
			if ( (filename = assetLoader.load(filename) ) == "" ) return false;
			uf::Serializer json;
			if ( !json.readFromFile(filename) ) {
				uf::iostream << "Error: failed to open `" + filename + "`" << "\n";
				return false;
			}
			json["root"] = root;
			if ( this->loadChild(json) == -1 ) return false;
		}
	}

	return true;
}
std::size_t uf::Object::loadChild( const uf::Serializer& json, bool initialize ) {
	uf::Entity* entity;
	std::string type = json["type"].asString();
	if ( json["ignore"].asBool() ) return 0;
	entity = uf::instantiator::instantiate(type);

	this->addChild(*entity);
	if ( !((uf::Object*) entity)->load(json) ) {
		uf::iostream << "Error loading `" << json << "!" << "\n";
		this->removeChild(*entity);
		delete entity;
		return -1;
	}
	if ( initialize ) entity->initialize();
	return entity->getUid();
}
/*
bool uf::Object::load( const uf::Serializer& json ) {
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	uf::Object& root = this->getRootParent<uf::Object>();
	uf::Asset& assetLoader = root.getComponent<uf::Asset>();

	// Load metadata
	{
		// name
		this->m_name = json["name"].asString();
		// transform
		pod::Transform<>& transform = this->getComponent<pod::Transform<>>();
	}

	return true;
}
std::size_t uf::Object::loadChild( const uf::Serializer& json, bool initialize ) {
	return 0;
}
*/
/*
bool uf::Object::load( const uf::Serializer& json ) {
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	// Basic entity information
	{
		// Set name
		this->m_name = json["name"].asString();
		// Set transform
		bool load = json["transform"].isObject();
		if ( this->hasComponent<pod::Transform<>>() ) load = false;
		pod::Transform<>& transform = this->getComponent<pod::Transform<>>();
		if ( transform.position.x == 0 && transform.position.y == 0 && transform.position.z == 0 ) {
			load = true;
		}
		if ( load ) {
			transform.position = uf::vector::create( json["transform"]["position"][0].asFloat(),json["transform"]["position"][1].asFloat(),json["transform"]["position"][2].asFloat() );
			transform.orientation = uf::quaternion::identity();
			if ( json["transform"]["rotation"]["angle"].asFloat() != 0 ) {
				transform.orientation = uf::quaternion::axisAngle( {
					json["transform"]["rotation"]["axis"][0].asFloat(),
					json["transform"]["rotation"]["axis"][1].asFloat(),
					json["transform"]["rotation"]["axis"][2].asFloat()
				}, 
					json["transform"]["rotation"]["angle"].asFloat()
				);
			}
			transform.scale = uf::vector::create( json["transform"]["scale"][0].asFloat(),json["transform"]["scale"][1].asFloat(),json["transform"]["scale"][2].asFloat() );
			transform = uf::transform::reorient( transform );
		}
	}

	uf::Scene& world = this->getRootParent<uf::Scene>();
	uf::Asset& assetLoader = world.getComponent<uf::Asset>();
	// initialize base entity, needed for asset hooks

	// Audio (singular)
	{
		// find first valid texture in asset list
		uf::Serializer target;
		if ( metadata["system"]["assets"].isArray() ) {
			target = metadata["system"]["assets"];
		} else if ( json["assets"].isArray() ) {
			target = json["assets"];
		} else if ( json["assets"].isObject() && !json["assets"]["audio"].isNull()  ) {
			target = json["assets"]["audio"];
		}
		for ( uint i = 0; i < target.size(); ++i ) {
			std::string filename = target[i].asString();
			if ( uf::string::extension(filename) != "ogg" ) continue;
			std::string canonical = "";
			if ( (canonical = assetLoader.load( filename )) != "" ) {
				uf::Serializer queue;
				queue["name"] = "asset:Load.%UID%";
				queue["payload"]["filename"] = canonical;
				metadata["system"]["hooks"]["queue"].append(queue);
			}
		}
	}

	// Texture (singular)
	{
		// find first valid texture in asset list
		uf::Serializer target;
		if ( metadata["system"]["assets"].isArray() ) {
			target = metadata["system"]["assets"];
		} else if ( json["assets"].isArray() ) {
			target = json["assets"];
		} else if ( json["assets"].isObject() && !json["assets"]["textures"].isNull()  ) {
			target = json["assets"]["textures"];
		}
		for ( uint i = 0; i < target.size(); ++i ) {
			std::string filename = target[i].asString();
			if ( uf::string::extension(filename) != "png" ) continue;
			std::string canonical = "";
			if ( (canonical = assetLoader.load( filename )) != "" ) {
				uf::Serializer queue;
				queue["name"] = "asset:Load.%UID%";
				queue["payload"]["filename"] = canonical;
				metadata["system"]["hooks"]["queue"].append(queue);
			}
		}
	}

	uf::Serializer queue = metadata["system"]["hooks"]["queue"];

	// Metadata
	if ( json["metadata"] != Json::nullValue ) {
		uf::Serializer& metadata = this->getComponent<uf::Serializer>();
		if ( json["metadata"].type() == Json::stringValue ) {
			std::string filename = json["root"].asString() + "/" + json["metadata"].asString();
			if ( !metadata.readFromFile(filename) ) {
				uf::iostream << "Error: failed to open `" + filename + "`" << "\n";
				return false;
			}
		} else {
			metadata = json["metadata"];
		}
	}
	metadata["_config"] = json;
	metadata["_config"].removeMember("metadata");
	metadata["system"]["hooks"]["queue"] = queue;

	// check for children
	{
		uf::Serializer target;
		if ( metadata["system"]["assets"].isArray() ) {
			target = metadata["system"]["assets"];
		} else if ( metadata["_config"]["assets"].isArray() ) {
			target = metadata["_config"]["assets"];
		} else if ( metadata["_config"]["assets"].isObject() && !metadata["_config"]["assets"]["entities"].isNull()  ) {
			target = metadata["_config"]["assets"]["entities"];
		}
		for ( uint i = 0; i < target.size(); ++i ) {
			std::string filename = target[i].asString();
			if ( uf::string::extension(filename) != "json" ) continue;
			std::string root = "./data/entities/" + uf::string::directory(filename);
			filename = root + uf::string::filename(filename);
			if ( (filename = assetLoader.load(filename) ) == "" ) return false;
			uf::Serializer json;
			if ( !json.readFromFile(filename) ) {
				uf::iostream << "Error: failed to open `" + filename + "`" << "\n";
				return false;
			}
			json["root"] = root;
			if ( this->loadChild(json) == -1 ) return false;
		}
	}

	return true;
}
std::size_t uf::Object::loadChild( const uf::Serializer& json, bool initialize ) {
	uf::Entity* entity;
	std::string type = json["type"].asString();
	if ( json["ignore"].asBool() ) return 0;
	if ( type == "Terrain" ) entity = new ext::Terrain;
	else if ( type == "Player" ) entity = new ext::Player;
	else if ( type == "Craeture" ) entity = new ext::Craeture;
	else if ( type == "Housamo" ) entity = new ext::HousamoSprite;
	else if ( type == "Gui" ) entity = new ext::Gui;
	else {
		uf::iostream << "Unimplemented entity: " << type << "\n";
		entity = new uf::Object;
	}
	// uf::iostream << entity << ": " << type << "\n";
	this->addChild(*entity);
	if ( !((uf::Object*) entity)->load(json) ) {
		uf::iostream << "Error loading `" << json << "!" << "\n";
		this->removeChild(*entity);
		delete entity;
		return -1;
	}
	if ( initialize ) entity->initialize();
	return entity->getUid();
}
*/