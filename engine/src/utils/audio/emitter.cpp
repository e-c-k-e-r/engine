#include <uf/utils/audio/audio.h>
#include <uf/utils/string/ext.h>

uf::AudioEmitter::~AudioEmitter() {
	this->cleanup(true);
}

pod::AudioSource& uf::AudioEmitter::emit( const uf::stl::string& key, pod::AudioClip* clip, bool unique ) {
	auto& pool = this->m_container[key];

	if ( unique && !pool.empty() ) {
		pod::AudioSource& source = pool.front();
		if ( !source.alSource.playing() ) uf::audio::bind( source, clip );
		return source;
	}

	for ( auto& source : pool ) {
		if ( !source.alSource.playing() ) {
			uf::audio::bind( source, clip );
			return source;
		}
	}

	pod::AudioSource& source = pool.emplace_back();
	uf::audio::initialize( source );
	uf::audio::bind( source, clip );
	return source;
/*
	if ( unique && !pool.empty() ) {
		pod::AudioSource& source = pool.front();
		uf::audio::stop( source );
		uf::audio::bind( source, clip );

		return source;
	}

	for ( auto& source : pool ) {
		if ( !source.alSource.playing() ) {
			uf::audio::stop( source );
			uf::audio::bind( source, clip );
			return source;
		}
	}

	pod::AudioSource& source = pool.emplace_back();
	uf::audio::initialize( source );
	uf::audio::bind( source, clip );
	return source;
*/
}

void uf::AudioEmitter::update() {
	for ( auto& pair : this->m_container ) {
		for ( auto& source : pair.second ) {
			bool isPlaying = source.alSource.playing();
			bool hasPending = !source.info.pending.empty();

			ALint queued = 0;
			source.alSource.get(AL_BUFFERS_QUEUED, queued);

			if ( isPlaying || hasPending || queued > 0 ) {
				uf::audio::update( source );
			}
		}
	}
}

void uf::AudioEmitter::update( const pod::Vector3f& position, const pod::Quaternion<>& orientation ) {
	bool spatial = false;
	for ( auto& pair : this->m_container ) {
		for ( auto& source : pair.second ) {
			if ( source.alSource.playing() && source.settings.spatial ) {
				spatial = true;
				break;
			}
		}
		if ( spatial ) break;
	}

	float occlusionValue = 1.0f;
	if ( spatial ) {
		occlusionValue = uf::audio::occlusion( position );

		if ( this->m_acousticTimer.elapsed().asDouble() >= 0.1 ) {
			this->updateAcoustics( position, orientation );
			this->m_acousticTimer.reset();
		}
	}

	for ( auto& pair : this->m_container ) {
		for ( auto& source : pair.second ) {
			bool isPlaying = source.alSource.playing();
			bool hasPending = !source.info.pending.empty();

			ALint queued = 0;
			source.alSource.get(AL_BUFFERS_QUEUED, queued);

			if ( !isPlaying && !hasPending && queued == 0 ) continue;

			if ( source.settings.spatial ) {
				uf::audio::position( source, position );
				uf::audio::orientation( source, orientation );
				uf::audio::occlude( source, occlusionValue );

				if ( m_efxInitialized ) {
					AL_CHECK_RESULT(alSource3i( source.alSource.getIndex(), AL_AUXILIARY_SEND_FILTER, (ALint)m_effectSlot.getIndex(), 0, AL_FILTER_NULL ));
				}
			} else {
				if ( m_efxInitialized ) {
					AL_CHECK_RESULT(alSource3i( source.alSource.getIndex(), AL_AUXILIARY_SEND_FILTER, 0, 0, AL_FILTER_NULL ));
				}
			}

			uf::audio::update( source );
		}
	}
}

void uf::AudioEmitter::updateAcoustics( const pod::Vector3f& position, const pod::Quaternion<>& orientation ) {
	
	if ( !m_efxInitialized ) {
		m_effect.initialize();
		m_effectSlot.initialize();
		m_efxInitialized = true;
	}

	float energy = 0.0f;
	float distance = 0.0f;
	int bounces = 0;

	uf::audio::acoustics( position, orientation, energy, distance, bounces );

	if ( bounces == 0 ) {
		m_effect.set( AL_REVERB_GAIN, 0.0f );
		m_effectSlot.set( AL_EFFECTSLOT_EFFECT, (ALint)m_effect.getIndex() );
		return;
	}

	float avgEnergy = energy / bounces;
	float avgDistance = distance / bounces;
	float decayTime = std::max( 0.1f, std::min( (avgDistance / 15.0f) * avgEnergy, 1.5f ) );
	float gain = avgEnergy * 0.3f;
	float hfRatio = std::max( 0.1f, 1.0f - (avgDistance / 30.0f) );

	m_effect.set( AL_REVERB_DECAY_TIME, decayTime );
	m_effect.set( AL_REVERB_GAIN, gain );
	m_effect.set( AL_REVERB_DECAY_HFRATIO, hfRatio );
	m_effect.set( AL_REVERB_DIFFUSION, 1.0f );
	m_effectSlot.set( AL_EFFECTSLOT_EFFECT, (ALint)m_effect.getIndex() );
}

void uf::AudioEmitter::cleanup( bool purge ) {
	for ( auto mapIt = this->m_container.begin(); mapIt != this->m_container.end(); ) {
		auto& pool = mapIt->second;

		for ( auto vecIt = pool.begin(); vecIt != pool.end(); ) {
			auto& source = *vecIt;

			ALint state;
			alGetSourcei( source.alSource.getIndex(), AL_SOURCE_STATE, &state );
			bool active = (state == AL_PLAYING || state == AL_PAUSED);

			if ( purge || (!active && !source.settings.loop) ) {
				uf::audio::destroy( source );
				vecIt = pool.erase(vecIt);
			} else {
				++vecIt;
			}
		}

		if ( pool.empty() ) {
			mapIt = this->m_container.erase(mapIt);
		} else {
			++mapIt;
		}
	}
}

bool uf::AudioEmitter::has( const uf::stl::string& key ) const {
	auto it = this->m_container.find(key);
	return it != this->m_container.end() && !it->second.empty();
}

pod::AudioSource& uf::AudioEmitter::get( const uf::stl::string& key ) {
	return this->m_container[key].front();
}

const pod::AudioSource& uf::AudioEmitter::get( const uf::stl::string& key ) const {
	return this->m_container.at(key).front();
}

uf::AudioEmitter::container_t& uf::AudioEmitter::get() {
	return this->m_container;
}

const uf::AudioEmitter::container_t& uf::AudioEmitter::get() const {
	return this->m_container;
}