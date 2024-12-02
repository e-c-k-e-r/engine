#include <uf/engine/object/object.h>
#include <uf/engine/object/behavior.h>
#include <uf/engine/asset/asset.h>
#include <uf/engine/scene/scene.h>
#include <uf/utils/time/time.h>
#include <uf/utils/math/transform.h>
#include <uf/utils/math/physics.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/mesh/mesh.h>
#include <uf/ext/gltf/gltf.h>

/*
namespace {
	uf::Object null;
}
*/

uf::Timer<long long> uf::Object::timer(false);
bool uf::Object::assertionLoad = true;
bool uf::Object::deferLazyCalls = true;

#if UF_ENTITY_OBJECT_UNIFIED
	uf::Entity::Entity() UF_BEHAVIOR_ENTITY_CPP_ATTACH(uf::Object)
#else
	UF_OBJECT_REGISTER_BEGIN(uf::Object)
		UF_OBJECT_REGISTER_BEHAVIOR(uf::EntityBehavior)
		UF_OBJECT_REGISTER_BEHAVIOR(uf::ObjectBehavior)
	UF_OBJECT_REGISTER_END()
	uf::Object::Object() UF_BEHAVIOR_ENTITY_CPP_ATTACH(uf::Object)
#endif


void uf::Object::queueDeletion() {
	//UF_MSG_DEBUG("Marked for deletion: {}", uf::string::toString(*this));
	uf::scene::invalidateGraphs();
	uf::renderer::states::rebuild = true;

	auto children = this->getChildren();
	for ( uf::Entity* kv : children ) ((uf::Object*) kv)->queueDeletion();

	if ( this->hasParent() ) this->getParent().removeChild(*this);
	auto& metadata = this->getComponent<uf::ObjectBehavior::Metadata>();
	metadata.system.markedForDeletion = true;
}

uf::Hooks::return_t uf::Object::callHook( const uf::stl::string& name ) {
	return uf::hooks.call( this->formatHookName( name ) );
}
uf::Hooks::return_t uf::Object::callHook( const uf::stl::string& name, const pod::Hook::userdata_t& payload ) {
	return uf::hooks.call( this->formatHookName( name ), payload );
}
uf::Hooks::return_t uf::Object::lazyCallHook( const uf::stl::string& name ) {
	if ( uf::Object::deferLazyCalls ) {
		this->queueHook( name, 0.0f );
		return {};
	}
	return this->callHook( name );
}
uf::Hooks::return_t uf::Object::lazyCallHook( const uf::stl::string& name, const pod::Hook::userdata_t& payload ) {
	if ( uf::Object::deferLazyCalls ) {
		this->queueHook( name, payload, 0.0f );
		return {};
	}
	return this->callHook( name, payload );
}

