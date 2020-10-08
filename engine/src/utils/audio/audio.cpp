#include <uf/utils/audio/audio.h>
#include <uf/utils/string/ext.h>

#if defined(UF_USE_OPENAL)
#include <uf/ext/vorbis/vorbis.h>
#endif
/*
UF_API uf::Audio::Audio( const std::string& filename ) : m_filename(filename) {
}
UF_API uf::Audio::Audio( uf::Audio&& move ) :
	m_filename(move.m_filename),
	m_source(move.m_source),
	m_buffer(move.m_buffer)
{
}
UF_API uf::Audio::Audio( const uf::Audio& copy ) :
	m_filename(copy.m_filename),
	m_source(copy.m_source),
	m_buffer(copy.m_buffer)
{
}
*/
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
	std::string extension = uf::io::extension( this->m_filename );

	std::vector<char> buffer; ALenum format; ALsizei frequency;
	if ( extension == "ogg" ) {
		ext::Vorbis vorbis; vorbis.load( this->m_filename );

		buffer = vorbis.getBuffer();
		format = vorbis.getFormat();
		frequency = vorbis.getFrequency();
		this->m_duration = vorbis.getDuration();

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
const std::string& UF_API uf::Audio::getFilename() const {
	return this->m_filename;
}
float uf::Audio::getDuration() const {
	return this->m_duration;
}
#include <uf/ext/oal/oal.h>

ALfloat UF_API uf::Audio::getTime() {
	if ( this->playing() ) return 0;
	ALfloat pos; 
	alGetSourcef(this->m_source.getIndex(), AL_SEC_OFFSET,  &pos ); ext::oal.checkError(__FUNCTION__, __LINE__);
	return pos;
}
void UF_API uf::Audio::setTime( ALfloat pos ) { 
	if ( this->playing() ) return;
    this->m_source.source("SEC_OFFSET", std::vector<ALfloat>{ pos } ); 
}

ALfloat UF_API uf::Audio::getPitch() {
	if ( this->playing() ) return 0;
	ALfloat pitch; 
	alGetSourcef(this->m_source.getIndex(), AL_PITCH,  &pitch ); ext::oal.checkError(__FUNCTION__, __LINE__);
	return pitch;
}
void UF_API uf::Audio::setPitch( ALfloat pitch ) { 
	if ( this->playing() ) return;
    this->m_source.source("PITCH", std::vector<ALfloat>{ pitch } ); 
}

ALfloat UF_API uf::Audio::getGain() {
	if ( this->playing() ) return 0;
	ALfloat gain; 
	alGetSourcef(this->m_source.getIndex(), AL_GAIN,  &gain ); ext::oal.checkError(__FUNCTION__, __LINE__);
	return gain;
}
void UF_API uf::Audio::setGain( ALfloat gain ) { 
	if ( this->playing() ) return;
    this->m_source.source("GAIN", std::vector<ALfloat>{ gain } ); 
}

ALfloat UF_API uf::Audio::getRolloffFactor() {
	if ( this->playing() ) return 0;
	ALfloat rolloffFactor; 
	alGetSourcef(this->m_source.getIndex(), AL_ROLLOFF_FACTOR,  &rolloffFactor ); ext::oal.checkError(__FUNCTION__, __LINE__);
	return rolloffFactor;
}
void UF_API uf::Audio::setRolloffFactor( ALfloat rolloffFactor ) { 
	if ( this->playing() ) return;
    this->m_source.source("ROLLOFF_FACTOR", std::vector<ALfloat>{ rolloffFactor } ); 
}

ALfloat UF_API uf::Audio::getMaxDistance() {
	if ( this->playing() ) return 0;
	ALfloat maxDistance; 
	alGetSourcef(this->m_source.getIndex(), AL_MAX_DISTANCE,  &maxDistance ); ext::oal.checkError(__FUNCTION__, __LINE__);
	return maxDistance;
}
void UF_API uf::Audio::setMaxDistance( ALfloat maxDistance ) { 
	if ( this->playing() ) return;
    this->m_source.source("MAX_DISTANCE", std::vector<ALfloat>{ maxDistance } ); 
}

void UF_API uf::Audio::setPosition( const pod::Vector3& position ) {
	this->m_source.source("POSITION", std::vector<ALfloat>{position.x, position.y, position.z} );
}
void UF_API uf::Audio::setOrientation( const pod::Quaternion<>& orientation ) {

}

void UF_API uf::Audio::setVolume( float volume ) {
	this->m_source.source("GAIN", std::vector<ALfloat>{volume} );
}
float UF_API uf::Audio::getVolume() const {
	ALfloat pos; 
	alGetSourcef(this->m_source.getIndex(), AL_GAIN,  &pos ); ext::oal.checkError(__FUNCTION__, __LINE__);	
	return pos;
}

uf::Audio& UF_API uf::SoundEmitter::add( const std::string& filename ) {
	if ( this->m_container.find( filename ) != this->m_container.end() ) return this->get(filename);
	uf::Audio& sound = this->m_container[filename];
	sound.load(filename);
	return sound;
}
uf::Audio& UF_API uf::SoundEmitter::add( const uf::Audio& audio ) {
	std::string filename = audio.getFilename();
	if ( this->m_container.find( filename ) != this->m_container.end() ) return this->get(filename);
	return this->m_container[filename] = audio;
}
uf::Audio& UF_API uf::SoundEmitter::get( const std::string& filename ) {
	return this->m_container[filename];
}
const uf::Audio& UF_API uf::SoundEmitter::get( const std::string& filename ) const {
	return this->m_container.at(filename);
}
uf::SoundEmitter::container_t& UF_API uf::SoundEmitter::get() {
	return this->m_container;
}
const uf::SoundEmitter::container_t& UF_API uf::SoundEmitter::get() const {
	return this->m_container;
}

void UF_API uf::SoundEmitter::cleanup( bool purge ) {
/*
	for ( uf::Audio& k : this->m_container ) {
		if ( !k.playing() ) this->m_container.erase(k);
	}
*/
}