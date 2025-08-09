#include <uf/utils/audio/audio.h>
#include <uf/utils/string/ext.h>

uf::AudioEmitter::~AudioEmitter() {
	this->cleanup(true);
}
bool uf::AudioEmitter::has( const uf::stl::string& filename ) const {
	for ( auto& audio : this->m_container ) if ( audio.getFilename() == filename ) return true;
	return false;
}
uf::Audio& uf::AudioEmitter::add() {
	return this->m_container.emplace_back();
}
uf::Audio& uf::AudioEmitter::add( const uf::stl::string& filename ) {
//	if ( this->has(filename) ) return this->get(filename);
	uf::Audio& sound = this->add();
	sound.open(filename);
	return sound;
}
uf::Audio& uf::AudioEmitter::load( const uf::stl::string& filename ) {
//	if ( this->has(filename) ) return this->get(filename);
	uf::Audio& sound = this->add();
	sound.load(filename);
	return sound;
}
uf::Audio& uf::AudioEmitter::stream( const uf::stl::string& filename ) {
//	if ( this->has(filename) ) return this->get(filename);
	uf::Audio& sound = this->add();
	sound.stream(filename);
	return sound;
}

uf::Audio& uf::AudioEmitter::get( const uf::stl::string& filename ) {
	if ( !this->has(filename) ) return this->add(filename);
	for ( auto& audio : this->m_container ) if ( audio.getFilename() == filename ) return audio;
	return this->add();
}
const uf::Audio& uf::AudioEmitter::get( const uf::stl::string& filename ) const {
	for ( auto& audio : this->m_container ) if ( audio.getFilename() == filename ) return audio;
	return uf::audio::null;
}
uf::AudioEmitter::container_t& uf::AudioEmitter::get() {
	return this->m_container;
}
const uf::AudioEmitter::container_t& uf::AudioEmitter::get() const {
	return this->m_container;
}
void uf::AudioEmitter::update() {
	for ( auto& audio : this->m_container ) if ( audio.playing() ) audio.update();
}
void uf::AudioEmitter::cleanup( bool purge ) {
	for ( size_t i = 0; i < this->m_container.size(); ++i ) {
		if ( !purge && this->m_container[i].playing() ) continue;
		this->m_container[i].destroy();
		this->m_container.erase(this->m_container.begin() + i);
	}
}

//
uf::MappedAudioEmitter::~MappedAudioEmitter() {
	this->cleanup(true);
}
bool uf::MappedAudioEmitter::has( const uf::stl::string& filename ) const {
	return this->m_container.count(filename) > 0;
}
uf::Audio& uf::MappedAudioEmitter::add( const uf::stl::string& filename ) {
	if ( this->has(filename) ) return this->get(filename);

	uf::Audio& sound = this->m_container[filename];
	sound.open(filename);
	return sound;
}

uf::Audio& uf::MappedAudioEmitter::load( const uf::stl::string& filename ) {
	if ( this->has(filename) ) return this->get(filename);

	uf::Audio& sound = this->m_container[filename];
	sound.load(filename);
	return sound;
}
uf::Audio& uf::MappedAudioEmitter::stream( const uf::stl::string& filename ) {
	if ( this->has(filename) ) return this->get(filename);

	uf::Audio& sound = this->m_container[filename];
	sound.stream(filename);
	return sound;
}

uf::Audio& uf::MappedAudioEmitter::get( const uf::stl::string& filename ) {
	if ( !this->has(filename) ) return this->add(filename);
	return this->m_container[filename];
}
const uf::Audio& uf::MappedAudioEmitter::get( const uf::stl::string& filename ) const {
	return this->m_container.at(filename);
}
uf::MappedAudioEmitter::container_t& uf::MappedAudioEmitter::get() {
	return this->m_container;
}
const uf::MappedAudioEmitter::container_t& uf::MappedAudioEmitter::get() const {
	return this->m_container;
}
void uf::MappedAudioEmitter::update() {
	for ( auto& pair : this->m_container ) {
		pair.second.update();
	}
}
void uf::MappedAudioEmitter::cleanup( bool purge ) {
	for ( auto& pair : this->m_container ) {
		if ( purge || !pair.second.playing() ) {
			pair.second.stop();
			pair.second.destroy();
			this->m_container.erase(pair.first);
		}
	}
}