void uf::Object::queueHook( const uf::stl::string& name, double timeout ) {
	return queueHook( name, (float) timeout );
}
void uf::Object::queueHook( const uf::stl::string& name, float timeout ) {
//	if ( !uf::Object::timer.running() ) uf::Object::timer.start();
//	double start = uf::Object::timer.elapsed().asDouble();

	auto& metadata = this->getComponent<uf::ObjectBehavior::Metadata>();
	auto& queue = metadata.hooks.queue.emplace_back(uf::ObjectBehavior::Metadata::Queued{
		.name = name,
		.timeout = uf::time::current + timeout,
		.type = 0,
	});
}
void uf::Object::queueHook( const uf::stl::string& name, const ext::json::Value& payload, double timeout ) {
	return queueHook( name, payload, (float) timeout );
}
void uf::Object::queueHook( const uf::stl::string& name, const ext::json::Value& payload, float timeout ) {
//	if ( !uf::Object::timer.running() ) uf::Object::timer.start();
//	double start = uf::Object::timer.elapsed().asDouble();

	auto& metadata = this->getComponent<uf::ObjectBehavior::Metadata>();
	auto& queue = metadata.hooks.queue.emplace_back(uf::ObjectBehavior::Metadata::Queued{
		.name = name,
		.timeout = uf::time::current + timeout,
		.type = -1,
	});
	queue.json = payload;
}
uf::stl::string uf::Object::formatHookName( const uf::stl::string& n, size_t uid, bool fetch ) {
	if ( fetch ) {
		uf::Object* object = (uf::Object*) uf::Entity::globalFindByUid( uid );
		if ( object ) return object->formatHookName( n );
	}
	uf::stl::unordered_map<uf::stl::string, uf::stl::string> formats = {
		{"%UID%", std::to_string(uid)},
	};
	uf::stl::string name = n;
	for ( auto& pair : formats ) {
		name = uf::string::replace( name, pair.first, pair.second );
	}
	return name;
}
uf::stl::string uf::Object::formatHookName( const uf::stl::string& n ) {
	size_t uid = this->getUid();
	size_t parent = uid;
	if ( this->hasParent() ) {
		parent = this->getParent().getUid();
	}
	uf::stl::unordered_map<uf::stl::string, uf::stl::string> formats = {
		{"%UID%", std::to_string(uid)},
		{"%P-UID%", std::to_string(parent)},
	};
	uf::stl::string name = n;
	for ( auto& pair : formats ) {
		name = uf::string::replace( name, pair.first, pair.second );
	}
	return name;
}

bool uf::Object::load( const uf::stl::string& f, bool inheritRoot ) {
	uf::Serializer json;
	uf::stl::string root = "";

	if ( inheritRoot && this->hasParent() ) {
		auto& parent = this->getParent<uf::Object>();
		auto& metadata = parent.getComponent<uf::ObjectBehavior::Metadata>();
		auto& metadataJson = parent.getComponent<uf::Serializer>();
		root = metadata.system.root;
	}

	uf::stl::string filename = uf::io::resolveURI( f, root );
	if ( !json.readFromFile( filename ) ) return false;

	json["source"] = filename;
	json["root"] = uf::io::directory(filename);
	json["hot reload"]["mtime"] = uf::io::mtime(filename) + 10;

	return this->load(json);
}

bool uf::Object::reload( bool hard ) {
	auto& metadata = this->getComponent<uf::ObjectBehavior::Metadata>();
	auto& metadataJson = this->getComponent<uf::Serializer>();

	if ( metadata.system.filename == "" ) return false;
	
	uf::Serializer json;	
	if ( !json.readFromFile( metadata.system.filename ) ) return false;
	if ( hard ) return this->load(metadata.system.filename);

	ext::json::Value payload;
	payload["old"] = metadataJson;
	ext::json::forEach( json["metadata"], [&]( const uf::stl::string& key, const ext::json::Value& value ){
		metadataJson[key] = value;
	});
	// update transform if requested
	if ( ext::json::isObject(json["transform"]) ) {
		auto& transform = this->getComponent<pod::Transform<>>();
		auto* reference = transform.reference;
		transform = uf::transform::decode( json["transform"], transform );
		transform.reference = reference;
	}
	payload["new"] = metadataJson;
	this->queueHook("object:Reload.%UID%", metadataJson);
	return true;
}

