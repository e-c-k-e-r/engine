#include <uf/utils/audio/audio.h>
#include <uf/utils/string/ext.h>

#if defined(UF_USE_OPENAL)
#include <uf/ext/vorbis/vorbis.h>
#endif

UF_API uf::Audio::Audio( const std::string& filename ) : m_filename(filename) {
}
bool UF_API uf::Audio::initialized() {
	if ( !this->m_source.getIndex() ) return false;
	if ( !this->m_buffer.getIndex() ) return false;
	return true;
}
bool UF_API uf::Audio::playing() {
	if ( !this->initialized() ) return false;
	if ( !this->m_source.playing() ) return false;
	return true;
}
void UF_API uf::Audio::load( const std::string& filename ) { if ( filename != "" ) this->m_filename = filename;
	std::string extension = uf::string::extension( this->m_filename );

	std::vector<char> buffer; ALenum format; ALsizei frequency;
	if ( extension == "ogg" ) {
		ext::Vorbis vorbis; vorbis.load( this->m_filename );

		buffer = vorbis.getBuffer();
		format = vorbis.getFormat();
		frequency = vorbis.getFrequency();

		this->m_buffer.generate();
		this->m_source.generate();
	
		this->m_buffer.buffer( format, &buffer[0], buffer.size(), frequency );
		this->m_source.source( "BUFFER", std::vector<ALint>{ this->m_buffer.getIndex() } );

		this->m_source.source( "PITCH", std::vector<ALfloat>{ 1 } );
		this->m_source.source( "GAIN", std::vector<ALfloat>{ 1 } );
		this->m_source.source( "LOOPING", std::vector<ALint>{ AL_FALSE } );
	}
	
}
void UF_API uf::Audio::play() {
	this->m_source.play();
}
void UF_API uf::Audio::stop() {
	this->m_source.stop();
}
void UF_API uf::Audio::setPosition( const pod::Vector3& position ) {
	this->m_source.source("POSITION", std::vector<ALfloat>{position.x, position.y, position.z} );
}
void UF_API uf::Audio::setOrientation( const pod::Quaternion<>& orientation ) {

}

uf::Audio& UF_API uf::SoundEmitter::add( const std::string& filename ) {
	if ( this->m_container.find( filename ) != this->m_container.end() ) return this->get(filename);
	uf::Audio& sound = this->m_container[filename];
	sound.load(filename);
	return sound;
}
uf::Audio& UF_API uf::SoundEmitter::get( const std::string& filename ) {
	return this->m_container[filename];
}
const uf::Audio& UF_API uf::SoundEmitter::get( const std::string& filename ) const {
	return this->m_container.at(filename);
}

void UF_API uf::SoundEmitter::cleanup( bool purge ) {
/*
	for ( uf::Audio& k : this->m_container ) {
		if ( !k.playing() ) this->m_container.erase(k);
	}
*/
}