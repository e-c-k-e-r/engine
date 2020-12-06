#include "behavior.h"

#include <uf/utils/serialize/serializer.h>

#include <uf/utils/audio/audio.h>
#include <uf/utils/math/transform.h>
#include <uf/engine/asset/asset.h>

#include <mutex>

UF_BEHAVIOR_REGISTER_CPP(ext::SoundEmitterBehavior)
#define this ((uf::Object*) &self)

void ext::SoundEmitterBehavior::initialize( uf::Object& self ) {
	auto& emitter = this->getComponent<uf::SoundEmitter>();
	auto& metadata = this->getComponent<uf::Serializer>();
	
	auto& scene = uf::scene::getCurrentScene();
	auto& sMetadata = scene.getComponent<uf::Serializer>();
	auto& assetLoader = scene.getComponent<uf::Asset>();

	this->addHook( "asset:Load.%UID%", [&](ext::json::Value& json){
		std::string filename = json["filename"].as<std::string>();

		if ( uf::io::extension(filename) != "ogg" ) return;
		const uf::Audio* audioPointer = NULL;
		try { audioPointer = &assetLoader.get<uf::Audio>(filename); } catch ( ... ) {}
		if ( !audioPointer ) return;

		uf::Audio& audio = this->getComponent<uf::Audio>(); //emitter.add(filename);
		audio.load(filename);
		{
			float volume = 1.0f; 
			if ( metadata["audio"]["volume"].is<double>() ) {
				volume = metadata["audio"]["volume"].as<float>();
			} else if ( metadata["audio"]["volume"].is<std::string>() ) {
				std::string key = metadata["audio"]["volume"].as<std::string>();
				if ( sMetadata["volumes"][key].is<double>() ) {
					volume = sMetadata["volumes"][key].as<float>();
				}
			}
			audio.setVolume(volume);
		}
		if ( metadata["audio"]["pitch"].is<double>() ) {
			audio.setPitch(metadata["audio"]["pitch"].as<float>());
		}
		if ( metadata["audio"]["gain"].is<double>() ) {
			audio.setGain(metadata["audio"]["gain"].as<float>());
		}
		if ( metadata["audio"]["rolloffFactor"].is<double>() ) {
			audio.setRolloffFactor(metadata["audio"]["rolloffFactor"].as<float>());
		}
		if ( metadata["audio"]["maxDistance"].is<double>() ) {
			audio.setMaxDistance(metadata["audio"]["maxDistance"].as<float>());
		}
		audio.play();
	});
}
void ext::SoundEmitterBehavior::tick( uf::Object& self ) {
	auto& emitter = this->getComponent<uf::SoundEmitter>();
	auto& metadata = this->getComponent<uf::Serializer>();
	auto transform = this->getComponent<pod::Transform<>>();

//	for ( auto pair : emitter.get() ) {
//		uf::Audio& audio = pair.second;
	{
		uf::Audio& audio = this->getComponent<uf::Audio>();
		if ( metadata["audio"]["spatial"].as<bool>() ) {
			audio.setPosition( transform.position );
			audio.setOrientation( transform.orientation );
		}

		if ( metadata["audio"]["loop"].as<bool>() ) {
			float current = audio.getTime();
			float end = audio.getDuration();
			float epsilon = 0.005f;
			if ( current + epsilon >= end || !audio.playing() ) {
				audio.setTime(0);
				if ( !audio.playing() ) audio.play();
			}
		}
	}
}

void ext::SoundEmitterBehavior::render( uf::Object& self ){}
void ext::SoundEmitterBehavior::destroy( uf::Object& self ){}
#undef this