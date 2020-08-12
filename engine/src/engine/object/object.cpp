#include <uf/engine/object/object.h>
#include <uf/engine/asset/asset.h>
#include <uf/engine/scene/scene.h>
#include <uf/utils/time/time.h>
#include <uf/utils/math/transform.h>

namespace {
	uf::Timer<long long> timer(false);

	std::string grabURI( std::string filename, std::string root = "" ) {
		if ( filename.substr(0,8) == "https://" ) return filename;
		std::string extension = uf::string::extension(filename);
		if ( filename[0] == '/' || root == "" ) {
			if ( filename.substr(0,9) == "/smtsamo/" ) root = "./data/";
			else if ( extension == "json" ) root = "./data/entities/";
			else if ( extension == "png" ) root = "./data/textures/";
			else if ( extension == "ogg" ) root = "./data/audio/";
		}
		return uf::string::sanitize(filename, root);
	}
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
	// listen for metadata file changes
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	if ( metadata["system"]["hot reload"]["enabled"].asBool() ) {
		std::string filename = "./entities/"+metadata["system"]["source"].asString();
		std::string root = "./data/" + uf::string::directory(filename);
		size_t mtime = uf::string::mtime( root + uf::string::filename(filename) );
		if ( metadata["system"]["hot reload"]["mtime"].asUInt64() < mtime ) {
			metadata["system"]["hot reload"]["mtime"] = mtime;
			this->reload();
			//this->queueHook("metadata:Reload.%UID%");
		}
	}

	// Call queued hooks
	{
		if ( !timer.running() ) timer.start();
		float curTime = timer.elapsed().asDouble();
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
		uf::iostream << "Error: failed to open `" + filename + "`" << "\n";
		return false;
	}
	json["root"] = uf::string::directory(filename);
	json["source"] = uf::string::filename(filename);
	json["hot reload"]["mtime"] = uf::string::mtime(filename);
	return this->load(json);
}

bool uf::Object::reload( bool hard ) {
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	if ( !metadata["system"]["source"].isString() ) return false;
	uf::Serializer json;
	std::string filename = grabURI( metadata["system"]["source"].asString(), metadata["system"]["root"].asString() ); // uf::string::sanitize(metadata["system"]["source"].asString(), metadata["system"]["root"].asString());
	if ( !json.readFromFile( filename ) ) {
		uf::iostream << "Error: failed to open `" + filename + "`" << "\n";
		return false;
	}
	if ( hard ) return this->load(filename);
	for ( auto it = json["metadata"].begin(); it != json["metadata"].end(); ++it ) {
		metadata[it.key().asString()] = json["metadata"][it.key().asString()];
	}
	this->queueHook("object:Reload.%UID%");
	return true;
}

std::size_t uf::Object::loadChild( const std::string& f, bool initialize ) {
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	uf::Serializer json;
	std::string filename = grabURI( f, metadata["system"]["root"].asString() );
	if ( !json.readFromFile(filename) ) {
		uf::iostream << "Error: failed to open `" + filename + "`" << "\n";
		return -1;
	}
	json["root"] = uf::string::directory(filename);
	json["source"] = uf::string::filename(filename);
	json["hot reload"]["mtime"] = uf::string::mtime( filename );
	return this->loadChild(json, initialize);
}
bool uf::Object::load( const uf::Serializer& json ) {
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	// Basic entity information
	{
		// Set name
		this->m_name = json["name"].isString() ? json["name"].asString() : json["type"].asString();
		// Set transform
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

	uf::Scene& scene = this->getRootParent<uf::Scene>();
	uf::Asset& assetLoader = scene.getComponent<uf::Asset>();
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
			std::string canonical = "";
			std::string f = target[i].asString();
			std::string filename = grabURI( target[i].asString(), json["root"].asString() );
			if ( uf::string::extension(filename) != "ogg" ) continue;
			if ( (canonical = assetLoader.load( filename )) != "" ) {
			/*
				uf::Serializer queue;
				queue["name"] = "asset:Load.%UID%";
				queue["payload"]["filename"] = canonical;
				metadata["system"]["hooks"]["queue"].append(queue);
			*/
				uf::Serializer payload;
				payload["filename"] = canonical;
				this->queueHook( "asset:Load.%UID%", payload );
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
			std::string canonical = "";
			std::string f = target[i].asString();
			std::string filename = grabURI( target[i].asString(), json["root"].asString() );
			if ( uf::string::extension(filename) != "png" ) continue;
			if ( (canonical = assetLoader.load( filename )) != "" ) {
			/*
				uf::Serializer queue;
				queue["name"] = "asset:Load.%UID%";
				queue["payload"]["filename"] = canonical;
				metadata["system"]["hooks"]["queue"].append(queue);
			*/
				uf::Serializer payload;
				payload["filename"] = canonical;
				this->queueHook( "asset:Load.%UID%", payload );
			}
		}
	}

	uf::Serializer queue = metadata["system"]["hooks"]["queue"];
	// Metadata
	if ( json["metadata"] != Json::nullValue ) {
		uf::Serializer& metadata = this->getComponent<uf::Serializer>();
		if ( json["metadata"].type() == Json::stringValue ) {
			std::string f = json["metadata"].asString();
			std::string filename = grabURI( json["metadata"].asString(), json["root"].asString() );
			if ( !metadata.readFromFile(filename) ) {
				uf::iostream << "Error: failed to open `" + filename + "`" << "\n";
				return false;
			}
		} else {
			metadata = json["metadata"];
		}
	}
	metadata["system"] = json;
	metadata["system"].removeMember("metadata");
	metadata["system"]["hooks"]["queue"] = queue;

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
			uf::Serializer json;
			std::string canonical = "";
			std::string f = target[i].asString();
			std::string filename = grabURI( target[i].asString(), metadata["system"]["root"].asString() );
			if ( uf::string::extension(filename) != "json" ) continue;
			if ( (filename = assetLoader.load(filename) ) == "" ) continue;
			if ( !json.readFromFile(filename) ) {
				uf::iostream << "Error: failed to open `" + filename + "`" << "\n";
				continue;
			}
			json["root"] = uf::string::directory(filename);
			json["source"] = uf::string::filename(filename);
			json["hot reload"]["mtime"] = uf::string::mtime( filename );
			if ( this->loadChild(json) == -1 ) continue;
		}
	}

	return true;
}
std::size_t uf::Object::loadChild( const uf::Serializer& json, bool initialize ) {
	uf::Entity* entity;
	std::string type = json["type"].asString();
	if ( json["ignore"].asBool() ) return 0;
	entity = uf::instantiator::instantiate(type);

	if ( !((uf::Object*) entity)->load(json) ) {
		uf::iostream << "Error loading `" << json << "!" << "\n";
		delete entity;
		return -1;
	}
	this->addChild(*entity);
	if ( initialize ) entity->initialize();
	return entity->getUid();
}