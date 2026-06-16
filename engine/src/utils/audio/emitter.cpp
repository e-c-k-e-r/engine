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
}

void uf::AudioEmitter::update() {
	for ( auto& pair : this->m_container ) {
		for ( auto& source : pair.second ) {
			if ( source.alSource.playing() ) uf::audio::update( source );
		}
	}
}

void uf::AudioEmitter::update( const pod::Vector3f& position, const pod::Quaternion<>& orientation ) {
	for ( auto& pair : this->m_container ) {
		for ( auto& source : pair.second ) {
			if ( !source.alSource.playing() ) continue;
			uf::audio::position( source, position );
			uf::audio::orientation( source, orientation );
			uf::audio::update( source );
		}
	}
}

void uf::AudioEmitter::cleanup( bool purge ) {
	for ( auto mapIt = this->m_container.begin(); mapIt != this->m_container.end(); ) {
		auto& pool = mapIt->second;

		for ( auto vecIt = pool.begin(); vecIt != pool.end(); ) {
			auto& source = *vecIt;
			if ( purge || (!source.alSource.playing() && !source.settings.loop) ) {
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