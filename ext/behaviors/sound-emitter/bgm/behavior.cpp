#include "behavior.h"

#include <uf/utils/serialize/serializer.h>

#include <uf/utils/audio/audio.h>
#include <uf/utils/math/transform.h>
#include <uf/engine/asset/asset.h>

#include <mutex>

UF_BEHAVIOR_REGISTER_CPP(ext::BgmEmitterBehavior)
UF_BEHAVIOR_TRAITS_CPP(ext::BgmEmitterBehavior, ticks = true, renders = false, multithread = true)
#define this ((uf::Object*) &self)
void ext::BgmEmitterBehavior::initialize( uf::Object& self ) {
	auto& metadata = this->getComponent<ext::BgmEmitterBehavior::Metadata>();
	auto& metadataJson = this->getComponent<uf::Serializer>();

	UF_BEHAVIOR_METADATA_BIND_SERIALIZER_HOOKS(metadata, metadataJson);

	this->addHook( "asset:Load.%UID%", [&](pod::payloads::assetLoad& payload){
		if ( !uf::asset::isExpected( payload, uf::asset::Type::AUDIO ) ) return;
		auto& track = metadata.tracks[payload.uri];

		
		track.userdata = uf::asset::release( payload.filename );
		auto& audio = uf::pointeredUserdata::get<uf::Audio>( track.userdata );

	#if UF_AUDIO_MAPPED_VOLUMES
		auto volume = uf::audio::volumes.count("bgm") > 0 ? uf::audio::volumes.at("bgm") : 1.0;
	#else
		auto volume = uf::audio::volumes::bgm;
	#endif

		audio.setVolume(track.fade.x > 0 ? 0 : volume);
		audio.loop( payload.uri != track.intro );
		audio.play();

		metadata.current = payload.uri;
		track.active = true;
	});

	this->addHook( "bgm:Play.%UID%", [&](ext::json::Value& payload){
		auto filename = payload["filename"].as<uf::stl::string>();
		auto delay = payload["delay"].as<float>(0.0f);

		if ( filename == "" && !metadata.tracks.empty() ) {
			filename = uf::stl::random_it( metadata.tracks.begin(), metadata.tracks.end() )->first;
		}
		if ( metadata.tracks.count( filename ) == 0 ) {
			if ( filename == "" ) return;
		}

		auto& track = metadata.tracks[filename];
		if ( track.intro != "" ) filename = track.intro;
		this->callHook( "asset:QueueLoad.%UID%", this->resolveToPayload( filename ) );
	});

	if ( !metadata.tracks.empty() || metadata.current != "" ) {
		ext::json::Value payload;
		payload["filename"] = metadata.current;
		this->queueHook( "bgm:Play.%UID%", payload );
	}
}
void ext::BgmEmitterBehavior::tick( uf::Object& self ) {
	auto& metadata = this->getComponent<ext::BgmEmitterBehavior::Metadata>();
	auto& metadataJson = this->getComponent<uf::Serializer>();
#if UF_ENTITY_METADATA_USE_JSON
	metadata.deserialize(self, metadataJson);
#endif

#if UF_AUDIO_MAPPED_VOLUMES
	auto volume = uf::audio::volumes.count("bgm") > 0 ? uf::audio::volumes.at("bgm") : 1.0;
#else
	auto volume = uf::audio::volumes::bgm;
#endif

	if ( metadata.tracks.count( metadata.current ) > 0 ) {
		auto& track = metadata.tracks[metadata.current];
		if ( track.active ) {
			auto& audio = uf::pointeredUserdata::get<uf::Audio>( track.userdata );

			float current = audio.getTime();
			float end = audio.getDuration();
			bool isIntro = metadata.current == track.intro;

			float a = volume;
			if ( current < track.fade.x ) {
				a *= current / track.fade.x;
			} else if ( !audio.loops() && end - current < track.fade.y ) {
				a *= 1.0f - (end - current) / track.fade.y;
			}

			audio.setVolume(a);

			if ( audio.loops() ) {
				if ( !audio.playing() ) audio.play();
			} else if ( isIntro && end > 0 && (current + track.epsilon >= end || !audio.playing()) ) {
				auto payload = this->resolveToPayload( track.filename );
				this->callHook( "asset:QueueLoad.%UID%", payload );
			}
		}
	}

	for ( auto it : metadata.tracks ) {
		auto track = metadata.tracks[it.first];
		if ( !track.active ) continue;

		auto& audio = uf::pointeredUserdata::get<uf::Audio>( track.userdata );
		
		if ( audio.playing() ) audio.update();
		if ( it.first == metadata.current ) continue;

		float current = audio.getTime();
		float end = audio.getDuration();

		if ( current >= end && audio.playing() ) {
			audio.stop();
			uf::pointeredUserdata::destroy( track.userdata );
			track.active = false;
			continue;
		}

		float a = volume;
		if ( current < track.fade.x ) {
			a *= current / track.fade.x;
		} else if ( end - current < track.fade.y ) {
			a *= 1.0f - (end - current) / track.fade.y;
		}
		audio.setVolume(a);
	}
}

void ext::BgmEmitterBehavior::render( uf::Object& self ){}
void ext::BgmEmitterBehavior::destroy( uf::Object& self ){
	auto& metadata = this->getComponent<ext::BgmEmitterBehavior::Metadata>();
	for ( auto it : metadata.tracks ) {
		auto track = metadata.tracks[it.first];
		if ( !track.active ) continue;
		auto& audio = uf::pointeredUserdata::get<uf::Audio>( track.userdata );
		audio.stop();
		uf::pointeredUserdata::destroy( track.userdata );
		track.active = false;
	}
}
void ext::BgmEmitterBehavior::Metadata::serialize( uf::Object& self, uf::Serializer& serializer ){
	for ( auto it : /*this->*/tracks ) {
		serializer["bgm"]["tracks"][it.first]["filename"] = it.first;
		serializer["bgm"]["tracks"][it.first]["intro"] = it.second.intro;
		serializer["bgm"]["tracks"][it.first]["filename"] = it.second.filename;
		serializer["bgm"]["tracks"][it.first]["epsilon"] = it.second.epsilon;
		serializer["bgm"]["tracks"][it.first]["fade"] = ext::json::encode( it.second.fade );
	}
}
void ext::BgmEmitterBehavior::Metadata::deserialize( uf::Object& self, uf::Serializer& serializer ){
	ext::json::forEach( serializer["bgm"]["tracks"], [&]( const uf::stl::string& key, const ext::json::Value& value ){
		auto& track = /*this->*/tracks[key];
		track.filename = key;
		track.intro = value["intro"].as(track.intro);
		track.epsilon = value["epsilon"].as(track.epsilon);
		if ( value["fade"].is<bool>() ) {
			track.fade = { track.epsilon, track.epsilon };
		} else if ( value["fade"].is<float>() ) {
			track.fade = { value["fade"].as<float>(), value["fade"].as<float>() };
		} else {
			track.fade = ext::json::decode( value["fade"], track.fade );
		}

		if ( track.intro != "" ) tracks[track.intro] = track;
	});

	if ( serializer["bgm"]["load"].is<uf::stl::string>() ) {
		/*this->*/current = serializer["bgm"]["load"].as(/*this->*/current);
	}
}
#undef this