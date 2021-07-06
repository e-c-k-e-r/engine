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

uf::Timer<long long> uf::Object::timer(false);
namespace {
	uf::Object null;
}

UF_OBJECT_REGISTER_BEGIN(uf::Object)
	UF_OBJECT_REGISTER_BEHAVIOR(uf::EntityBehavior)
	UF_OBJECT_REGISTER_BEHAVIOR(uf::ObjectBehavior)
UF_OBJECT_REGISTER_END()
uf::Object::Object() UF_BEHAVIOR_ENTITY_CPP_ATTACH(uf::Object)
void uf::Object::queueHook( const uf::stl::string& name, const ext::json::Value& payload, double timeout ) {
	if ( !uf::Object::timer.running() ) uf::Object::timer.start();
	double start = uf::Object::timer.elapsed().asDouble();
#if UF_ENTITY_METADATA_USE_JSON
	uf::Serializer queue;
	queue["name"] = name;
	queue["payload"] = payload;
	queue["timeout"] = start + timeout;
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	metadata["system"]["hooks"]["queue"].emplace_back(queue);
#else
	auto& metadata = this->getComponent<uf::ObjectBehavior::Metadata>();
	auto& queue = metadata.hooks.queue.emplace_back(uf::ObjectBehavior::Metadata::Queued{
		.name = name,
		.payload = payload,
		.timeout = start + timeout,
	});
#endif
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

uf::Hooks::return_t uf::Object::callHook( const uf::stl::string& name ) {
	return uf::hooks.call( this->formatHookName( name ), ext::json::null() );
}
uf::Hooks::return_t uf::Object::callHook( const uf::stl::string& name, const ext::json::Value& json ) {
	return uf::hooks.call( this->formatHookName( name ), json );
}
uf::Hooks::return_t uf::Object::callHook( const uf::stl::string& name, const uf::Serializer& serializer ) {
	return uf::hooks.call( this->formatHookName( name ), (const ext::json::Value&) serializer );
}
bool uf::Object::load( const uf::stl::string& f, bool inheritRoot ) {
	uf::Serializer json;
	uf::stl::string root = "";
	if ( inheritRoot && this->hasParent() ) {
		auto& parent = this->getParent<uf::Object>();
		uf::Serializer& metadata = parent.getComponent<uf::Serializer>();
		root = metadata["system"]["root"].as<uf::stl::string>();
	}
	uf::stl::string filename = uf::io::resolveURI( f, root );
	if ( !json.readFromFile( filename ) ) return false;

	json["root"] = uf::io::directory(filename);
	json["source"] = filename;
#if UF_ENTITY_METADATA_USE_JSON
	json["hot reload"]["mtime"] = uf::io::mtime(filename) + 10;
#else
	auto& metadata = this->getComponent<uf::ObjectBehavior::Metadata>();
	metadata.hotReload.source = filename;
	metadata.hotReload.mtime = uf::io::mtime(filename) + 10;
#endif
	return this->load(json);
}

bool uf::Object::reload( bool hard ) {
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	if ( !metadata["system"]["source"].is<uf::stl::string>() ) return false;
	uf::Serializer json;
	uf::Serializer payload;
	uf::stl::string filename = metadata["system"]["source"].as<uf::stl::string>();
	if ( !json.readFromFile( filename ) ) return false;
	if ( hard ) return this->load(filename);

	payload["old"] = metadata;
	ext::json::forEach( json["metadata"], [&]( const uf::stl::string& key, const ext::json::Value& value ){
		metadata[key] = value;
	});
	// update transform if requested
	if ( ext::json::isObject(json["transform"]) ) {
		auto& transform = this->getComponent<pod::Transform<>>();
		auto* reference = transform.reference;
		transform = uf::transform::decode( json["transform"], transform );
		transform.reference = reference;
	}
	payload["new"] = metadata;
	this->queueHook("object:Reload.%UID%", payload);
	return true;
}

bool uf::Object::load( const uf::Serializer& _json ) {
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	uf::Serializer json = _json;
	// import
	if ( json["import"].is<uf::stl::string>() || json["include"].is<uf::stl::string>() ) {
		uf::Serializer chain = json;
		uf::stl::string root = json["root"].as<uf::stl::string>();
		uf::Serializer separated;
		separated["assets"] = json["assets"];
		separated["behaviors"] = json["behaviors"];
		do {
			uf::stl::string filename = chain["import"].is<uf::stl::string>() ? chain["import"].as<uf::stl::string>() : chain["include"].as<uf::stl::string>();
			filename = uf::io::resolveURI( filename, root );
			chain.readFromFile( filename );
			root = uf::io::directory( filename );
			ext::json::forEach(chain["assets"], [&](ext::json::Value& value){
				if ( ext::json::isObject( value ) ) value["filename"] = uf::io::resolveURI( value["filename"].as<uf::stl::string>(), root );
				else value = uf::io::resolveURI( value.as<uf::stl::string>(), root );
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
#if UF_ENTITY_METADATA_USE_JSON
	json["hot reload"]["enabled"] = json["system"]["hot reload"]["enabled"];
#else
	{
		auto& metadata = this->getComponent<uf::ObjectBehavior::Metadata>();
		metadata.hotReload.enabled = json["system"]["hot reload"]["enabled"].as<bool>();
	}
#endif
	// Basic entity information
	// Set name
	this->m_name = json["name"].is<uf::stl::string>() ? json["name"].as<uf::stl::string>() : json["type"].as<uf::stl::string>();
	uf::Scene& scene = uf::scene::getCurrentScene();
	uf::Asset& assetLoader = scene.getComponent<uf::Asset>();
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
	{
		if ( ext::json::isObject( json["physics"] ) && !this->hasComponent<pod::Physics>() ) {
			auto& physics = this->getComponent<pod::Physics>();
			if ( ext::json::isArray( json["physics"]["linear"]["velocity"] ) )
				for ( size_t j = 0; j < 3; ++j )
					physics.linear.velocity[j] = json["physics"]["linear"]["velocity"][j].as<float>();
			if ( ext::json::isArray( json["physics"]["linear"]["acceleration"] ) )
				for ( size_t j = 0; j < 3; ++j )
					physics.linear.acceleration[j] = json["physics"]["linear"]["acceleration"][j].as<float>();
			
			if ( ext::json::isArray( json["physics"]["rotational"]["velocity"] ) )
				for ( size_t j = 0; j < 4; ++j )
					physics.rotational.velocity[j] = json["physics"]["rotational"]["velocity"][j].as<float>();
			if ( ext::json::isArray( json["physics"]["rotational"]["acceleration"] ) )
				for ( size_t j = 0; j < 4; ++j )
					physics.rotational.acceleration[j] = json["physics"]["rotational"]["acceleration"][j].as<float>();
		}
	}

	#define UF_OBJECT_LOAD_ASSET_HEADER(type)\
		uf::stl::string assetCategory = #type;\
		uf::Serializer target;\
		bool override = false;\
		if ( ext::json::isObject( metadata["system"]["assets"] ) ) {\
			target = metadata["system"]["assets"];\
		} else if ( ext::json::isArray( json["assets"] ) ) {\
			target = json["assets"];\
		} else if ( ext::json::isObject( json["assets"] ) && !ext::json::isNull( json["assets"][#type] )  ) {\
			target = json["assets"][#type];\
		}

	uf::stl::vector<ext::json::Value> rejects;
	#define UF_OBJECT_LOAD_ASSET(...)\
		bool isObject = ext::json::isObject( target[i] );\
		uf::stl::string canonical = "";\
		uf::stl::string hash = isObject ? target[i]["hash"].as<uf::stl::string>() : "";\
		uf::stl::string f = isObject ? target[i]["filename"].as<uf::stl::string>() : target[i].as<uf::stl::string>();\
		float delay = isObject ? target[i]["delay"].as<float>() : 0;\
		uf::stl::string category = isObject ? target[i]["category"].as<uf::stl::string>() : "";\
		bool bind = isObject && target[i]["bind"].is<bool>() ? target[i]["bind"].as<bool>() : true;\
		uf::stl::string filename = uf::io::resolveURI( f, json["root"].as<uf::stl::string>() );\
		bool singleThreaded = isObject ? target[i]["single threaded"].as<bool>() : false;\
		bool overriden = isObject ? target[i]["override"].as<bool>() : false;\
		uf::stl::vector<uf::stl::string> allowedExtensions = {__VA_ARGS__};\
		uf::stl::string extension = uf::io::extension(filename);\
		if ( override && overriden ) {\
			if ( category == "" ) continue;\
		} else {\
			if ( category != "" && category != assetCategory ) continue;\
			if ( category == "" && std::find( allowedExtensions.begin(), allowedExtensions.end(), extension ) == allowedExtensions.end() ) continue;\
		}\
		uf::Serializer payload;\
		if ( isObject ) payload = target[i];\
		payload["filename"] = filename;\
		if ( hash != "" ) payload["hash"] = hash;\
		payload["category"] = assetCategory != "" ? assetCategory : category;\
		payload["single threaded"] = singleThreaded;\
		this->queueHook( "asset:QueueLoad.%UID%", payload, delay );\

	// Audio
	{
		UF_OBJECT_LOAD_ASSET_HEADER(audio)
		for ( size_t i = 0; i < target.size(); ++i ) {
			UF_OBJECT_LOAD_ASSET("ogg")
		}
	}

	// Images
	{
		UF_OBJECT_LOAD_ASSET_HEADER(images)
		for ( size_t i = 0; i < target.size(); ++i ) {
			UF_OBJECT_LOAD_ASSET("png", "jpg", "jpeg")
		}
	}

	// GLTf models
	{
		UF_OBJECT_LOAD_ASSET_HEADER(models)
		for ( size_t i = 0; i < target.size(); ++i ) {
			UF_OBJECT_LOAD_ASSET("gltf", "glb", "graph")
			if ( bind ) uf::instantiator::bind("GraphBehavior", *this);
			
			auto& aMetadata = assetLoader.getComponent<uf::Serializer>();
			aMetadata[filename] = json["metadata"]["model"];
			aMetadata[filename]["root"] = json["root"];
		}
	}
	// Lua scripts
	{
		UF_OBJECT_LOAD_ASSET_HEADER(scripts)
		for ( size_t i = 0; i < target.size(); ++i ) {
			UF_OBJECT_LOAD_ASSET("lua")
			if ( bind ) uf::instantiator::bind("LuaBehavior", *this);
		}
	}
	// Override
	{
		UF_OBJECT_LOAD_ASSET_HEADER()
		override = true;
		for ( size_t i = 0; i < target.size(); ++i ) {
			UF_OBJECT_LOAD_ASSET()
		}
	}
	// Bind behaviors
	{
		if ( json["type"].is<uf::stl::string>() ) uf::instantiator::bind( json["type"].as<uf::stl::string>(), *this );

		uf::Serializer target;
		if ( ext::json::isArray( metadata["system"]["behaviors"] ) ) {
			target = metadata["system"]["behaviors"];
		} else if ( ext::json::isArray( json["behaviors"] ) ) {
			target = json["behaviors"];
		}
		for ( size_t i = 0; i < target.size(); ++i ) {
			uf::instantiator::bind( target[i].as<uf::stl::string>(), *this );
		}
	}
	uf::Serializer hooks = metadata["system"]["hooks"];
	// Metadata
	if ( !ext::json::isNull( json["metadata"] ) ) {
		uf::Serializer& metadata = this->getComponent<uf::Serializer>();
		if ( json["metadata"].is<uf::stl::string>() ) {
			uf::stl::string f = json["metadata"].as<uf::stl::string>();
			uf::stl::string filename = uf::io::resolveURI( json["metadata"].as<uf::stl::string>(), json["root"].as<uf::stl::string>() );
			if ( !metadata.readFromFile(filename) ) return false;
		} else {
			metadata = json["metadata"];
		}
	}
	metadata["system"] = json;
	metadata["system"].erase("metadata");
	metadata["system"]["hooks"] = hooks;

	// check for children
	{
		UF_OBJECT_LOAD_ASSET_HEADER(entities)
		for ( size_t i = 0; i < target.size(); ++i ) {
			uf::stl::string canonical = "";
			uf::stl::string hash = ext::json::isObject( target[i] ) ? target[i]["hash"].as<uf::stl::string>() : "";
			uf::stl::string f = ext::json::isObject( target[i] ) ? target[i]["filename"].as<uf::stl::string>() : target[i].as<uf::stl::string>();
			float delay = ext::json::isObject( target[i] ) ? target[i]["delay"].as<float>() : -1;
			uf::stl::string category = ext::json::isObject( target[i] ) ? target[i]["category"].as<uf::stl::string>() : "";\
			uf::stl::string filename = uf::io::resolveURI( f, json["root"].as<uf::stl::string>() );
			if ( category != "" && category != assetCategory ) continue;
			if ( category == "" && uf::io::extension(filename) != "json" ) continue;
			if ( (canonical = assetLoader.load( filename, hash )) == "" ) continue;
			if ( delay > -1 ) {
				uf::Serializer payload;
				payload["filename"] = canonical;
				if ( hash != "" ) payload["hash"] = hash;
				payload["category"] = assetCategory;
				this->queueHook( "asset:Load.%UID%", payload, delay );
				continue;
			}
			{
				uf::Serializer json;
				if ( !json.readFromFile(filename, hash) ) continue;

				json["root"] = uf::io::directory(filename);
				json["source"] = filename;
			#if UF_ENTITY_METADATA_USE_JSON
				json["hot reload"]["mtime"] = uf::io::mtime(filename) + 10;
			#else
				{
					auto& metadata = this->getComponent<uf::ObjectBehavior::Metadata>();
					metadata.hotReload.mtime = uf::io::mtime(filename) + 10;
				}
			#endif

				if ( this->loadChildUid(json) == -1 ) continue;
			}
		}
	}
	// Add lights
	if ( ext::json::isArray( metadata["system"]["lights"] ) ) {
		uf::Serializer target = metadata["system"]["lights"];
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

	return true;
}
uf::Object& uf::Object::loadChild( const uf::stl::string& f, bool initialize ) {
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	uf::Serializer json;
	uf::stl::string filename = uf::io::resolveURI( f, metadata["system"]["root"].as<uf::stl::string>() );
	if ( !json.readFromFile(filename) ) {
		return ::null;
	}

	json["root"] = uf::io::directory(filename);
	json["source"] = filename;
#if UF_ENTITY_METADATA_USE_JSON
	json["hot reload"]["mtime"] = uf::io::mtime(filename) + 10;
#else
	{
		auto& metadata = this->getComponent<uf::ObjectBehavior::Metadata>();
		metadata.hotReload.mtime = uf::io::mtime(filename) + 10;
	}
#endif

	return this->loadChild(json, initialize);
}
uf::Object& uf::Object::loadChild( const uf::Serializer& _json, bool initialize ) {
	uf::Serializer json = _json;
	uf::stl::string type = json["type"].as<uf::stl::string>();
	if ( type == "" ) type = "Object";
	if ( json["ignore"].as<bool>() ) return ::null;

	uf::Entity& entity = uf::instantiator::instantiate(type);
	uf::Object& object = entity.as<uf::Object>();
	this->addChild(entity);
	if ( !object.load(json) ) {
		this->removeChild(entity);
		return ::null;
	}
	if ( initialize ) entity.initialize();
	return object;
}
uf::Object* uf::Object::loadChildPointer( const uf::stl::string& f, bool initialize ) {
	uf::Object* pointer = &this->loadChild(f, initialize);
	return pointer != &::null ? pointer : NULL;
}
uf::Object* uf::Object::loadChildPointer( const uf::Serializer& json, bool initialize ) {
	uf::Object* pointer = &this->loadChild(json, initialize);
	return pointer != &::null ? pointer : NULL;
}
std::size_t uf::Object::loadChildUid( const uf::stl::string& f, bool initialize ) {
	uf::Object* pointer = this->loadChildPointer(f, initialize);
	return pointer ? pointer->getUid() : -1;
}
std::size_t uf::Object::loadChildUid( const uf::Serializer& json, bool initialize ) {
	uf::Object* pointer = this->loadChildPointer(json, initialize);
	return pointer ? pointer->getUid() : -1;
}

uf::stl::string uf::Object::grabURI( const uf::stl::string& filename, const uf::stl::string& root  ) {
	return uf::io::resolveURI( filename, root );
}


#include <uf/utils/string/ext.h>
uf::stl::string uf::string::toString( const uf::Object& object ) {
	uf::stl::stringstream ss;
	ss << object.getName() << ": " << object.getUid();
	return ss.str();
}