void uf::Object::loadAssets( const uf::Serializer& _json ){
	auto& metadata = this->getComponent<uf::ObjectBehavior::Metadata>();
	auto& metadataJson = this->getComponent<uf::Serializer>();
	auto& scene = uf::scene::getCurrentScene();
//	auto& assetLoader = scene.getComponent<uf::asset>();

	ext::json::Value json = _json;

	#define ASSET_ENTRY(type) { uf::string::lowercase(#type), uf::asset::Type::type }
	const uf::stl::unordered_map<uf::stl::string, uf::asset::Type> assets = {
		ASSET_ENTRY(AUDIO),
		ASSET_ENTRY(IMAGE),
		ASSET_ENTRY(GRAPH),
		ASSET_ENTRY(LUA),
	};

	for ( auto& pair : assets  ) {
		auto& assetType = pair.second;
		auto& assetTypeString = pair.first;

		uf::Serializer target;
		bool override = false;
		if ( ext::json::isObject( json ) && ext::json::isArray( json["assets"] ) ) {
			target = json["assets"];
		} else if ( ext::json::isObject( json ) &&  ext::json::isObject( json["assets"] ) && !ext::json::isNull( json["assets"][assetTypeString] )  ) {
			target = json["assets"][assetTypeString];
		} else if ( ext::json::isArray( json ) ) {
			target = json;
		} else if ( ext::json::isObject( json ) && !ext::json::isNull( json[assetTypeString] )  ) {
			target = json[assetTypeString];
		}/* else if ( ext::json::isObject( metadataJson["system"]["assets"] ) ) {
			target = metadataJson["system"]["assets"];
		}*/

		for ( size_t i = 0; i < target.size(); ++i ) {
			bool isObject = ext::json::isObject( target[i] );
			uf::stl::string filename = isObject ? target[i]["filename"].as<uf::stl::string>() : target[i].as<uf::stl::string>();
			uf::stl::string uri = uf::io::resolveURI( filename, metadata.system.root );
			uf::stl::string mime = isObject ? target[i]["mime"].as<uf::stl::string>("") : "";
			bool bind = isObject ? target[i]["bind"].as<bool>(true) : true;

			uf::asset::Payload payload = this->resolveToPayload( uri, mime );
			if ( !uf::asset::isExpected( payload, assetType ) ) continue;

			payload.hash = isObject ? target[i]["hash"].as<uf::stl::string>("") : "";
			payload.object = this->resolvable<>();
			payload.monoThreaded = isObject ? !target[i]["multithreaded"].as<bool>(true) : !true;
			payload.asComponent = isObject ? target[i]["component"].as<bool>(true) : true;
			
			switch ( assetType ) {
				case uf::asset::Type::LUA: {
				//	payload.asComponent = true;
					if ( bind ) uf::instantiator::bind("LuaBehavior", *this);
				} break;
				case uf::asset::Type::GRAPH: {
				//	payload.asComponent = true;
					if ( bind ) uf::instantiator::bind("GraphBehavior", *this);

					auto metadata = json["metadata"]["graph"];
					payload.metadata = metadata;
					payload.metadata["root"] = json["root"];

					// nasty hack to get soundscapes/music to load while the rest of the graph loads
					if ( ext::json::isObject(metadata["assets"]) || ext::json::isArray(metadata["assets"]) ) {
						scene.loadAssets(metadata["assets"]);
					}

					// nasty hack to get a graph to queue BGM
					auto& sceneMetadata = scene.getComponent<uf::Serializer>();
					if ( ext::json::isObject(metadata["bgm"]) ) {
						sceneMetadata["bgm"] = metadata["bgm"];
					} else if ( metadata["bgm"].is<uf::stl::string>() ) {
						sceneMetadata["bgm"]["load"] = metadata["bgm"];
					}


					/*
					if ( ext::json::isObject(metadata["bgm"]) ) {
						ext::json::Value payload;
						payload["filename"] = metadata["bgm"]["load"];
						UF_MSG_DEBUG("{}", payload.dump(1, '\t'));
						scene.queueHook("bgm:Load.%UID%", payload, 0.5f);
					} else if ( metadata["bgm"].is<uf::stl::string>() ) {
						ext::json::Value payload;
						UF_MSG_DEBUG("{}", payload.dump(1, '\t'));
						scene.queueHook("bgm:Load.%UID%", payload, 0.5f);
					}
					*/
				} break;
			}

			this->queueHook( "asset:QueueLoad.%UID%", payload, isObject ? target[i]["delay"].as<float>() : 0 );
		}
	}
}

