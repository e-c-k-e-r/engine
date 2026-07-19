#include "behavior.h"

#include <uf/utils/serialize/serializer.h>
#include <uf/utils/audio/audio.h>
#include <uf/utils/math/transform.h>
#include <uf/engine/asset/asset.h>

#include <mutex>

UF_BEHAVIOR_REGISTER_CPP(ext::AudioEmitterBehavior)
// TICK IS NOW TRUE!
UF_BEHAVIOR_TRAITS_CPP(ext::AudioEmitterBehavior, ticks = true, renders = false, thread = uf::thread::asyncThreadName)
#define this ((uf::Object*) &self)

void ext::AudioEmitterBehavior::initialize( uf::Object& self ) {
	auto& metadataJson = this->getComponent<uf::Serializer>();
	auto& metadata = this->getComponent<ext::AudioEmitterBehavior::Metadata>();
	auto& emitter = this->getComponent<uf::SoundEmitter>();

	UF_BEHAVIOR_METADATA_BIND_SERIALIZER_HOOKS(metadata, metadataJson);

	this->addHook( "sound:Stop.%UID%", [&](ext::json::Value& json){
		uf::stl::string filename = json["filename"].as<uf::stl::string>();
		auto& pools = emitter.get();
		if ( pools.count(filename) > 0 ) {
			for ( auto& source : pools[filename] ) uf::audio::stop( source );
		}
	});

	this->addHook( "sound:Emit.%UID%", [&]( pod::PCM& waveform ){
		/*
		uf::stl::string fakeUri = "pcm_dynamic_" + std::to_string((size_t)&waveform);
		if ( !uf::asset::has(fakeUri) ) {
			auto payload = uf::asset::resolveToPayload(fakeUri, "");
			payload.type = uf::asset::Type::AUDIO;
			uf::audio::load( clip, waveform );
		}
		*/
	});

	this->addHook( "sound:Emit.%UID%", [&](ext::json::Value& json){
		if ( ext::json::isNull(json["volume"]) ) json["volume"] = metadataJson["audio"]["volume"];
		if ( ext::json::isNull(json["pitch"]) ) json["pitch"] = metadataJson["audio"]["pitch"];
		if ( ext::json::isNull(json["gain"]) ) json["gain"] = metadataJson["audio"]["gain"];
		if ( ext::json::isNull(json["rolloffFactor"]) ) json["rolloffFactor"] = metadataJson["audio"]["rolloffFactor"];
		if ( ext::json::isNull(json["maxDistance"]) ) json["maxDistance"] = metadataJson["audio"]["maxDistance"];
		if ( ext::json::isNull(json["epsilon"]) ) json["epsilon"] = metadataJson["audio"]["epsilon"];
		if ( ext::json::isNull(json["loop"]) ) json["loop"] = metadataJson["audio"]["loop"];
		if ( ext::json::isNull(json["streamed"]) ) json["streamed"] = metadataJson["audio"]["streamed"];
		if ( ext::json::isNull(json["unique"]) ) json["unique"] = metadataJson["audio"]["unique"];
		if ( ext::json::isNull(json["spatial"]) ) json["spatial"] = metadataJson["audio"]["spatial"];

		uf::stl::string filename = json["filename"].as<uf::stl::string>();
		bool unique = json["unique"].as<bool>();
		bool streamed = json["streamed"].as<bool>();

		if ( !uf::asset::has( filename ) ) {
			auto payload = uf::asset::resolveToPayload(filename, "");
			payload.type = uf::asset::Type::AUDIO;
			uf::asset::load( payload );
		}

		pod::AudioClip* clip = &uf::asset::get<pod::AudioClip>( filename );

		pod::AudioSource& source = emitter.emit( filename, clip, unique );

		if ( json["pitch"].is<double>() ) uf::audio::pitch(source, json["pitch"].as<float>());
		if ( json["gain"].is<double>() ) uf::audio::gain(source, json["gain"].as<float>());
		if ( json["rolloffFactor"].is<double>() ) uf::audio::rolloff(source, json["rolloffFactor"].as<float>());
		if ( json["maxDistance"].is<double>() ) uf::audio::maxDistance(source, json["maxDistance"].as<float>());
		if ( json["spatial"].is<bool>() ) source.settings.spatial = json["spatial"].as<bool>();

		if ( json["loop"].is<bool>() ) uf::audio::loop(source, json["loop"].as<bool>());
		else if ( json["wants loop"].is<bool>() ) {
			auto wants = json["wants loop"].as<bool>(true);
			uf::audio::loop(source, wants && clip && clip->info.loop.has);
		}

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

		uf::audio::gain(source, volume * json["gain"].as<float>(1.0f));
		uf::audio::play(source);
	});

	this->addHook( "sound:QueueTrack.%UID%", [&](ext::json::Value& payload){
		uf::stl::string filename = payload["filename"].as<uf::stl::string>();
		int layer = payload["layer"].as<int>(1);
		uf::stl::string channelName = "managed_bgm_channel_" + std::to_string(layer);
		UF_MSG_DEBUG("filename={}, channelName={}", filename, channelName);

		if ( emitter.has( channelName ) ) {
			auto& source = emitter.get(channelName);
			uf::audio::queue( source, filename );
		}
	});

	this->addHook( "sound:PlayTrack.%UID%", [&](ext::json::Value& payload){
		auto filename = payload["filename"].as<uf::stl::string>();

		if ( filename == "" && !metadata.tracks.empty() ) {
			filename = uf::stl::random_it( metadata.tracks.begin(), metadata.tracks.end() )->first;
		}

		if ( metadata.tracks.count( filename ) == 0 && filename != "" ) {
			metadata.tracks[filename] = {};
			metadata.tracks[filename].filename = filename;
		}

		auto& track = metadata.tracks[filename];
		if ( track.intro != "" ) filename = track.intro;
		track.epsilon = metadataJson["audio"]["epsilon"].as<float>(2.5f);

		auto pload = this->resolveToPayload( filename );
		pload.metadata["layer"] = payload["layer"];

		if ( payload["loop"].is<bool>() ) pload.metadata["loop"] = payload["loop"];
		if ( payload["notify"].is<bool>() ) pload.metadata["notify"] = payload["notify"];

		this->callHook( "asset:QueueLoad.%UID%", pload );
	});

	this->addHook( "asset:Load.%UID%", [&](pod::payloads::assetLoad& payload){
		if ( !uf::asset::isExpected( payload, uf::asset::Type::AUDIO ) ) return;
		if ( !uf::asset::has( payload ) ) uf::asset::load( payload );

		if ( metadata.tracks.count(payload.uri) > 0 || metadata.tracks.count(metadata.current) > 0 ) {
			auto& track = metadata.tracks[payload.uri];
			pod::AudioClip* clip = &uf::asset::get<pod::AudioClip>( payload.filename );
			
			int layer = payload.metadata["layer"].as<int>(1);
			uf::stl::string channelName = "managed_bgm_channel_" + std::to_string(layer);

			if ( emitter.has( channelName ) ) {
				uf::audio::stop( emitter.get( channelName ) );
			}
			pod::AudioSource& source = emitter.emit( channelName, clip, true );

		#if UF_AUDIO_MAPPED_VOLUMES
			auto volume = uf::audio::volumes.count("bgm") > 0 ? uf::audio::volumes.at("bgm") : 1.0f;
		#else
			auto volume = uf::audio::volumes::bgm;
		#endif

			bool shouldLoop = payload.metadata["loop"].as<bool>(payload.uri != track.intro);
			uf::audio::gain(source, track.fade.x > 0 ? 0 : volume);
			uf::audio::loop(source, shouldLoop);
			uf::audio::play(source);

			metadata.current = payload.uri;
			track.active = true;

			UF_MSG_DEBUG("Playing: {} (epsilon: {})", metadata.current, track.epsilon);
		} else {
			ext::json::Value json = metadataJson["audio"];
			json["filename"] = payload.filename;
			this->lazyCallHook("sound:Emit.%UID%", json);
		}
	});

	if ( !metadata.tracks.empty() || metadata.current != "" ) {
		ext::json::Value payload;
		payload["filename"] = metadata.current;
		this->queueHook( "sound:PlayTrack.%UID%", payload );
	}
}

