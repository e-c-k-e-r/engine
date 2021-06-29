#include <uf/utils/audio/audio.h>
#include <uf/utils/string/ext.h>

#if defined(UF_USE_OPENAL)
#include <uf/ext/vorbis/vorbis.h>
#include <uf/ext/oal/oal.h>
#endif

uf::SoundEmitter::~SoundEmitter() {
	this->cleanup(true);
}
bool uf::SoundEmitter::has( const uf::stl::string& filename ) const {
	for ( auto& audio : this->m_container ) if ( audio.getFilename() == filename ) return true;
	return false;
}
uf::Audio& UF_API uf::SoundEmitter::add() {
	return this->m_container.emplace_back();
}
uf::Audio& UF_API uf::SoundEmitter::add( const uf::stl::string& filename ) {
//	if ( this->has(filename) ) return this->get(filename);
	uf::Audio& sound = this->add();
	sound.open(filename);
	return sound;
}
uf::Audio& UF_API uf::SoundEmitter::load( const uf::stl::string& filename ) {
//	if ( this->has(filename) ) return this->get(filename);
	uf::Audio& sound = this->add();
	sound.load(filename);
	return sound;
}
uf::Audio& UF_API uf::SoundEmitter::stream( const uf::stl::string& filename ) {
//	if ( this->has(filename) ) return this->get(filename);
	uf::Audio& sound = this->add();
	sound.stream(filename);
	return sound;
}

uf::Audio& UF_API uf::SoundEmitter::get( const uf::stl::string& filename ) {
	if ( !this->has(filename) ) return this->add(filename);
	for ( auto& audio : this->m_container ) if ( audio.getFilename() == filename ) return audio;
	return this->add();
}
const uf::Audio& UF_API uf::SoundEmitter::get( const uf::stl::string& filename ) const {
	for ( auto& audio : this->m_container ) if ( audio.getFilename() == filename ) return audio;
	return uf::audio::null;
}
uf::SoundEmitter::container_t& UF_API uf::SoundEmitter::get() {
	return this->m_container;
}
const uf::SoundEmitter::container_t& UF_API uf::SoundEmitter::get() const {
	return this->m_container;
}
void UF_API uf::SoundEmitter::update() {
	for ( auto& audio : this->m_container ) if ( audio.playing() ) audio.update();
}
void UF_API uf::SoundEmitter::cleanup( bool purge ) {
	for ( size_t i = 0; i < this->m_container.size(); ++i ) {
		if ( !purge && this->m_container[i].playing() ) continue;
		this->m_container[i].destroy();
		this->m_container.erase(this->m_container.begin() + i);
	}
}

//
uf::MappedSoundEmitter::~MappedSoundEmitter() {
	this->cleanup(true);
}
bool uf::MappedSoundEmitter::has( const uf::stl::string& filename ) const {
	return this->m_container.count(filename) > 0;
}
uf::Audio& UF_API uf::MappedSoundEmitter::add( const uf::stl::string& filename ) {
	if ( this->has(filename) ) return this->get(filename);

	uf::Audio& sound = this->m_container[filename];
	sound.open(filename);
	return sound;
}

uf::Audio& UF_API uf::MappedSoundEmitter::load( const uf::stl::string& filename ) {
	if ( this->has(filename) ) return this->get(filename);

	uf::Audio& sound = this->m_container[filename];
	sound.load(filename);
	return sound;
}
uf::Audio& UF_API uf::MappedSoundEmitter::stream( const uf::stl::string& filename ) {
	if ( this->has(filename) ) return this->get(filename);

	uf::Audio& sound = this->m_container[filename];
	sound.stream(filename);
	return sound;
}

uf::Audio& UF_API uf::MappedSoundEmitter::get( const uf::stl::string& filename ) {
	if ( !this->has(filename) ) return this->add(filename);
	return this->m_container[filename];
}
const uf::Audio& UF_API uf::MappedSoundEmitter::get( const uf::stl::string& filename ) const {
	return this->m_container.at(filename);
}
uf::MappedSoundEmitter::container_t& UF_API uf::MappedSoundEmitter::get() {
	return this->m_container;
}
const uf::MappedSoundEmitter::container_t& UF_API uf::MappedSoundEmitter::get() const {
	return this->m_container;
}
void UF_API uf::MappedSoundEmitter::update() {
	for ( auto& pair : this->m_container ) {
		pair.second.update();
	}
}
void UF_API uf::MappedSoundEmitter::cleanup( bool purge ) {
	for ( auto& pair : this->m_container ) {
		if ( purge || !pair.second.playing() ) {
			pair.second.stop();
			pair.second.destroy();
			this->m_container.erase(pair.first);
		}
	}
}