bool uf::Object::load( const uf::Serializer& _json ) {
	auto& metadata = this->getComponent<uf::ObjectBehavior::Metadata>();
	auto& metadataJson = this->getComponent<uf::Serializer>();
	auto& scene = uf::scene::getCurrentScene();
//	auto& assetLoader = scene.getComponent<uf::asset>();
	uf::Serializer json = _json;

	// setup root/source/mtime
	if ( json["source"].is<uf::stl::string>() ) {
		metadata.system.filename = json["source"].as<uf::stl::string>();
		metadata.system.root = uf::io::directory( metadata.system.filename );
		metadata.system.hotReload.mtime = uf::io::mtime( metadata.system.filename ) + 10;
	} else if ( json["root"].is<uf::stl::string>() ) {
		metadata.system.root = json["root"].as<uf::stl::string>();
	}
	// import
	if ( json["import"].is<uf::stl::string>() || json["include"].is<uf::stl::string>() ) {
		uf::Serializer chain = json;
		uf::stl::string root = metadata.system.root;
		uf::Serializer separated;
		separated["assets"] = ext::json::isArray( json["assets"] ) ? json["assets"] : ext::json::array();
		separated["behaviors"] = ext::json::isArray( json["behaviors"] ) ? json["behaviors"] : ext::json::array();
		do {
			uf::stl::string filename = chain["import"].is<uf::stl::string>() ? chain["import"].as<uf::stl::string>() : chain["include"].as<uf::stl::string>();
			filename = uf::io::resolveURI( filename, root );
			chain.readFromFile( filename );
			root = uf::io::directory( filename );
			ext::json::forEach(chain["assets"], [&](ext::json::Value& value){
				if ( ext::json::isObject( value ) ) {
					value["filename"] = uf::io::resolveURI( value["filename"].as<uf::stl::string>(), root );
				} else {
					value = uf::io::resolveURI( value.as<uf::stl::string>(), root );
				}
				separated["assets"].emplace_back( value );
			});
			ext::json::forEach(chain["behaviors"], [&](ext::json::Value& value){
				separated["behaviors"].emplace_back(value);
			});
			chain["assets"] = ext::json::null();
			chain["behaviors"] = ext::json::null();
			// merge table
			json.import( chain );
		} while ( chain["import"].is<uf::stl::string>() || chain["include"].is<uf::stl::string>() );
		json["import"] = ext::json::null();
		json["assets"] = separated["assets"];
		json["behaviors"] = separated["behaviors"];
	}
	// copy system table to base
	ext::json::forEach( json["system"], [&](const uf::stl::string& key, const ext::json::Value& value){
		if ( ext::json::isNull( json[key] ) )
			json[key] = value;
	});
#if UF_ENV_DREAMCST
	metadata.system.hotReload.enabled = false;
#else
	metadata.system.hotReload.enabled = json["system"]["hot reload"]["enabled"].as<bool>();
#endif
	// Basic entity information
	// Set name
	this->m_name = json["name"].is<uf::stl::string>() ? json["name"].as<uf::stl::string>() : json["type"].as<uf::stl::string>();
	// Set transform
	{
		bool load = true;
		if ( this->hasComponent<pod::Transform<>>() ) {
			pod::Transform<>& transform = this->getComponent<pod::Transform<>>();
			load = transform.position.x == 0 && transform.position.y == 0 && transform.position.z == 0;
		}
		if ( load && ext::json::isObject( json["transform"] ) ) {
			pod::Transform<>& transform = this->getComponent<pod::Transform<>>();
			transform = uf::transform::decode(json["transform"], transform);
			if ( json["transform"]["reference"].as<uf::stl::string>() == "parent" ) {
				auto& parent = this->getParent().as<uf::Object>();
				transform.reference = &parent.getComponent<pod::Transform<>>();
			}
		}
	}
	// Set movement
	if ( ext::json::isObject( json["physics"] ) && !this->hasComponent<pod::Physics>() ) {
		auto& physics = this->getComponent<pod::Physics>();
		physics.linear.velocity = uf::vector::decode( json["physics"]["linear"]["velocity"], physics.linear.velocity );
		physics.linear.acceleration = uf::vector::decode( json["physics"]["linear"]["acceleration"], physics.linear.acceleration );
		physics.rotational.velocity = uf::vector::decode( json["physics"]["rotational"]["velocity"], physics.rotational.velocity );
		physics.rotational.acceleration = uf::vector::decode( json["physics"]["rotational"]["acceleration"], physics.rotational.acceleration );
	}

	// Load assets
	this->loadAssets( json );

	// Bind behaviors
	{
		if ( json["type"].is<uf::stl::string>() ) uf::instantiator::bind( json["type"].as<uf::stl::string>(), *this );

		uf::Serializer target;
		if ( ext::json::isArray( metadataJson["system"]["behaviors"] ) ) {
			target = metadataJson["system"]["behaviors"];
		} else if ( ext::json::isArray( json["behaviors"] ) ) {
			target = json["behaviors"];
		}
		for ( size_t i = 0; i < target.size(); ++i ) {
			uf::instantiator::bind( target[i].as<uf::stl::string>(), *this );
		}
	}

	// Metadata
	if ( !ext::json::isNull( json["metadata"] ) ) {
		if ( json["metadata"].is<uf::stl::string>() ) {
			uf::stl::string f = json["metadata"].as<uf::stl::string>();
			uf::stl::string filename = uf::io::resolveURI( json["metadata"].as<uf::stl::string>(), metadata.system.root );
			if ( !metadataJson.readFromFile(filename) ) return false;
		} else {
			metadataJson = json["metadata"];
		}
	}
	metadataJson["system"] = json;
	metadataJson["system"].erase("metadata");

	// check for children
	{
		uf::Serializer target;
		bool override = false;
		if ( ext::json::isObject( metadataJson["system"]["assets"] ) ) {
			target = metadataJson["system"]["assets"];
		} else if ( ext::json::isArray( json["assets"] ) ) {
			target = json["assets"];
		} else if ( ext::json::isObject( json["assets"] ) && !ext::json::isNull( json["assets"]["json"] )  ) {
			target = json["assets"]["json"];
		}
		for ( size_t i = 0; i < target.size(); ++i ) {
			uf::stl::string filename = ext::json::isObject( target[i] ) ? target[i]["filename"].as<uf::stl::string>() : target[i].as<uf::stl::string>();
			uf::stl::string uri = uf::io::resolveURI( filename, metadata.system.root );
			uf::stl::string mime = ext::json::isObject( target[i] ) ? target[i]["mime"].as<uf::stl::string>() : "";
			uf::stl::string hash = ext::json::isObject( target[i] ) ? target[i]["hash"].as<uf::stl::string>() : "";
			
			uf::asset::Payload payload = this->resolveToPayload( uri, mime );
			if ( !uf::asset::isExpected( payload, uf::asset::Type::JSON ) ) continue;
			if ( (uri = uf::asset::load( payload )) == "" ) continue;
			
			float delay = ext::json::isObject( target[i] ) ? target[i]["delay"].as<float>() : -1;
			if ( delay > -1 ) {
				payload.uri = uri;
				payload.hash = hash;
				payload.mime = mime;
				this->queueHook( "asset:Load.%UID%", payload, delay );
				continue;
			}
			{
				uf::Serializer json;
				if ( !json.readFromFile(uri, hash) ) continue;

				json["source"] = uri;
				json["root"] = uf::io::directory(uri);
				json["hot reload"]["mtime"] = uf::io::mtime(uri) + 10;

				if ( this->loadChildUid(json) == -1 ) continue;
			}
		}
	}
	// Add lights
#if UF_USE_UNUSED
	if ( ext::json::isArray( metadataJson["system"]["lights"] ) ) {
		uf::Serializer target = metadataJson["system"]["lights"];
		auto& pTransform = this->getComponent<pod::Transform<>>();
		for ( size_t i = 0; i < target.size(); ++i ) {
			uf::Serializer json = target[i];			

			auto* light = this->loadChildPointer("/light.json", false);
			if ( !light ) continue;
			auto& lightTransform = light->getComponent<pod::Transform<>>();
			auto& lightMetadata = light->getComponent<uf::Serializer>();
			lightTransform = uf::transform::decode( json["transform"], lightTransform );
			ext::json::forEach( json, [&]( const uf::stl::string& key, ext::json::Value& value ){
				if ( key == "transform" ) return;
				lightMetadata["light"][key] = value;
			});
			light->initialize();
		}
	}
#endif
	return true;
}
uf::Object& uf::Object::loadChild( const uf::stl::string& f, bool initialize ) {
	auto& metadata = this->getComponent<uf::ObjectBehavior::Metadata>();
	auto& metadataJson = this->getComponent<uf::Serializer>();
	
	uf::Serializer json;
	uf::stl::string filename = uf::io::resolveURI( f, metadata.system.root );
	if ( !json.readFromFile(filename) ) {
		if ( !uf::Object::assertionLoad ) {
			UF_MSG_ERROR("assertionLoad is unset, loading empty entity");
			auto& entity = uf::instantiator::instantiate("Object");
			entity.getComponent<uf::ObjectBehavior::Metadata>().system.invalid = true;
			this->addChild(entity);
		} else {
			UF_EXCEPTION("Failed to load file: {}", filename);
		}
	}

	json["source"] = filename;
	json["root"] = uf::io::directory(filename);
	json["hot reload"]["mtime"] = uf::io::mtime(filename) + 10;

	return this->loadChild(json, initialize);
}
uf::Object& uf::Object::loadChild( const uf::Serializer& _json, bool initialize ) {
	uf::Serializer json = _json;
	uf::stl::string type = json["type"].as<uf::stl::string>();
	if ( type == "" ) type = "Object";

	uf::Entity& entity = uf::instantiator::instantiate(type);
	uf::Object& object = entity.as<uf::Object>();
	this->addChild(entity);
	if ( json["ignore"].as<bool>() ) return object;
	if ( !object.load(json) ) {
		if ( !uf::Object::assertionLoad ) {
			UF_MSG_ERROR("assertionLoad is unset, loading empty entity");
			entity.getComponent<uf::ObjectBehavior::Metadata>().system.invalid = true;
		} else {
			UF_EXCEPTION("Failed to load JSON: {}", json.serialize());
		}
	}
	if ( initialize ) entity.initialize();
	return object;
}
uf::Object* uf::Object::loadChildPointer( const uf::stl::string& f, bool initialize ) {
	uf::Object* pointer = &this->loadChild(f, initialize);
	return pointer;
//	return pointer != &::null ? pointer : NULL;
}
uf::Object* uf::Object::loadChildPointer( const uf::Serializer& json, bool initialize ) {
	uf::Object* pointer = &this->loadChild(json, initialize);
	return pointer;
//	return pointer != &::null ? pointer : NULL;
}
size_t uf::Object::loadChildUid( const uf::stl::string& f, bool initialize ) {
	uf::Object* pointer = this->loadChildPointer(f, initialize);
	return pointer ? pointer->getUid() : 0;
}
size_t uf::Object::loadChildUid( const uf::Serializer& json, bool initialize ) {
	uf::Object* pointer = this->loadChildPointer(json, initialize);
	return pointer ? pointer->getUid() : 0;
}

uf::stl::string uf::Object::resolveURI( const uf::stl::string& filename, const uf::stl::string& root  ) {
	return uf::io::resolveURI( filename, root == "" ? this->getComponent<uf::ObjectBehavior::Metadata>().system.root : root );
}

uf::asset::Payload uf::Object::resolveToPayload( const uf::stl::string& filename, const uf::stl::string& mime ) {
	auto payload = uf::asset::resolveToPayload( this->resolveURI( filename ), mime );
	payload.uri = filename;
	payload.object = this->resolvable<>();
	return payload;
}


/*
#include <uf/utils/string/ext.h>
uf::stl::string uf::string::toString( const uf::Object& object ) {
	return ::fmt::format("{} ({}): {}", object.getName(), object.getUid(), (void*) &object);
}
*/