void ext::AudioEmitterBehavior::tick( uf::Object& self ) {
	auto& metadata = this->getComponent<ext::AudioEmitterBehavior::Metadata>();
	auto& metadataJson = this->getComponent<uf::Serializer>();
	auto& emitter = this->getComponent<uf::SoundEmitter>();

#if UF_ENTITY_METADATA_USE_JSON
	metadata.deserialize(self, metadataJson);
#endif

	auto& transform = this->getComponent<pod::Transform<>>();
	float distance = uf::audio::distance( transform.position );

	bool bgmFound = false;

	for ( auto& [ name, sources ] : emitter.get() ) {
		for ( auto& source : sources ) {
			if ( name.starts_with("managed_bgm_channel") ) {
				bgmFound = true;
			#if UF_AUDIO_MAPPED_VOLUMES
				auto volume = uf::audio::volumes.count("bgm") > 0 ? uf::audio::volumes.at("bgm") : 1.0f;
			#else
				auto volume = uf::audio::volumes::bgm;
			#endif

				if ( source.clip ) {
					float current = uf::audio::time(source);
					float end = source.clip->info.duration;

					bool fileChanged = (source.clip->filename != metadata.current);
					bool timerWrapped = (metadata.tracks.count(metadata.current) && !metadata.tracks[metadata.current].active && current < 1.0f);

					if ( fileChanged || timerWrapped ) {
						metadata.current = source.clip->filename;
						metadata.tracks[metadata.current].active = true;
					}

					if ( metadata.tracks.count( metadata.current ) > 0 ) {
						auto& track = metadata.tracks[metadata.current];

						if ( track.active ) {
							bool isIntro = metadata.current == track.intro;

							float a = volume;
							if ( track.fade.x > 0.0f && current < track.fade.x ) {
								a *= current / track.fade.x;
							} else if ( track.fade.y > 0.0f && !source.settings.loop && end - current < track.fade.y ) {
								a *= 1.0f - (end - current) / track.fade.y;
							}
							uf::audio::gain(source, a);

							bool timeReached = (current + track.epsilon >= end);
							bool stoppedPlaying = (!source.settings.loop && !source.alSource.playing());

							if ( end > 0 && (timeReached || stoppedPlaying) ) {
								track.active = false;
								if ( isIntro ) {
									auto payload = this->resolveToPayload( track.filename );
									this->callHook( "asset:QueueLoad.%UID%", payload );
								} else if ( !source.settings.loop ) {
									ext::json::Value msg;
									msg["filename"] = metadata.current;
									this->callHook("sound:TrackEnded.%UID%", msg);
								}
							}
						}
					}
				}
				continue;
			}

			if ( source.settings.spatial ) {
				float maxDist = uf::audio::maxDistance(source);
				if ( distance > maxDist * 1.1f ) {
					if ( source.alSource.playing() ) uf::audio::pause( source );
				} else {
					if ( uf::audio::paused(source) ) uf::audio::play( source );
				}
			}
		}
	}

	if ( metadata.tracks.count( metadata.current ) > 0 ) {
		auto& track = metadata.tracks[metadata.current];
		if ( track.active && !bgmFound ) {
			UF_MSG_DEBUG("BGM source vanished! Firing TrackEnded natively.");
			track.active = false;

			bool isIntro = metadata.current == track.intro;
			if ( isIntro ) {
				auto payload = this->resolveToPayload( track.filename );
				this->callHook( "asset:QueueLoad.%UID%", payload );
			} else {
				ext::json::Value msg;
				msg["filename"] = metadata.current;
				this->callHook("sound:TrackEnded.%UID%", msg);
			}
		}
	}
}

