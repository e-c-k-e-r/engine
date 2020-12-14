#include "behavior.h"

#include <uf/utils/serialize/serializer.h>

#include <uf/utils/audio/audio.h>
#include <uf/utils/math/transform.h>
#include <uf/engine/asset/asset.h>

#include <mutex>

UF_BEHAVIOR_REGISTER_CPP(ext::SoundEmitterBehavior)
#define this ((uf::Object*) &self)

void ext::SoundEmitterBehavior::initialize( uf::Object& self ) {
	auto& metadata = this->getComponent<uf::Serializer>();
	auto& emitter = this->getComponent<uf::SoundEmitter>();
	auto& sounds = emitter.get();
	
	auto& scene = uf::scene::getCurrentScene();
	auto& sMetadata = scene.getComponent<uf::Serializer>();
	auto& assetLoader = scene.getComponent<uf::Asset>();

	if ( !metadata["audio"]["epsilon"].is<float>() )
		metadata["audio"]["epsilon"] = 0.001f;

	// auto& sounds = this->getComponent<std::vector<uf::Audio*>>();
	this->addHook( "sound:Stop.%UID%", [&](ext::json::Value& json){
		std::string filename = json["filename"].as<std::string>();
		for ( size_t i = 0; i < sounds.size(); ++i ) {
			if ( sounds[i]->getFilename() != filename ) continue;
			sounds[i]->destroy();
			delete sounds[i];
			sounds.erase(sounds.begin() + i);
			metadata["sounds"].erase(i);
		}
	});
	this->addHook( "sound:Emit.%UID%", [&](ext::json::Value& json){
		if ( ext::json::isNull(json["volume"]) ) json["volume"] = metadata["audio"]["volume"];
		if ( ext::json::isNull(json["pitch"]) ) json["pitch"] = metadata["audio"]["pitch"];
		if ( ext::json::isNull(json["gain"]) ) json["gain"] = metadata["audio"]["gain"];
		if ( ext::json::isNull(json["rolloffFactor"]) ) json["rolloffFactor"] = metadata["audio"]["rolloffFactor"];
		if ( ext::json::isNull(json["maxDistance"]) ) json["maxDistance"] = metadata["audio"]["maxDistance"];
		if ( ext::json::isNull(json["epsilon"]) ) json["epsilon"] = metadata["audio"]["epsilon"];

		metadata["sounds"].emplace_back(json);
		uf::Audio& audio = emitter.add( json["filename"].as<std::string>(), json["unique"].as<bool>() );

		{
			float volume = 1.0f; 
			if ( json["volume"].is<double>() ) {
				volume = json["volume"].as<float>();
			} else if ( json["volume"].is<std::string>() ) {
				std::string key = json["volume"].as<std::string>();
				if ( sMetadata["volumes"][key].is<double>() ) {
					volume = sMetadata["volumes"][key].as<float>();
				}
			}
			audio.setVolume(volume);
		}
		if ( json["pitch"].is<double>() ) audio.setPitch(json["pitch"].as<float>());
		if ( json["gain"].is<double>() ) audio.setGain(json["gain"].as<float>());
		if ( json["rolloffFactor"].is<double>() ) audio.setRolloffFactor(json["rolloffFactor"].as<float>());
		if ( json["maxDistance"].is<double>() ) audio.setMaxDistance(json["maxDistance"].as<float>());

		audio.play();
	});
	this->addHook( "asset:Load.%UID%", [&](ext::json::Value& json){
		std::string filename = json["filename"].as<std::string>();

		if ( uf::io::extension(filename) != "ogg" ) return;
		const uf::Audio* audioPointer = NULL;
		try { audioPointer = &assetLoader.get<uf::Audio>(filename); } catch ( ... ) {}
		if ( !audioPointer ) return;

		uf::Serializer payload = metadata["audio"];
		payload["filename"] = filename;
		this->callHook("sound:Emit.%UID%", payload);
	});
}
void ext::SoundEmitterBehavior::tick( uf::Object& self ) {
	auto& transform = this->getComponent<pod::Transform<>>();
	auto flatten = uf::transform::flatten( transform );

	auto& metadata = this->getComponent<uf::Serializer>();
	auto& emitter = this->getComponent<uf::SoundEmitter>();
	auto& sounds = emitter.get();

	for ( size_t i = 0; i < sounds.size(); ++i ) {
		auto& audio = *sounds[i];
		auto& json = metadata["sounds"][i];

		if ( json["spatial"].as<bool>() && audio.playing() ) {
			audio.setPosition( flatten.position );
			audio.setOrientation( flatten.orientation );
		}

		if ( json["loop"].as<bool>() ) {
			float current = audio.getTime();
			float end = audio.getDuration();
			float epsilon = json["epsilon"].is<float>() ? json["epsilon"].as<float>() : 0.005f;
			if ( current + epsilon >= end || !audio.playing() ) {
				audio.setTime(0);
				if ( !audio.playing() ) audio.play();
			}
		} else if ( !audio.playing() ) {
			audio.destroy();
			delete sounds[i];
			sounds.erase(sounds.begin() + i);
			metadata["sounds"].erase(i);
		}
	}
}

void ext::SoundEmitterBehavior::render( uf::Object& self ){}
void ext::SoundEmitterBehavior::destroy( uf::Object& self ){}
#undef this