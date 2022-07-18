#include "behavior.h"

#include <uf/utils/serialize/serializer.h>

#include <uf/utils/audio/audio.h>
#include <uf/utils/math/transform.h>
#include <uf/engine/asset/asset.h>

#include <mutex>

UF_BEHAVIOR_REGISTER_CPP(ext::SoundEmitterBehavior)
UF_BEHAVIOR_TRAITS_CPP(ext::SoundEmitterBehavior, ticks = true, renders = false, multithread = true)
#define this ((uf::Object*) &self)
void ext::SoundEmitterBehavior::initialize( uf::Object& self ) {
	auto& metadata = this->getComponent<uf::Serializer>();
	auto& emitter = this->getComponent<uf::SoundEmitter>();
	auto& sounds = emitter.get();
	
	auto& scene = uf::scene::getCurrentScene();
	auto& assetLoader = scene.getComponent<uf::Asset>();

	if ( !metadata["audio"]["epsilon"].is<float>() )
		metadata["audio"]["epsilon"] = 0.001f;

	this->addHook( "sound:Stop.%UID%", [&](ext::json::Value& json){
		uf::stl::string filename = json["filename"].as<uf::stl::string>();
		for ( size_t i = 0; i < sounds.size(); ++i ) {
			if ( sounds[i].getFilename() != filename ) continue;
			sounds[i].destroy();
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
		if ( ext::json::isNull(json["loop"]) ) json["loop"] = metadata["audio"]["loop"];
		if ( ext::json::isNull(json["streamed"]) ) json["streamed"] = metadata["audio"]["streamed"];
		if ( ext::json::isNull(json["unique"]) ) json["unique"] = metadata["audio"]["unique"];
		metadata["sounds"].emplace_back(json);

		uf::stl::string filename = json["filename"].as<uf::stl::string>();
		bool unique = json["unique"].as<bool>();
		bool exists = unique && emitter.has( filename );

		uf::Audio& audio = exists ? emitter.get( filename ) : emitter.add();
		if ( !exists ) {
			if ( assetLoader.has<uf::Audio>(filename) ) {
				audio = std::move( assetLoader.get<uf::Audio>(filename) );
				assetLoader.remove<uf::Audio>(filename);
			} else {
				audio.open( filename, json["streamed"].as<bool>() );
			}
		}

		if ( json["pitch"].is<double>() ) audio.setPitch(json["pitch"].as<float>());
		if ( json["gain"].is<double>() ) audio.setGain(json["gain"].as<float>());
		if ( json["rolloffFactor"].is<double>() ) audio.setRolloffFactor(json["rolloffFactor"].as<float>());
		if ( json["maxDistance"].is<double>() ) audio.setMaxDistance(json["maxDistance"].as<float>());
		if ( json["loop"].is<bool>() ) audio.loop(json["loop"].as<bool>());
		
		float volume = 1.0f; 
		if ( json["volume"].is<double>() ) volume = json["volume"].as<float>();
		else if ( json["volume"].is<uf::stl::string>() ) {
			uf::stl::string key = json["volume"].as<uf::stl::string>();
		#if UF_AUDIO_MAPPED_VOLUMES
			if ( uf::audio::volumes.count(key) > 0 ) volume = uf::audio::volumes.at(key);
		#else
			if ( key == "bgm" ) volume = uf::audio::volumes::bgm;
			else if ( key == "sfx" ) volume = uf::audio::volumes::sfx;
			else if ( key == "voice" ) volume = uf::audio::volumes::voice;
		#endif
		}
		audio.setVolume(volume);
		
		audio.play();
	});
	this->addHook( "asset:Load.%UID%", [&](pod::payloads::assetLoad& payload){
		if ( !uf::Asset::isExpected( payload, uf::Asset::Type::AUDIO ) ) return;

		if ( !assetLoader.has<uf::Audio>(payload.filename) ) return;
		
		ext::json::Value json = metadata["audio"];
		json["filename"] = payload.filename;
		this->callHook("sound:Emit.%UID%", json);
	});
}
void ext::SoundEmitterBehavior::tick( uf::Object& self ) {
	auto& transform = this->getComponent<pod::Transform<>>();
	auto flatten = uf::transform::flatten( transform );

	auto& metadata = this->getComponent<uf::Serializer>();
	auto& emitter = this->getComponent<uf::SoundEmitter>();
	auto& sounds = emitter.get();

	for ( size_t i = 0; i < sounds.size(); ++i ) {
		auto& audio = sounds[i];
		auto& json = metadata["sounds"][i];

		if ( json["spatial"].as<bool>() && audio.playing() ) {
			audio.setPosition( flatten.position );
			audio.setOrientation( flatten.orientation );
		}
		if ( audio.loops() && !audio.playing() ) {
			audio.destroy();
			sounds.erase(sounds.begin() + i);
			metadata["sounds"].erase(i);
		}
	}
}

void ext::SoundEmitterBehavior::render( uf::Object& self ){}
void ext::SoundEmitterBehavior::destroy( uf::Object& self ){}
void ext::SoundEmitterBehavior::Metadata::serialize( uf::Object& self, uf::Serializer& serializer ){}
void ext::SoundEmitterBehavior::Metadata::deserialize( uf::Object& self, uf::Serializer& serializer ){}
#undef this