void ext::AudioEmitterBehavior::render( uf::Object& self ){}
void ext::AudioEmitterBehavior::destroy( uf::Object& self ){}
void ext::AudioEmitterBehavior::Metadata::serialize( uf::Object& self, uf::Serializer& serializer ){
	for ( auto it : /*this->*/tracks ) {
		serializer["bgm"]["tracks"][it.first]["filename"] = it.first;
		serializer["bgm"]["tracks"][it.first]["intro"] = it.second.intro;
		serializer["bgm"]["tracks"][it.first]["filename"] = it.second.filename;
		serializer["bgm"]["tracks"][it.first]["epsilon"] = it.second.epsilon;
		serializer["bgm"]["tracks"][it.first]["fade"] = ext::json::encode( it.second.fade );
	}
}
void ext::AudioEmitterBehavior::Metadata::deserialize( uf::Object& self, uf::Serializer& serializer ){
	// iterate by value instead of reference because some keys might not exist and jsoncpp is agony
	ext::json::forEach( serializer["bgm"]["tracks"], [&]( const uf::stl::string& key, ext::json::Value value ){
		auto& track = /*this->*/tracks[key];
		track.filename = key;
		track.intro = value["intro"].as(track.intro);
		track.epsilon = value["epsilon"].as(track.epsilon);
		if ( value["fade"].is<bool>() ) {
			track.fade = { track.epsilon, track.epsilon };
		} else if ( value["fade"].is<float>() ) {
			track.fade = { value["fade"].as<float>(), value["fade"].as<float>() };
		} else {
			// YUC
			track.fade = ext::json::decode<float,2>( value["fade"], track.fade );
		}

		if ( track.intro != "" ) tracks[track.intro] = track;
	});

	if ( serializer["bgm"]["load"].is<uf::stl::string>() ) {
		/*this->*/current = serializer["bgm"]["load"].as(/*this->*/current);
	}
}
#undef this