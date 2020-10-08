#include "behavior.h"

#include <uf/utils/serialize/serializer.h>

#include <uf/utils/audio/audio.h>
#include <uf/utils/math/transform.h>
#include <uf/engine/asset/asset.h>

#include <mutex>

EXT_BEHAVIOR_REGISTER_CPP(SoundEmitterBehavior)
#define this ((uf::Scene*) &self)

void ext::SoundEmitterBehavior::initialize( uf::Object& self ) {
	auto& emitter = this->getComponent<uf::SoundEmitter>();
	auto& metadata = this->getComponent<uf::Serializer>();
	
	auto& scene = uf::scene::getCurrentScene();
	auto& sMetadata = this->getComponent<uf::Serializer>();
	auto& assetLoader = scene.getComponent<uf::Asset>();

	this->addHook( "asset:Load.%UID%", [&](const std::string& event)->std::string{	
		uf::Serializer json = event;
		std::string filename = json["filename"].asString();

		if ( uf::io::extension(filename) != "ogg" ) return "false";
		const uf::Audio* audioPointer = NULL;
		try { audioPointer = &assetLoader.get<uf::Audio>(filename); } catch ( ... ) {}
		if ( !audioPointer ) return "false";

		uf::Audio& audio = this->getComponent<uf::Audio>(); //emitter.add(filename);
		audio.load(filename);
		{
			float volume = metadata["audio"]["volume"].isNumeric() ? metadata["audio"]["volume"].asFloat() : sMetadata["volumes"]["sfx"].asFloat();
			audio.setVolume(volume);
		}
		if ( metadata["audio"]["pitch"].isNumeric() ) {
			audio.setPitch(metadata["audio"]["pitch"].asFloat());
		}
		if ( metadata["audio"]["gain"].isNumeric() ) {
			audio.setGain(metadata["audio"]["gain"].asFloat());
		}
		if ( metadata["audio"]["rolloffFactor"].isNumeric() ) {
			audio.setRolloffFactor(metadata["audio"]["rolloffFactor"].asFloat());
		}
		if ( metadata["audio"]["maxDistance"].isNumeric() ) {
			audio.setMaxDistance(metadata["audio"]["maxDistance"].asFloat());
		}
		audio.play();

		return "true";
	});
}
void ext::SoundEmitterBehavior::tick( uf::Object& self ) {
	auto& emitter = this->getComponent<uf::SoundEmitter>();
	auto& metadata = this->getComponent<uf::Serializer>();
	auto& transform = this->getComponent<pod::Transform<>>();

//	for ( auto pair : emitter.get() ) {
//		uf::Audio& audio = pair.second;
	{
		uf::Audio& audio = this->getComponent<uf::Audio>();
		audio.setPosition( transform.position );
		audio.setOrientation( transform.orientation );

		if ( metadata["audio"]["loop"].asBool() ) {
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