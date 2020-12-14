#include <uf/utils/audio/audio.h>
#include <uf/utils/string/ext.h>

#if defined(UF_USE_OPENAL)
#include <uf/ext/vorbis/vorbis.h>
#include <uf/ext/oal/oal.h>
#endif

UF_API uf::Audio::Audio( const std::string& filename ) : m_filename(filename) {
}
UF_API uf::Audio::Audio( uf::Audio&& move ) :
	m_filename(move.m_filename),
	m_source(move.m_source),
	m_buffer(move.m_buffer)
{
}
UF_API uf::Audio::Audio( const uf::Audio& copy ) :
	m_filename(copy.m_filename)
{

}

bool uf::Audio::mute = false;
uf::Audio uf::Audio::null;

uf::Audio::~Audio() {
	this->destroy();
}

bool UF_API uf::Audio::initialized() {
	if ( !this->m_source.getIndex() ) return false;
	if ( !this->m_buffer.getIndex() ) return false;
	return true;
}
void UF_API uf::Audio::destroy() {
	this->m_source.destroy();
	this->m_buffer.destroy();
}
bool UF_API uf::Audio::playing() {
	if ( !this->initialized() ) return false;
	if ( !this->m_source.playing() ) return false;
	return true;
}
void UF_API uf::Audio::load( const std::string& filename ) {
	if ( uf::Audio::mute ) return;
	if ( this->initialized() ) this->destroy();
	if ( filename != "" ) this->m_filename = filename;
	ALenum format;
	ALsizei frequency;
	std::vector<char> buffer;
	std::string extension = uf::io::extension( this->m_filename );
	if ( extension == "ogg" ) {
		ext::Vorbis vorbis; vorbis.load( this->m_filename );

		buffer = vorbis.getBuffer();
		format = vorbis.getFormat();
		frequency = vorbis.getFrequency();
		this->m_duration = vorbis.getDuration();
	}

	if ( buffer.empty() ) return;

	AL_CHECK_ERROR(this->m_buffer.generate());
	AL_CHECK_ERROR(this->m_source.generate());

	AL_CHECK_ERROR(this->m_buffer.buffer( format, &buffer[0], buffer.size(), frequency ));
	AL_CHECK_ERROR(this->m_source.source( "BUFFER", std::vector<ALint>{ this->m_buffer.getIndex() } ));

	AL_CHECK_ERROR(this->m_source.source( "PITCH", std::vector<ALfloat>{ 1 } ));
	AL_CHECK_ERROR(this->m_source.source( "GAIN", std::vector<ALfloat>{ 1 } ));
	AL_CHECK_ERROR(this->m_source.source( "LOOPING", std::vector<ALint>{ AL_FALSE } ));
}

void UF_API uf::Audio::play() {
	if ( !this->initialized() ) return;
	this->m_source.play();
}
void UF_API uf::Audio::stop() {
	if ( !this->initialized() ) return;
	this->m_source.stop();
}
const std::string& UF_API uf::Audio::getFilename() const {
	return this->m_filename;
}

uf::Audio& uf::Audio::operator=( const uf::Audio& copy ) {
	this->m_filename = copy.m_filename;
	return *this;
}

float uf::Audio::getDuration() const {
	return this->m_duration;
}

ALfloat UF_API uf::Audio::getTime() {
	if ( !this->playing() ) return 0;
	ALfloat pos; 
	AL_CHECK_ERROR(alGetSourcef(this->m_source.getIndex(), AL_SEC_OFFSET,  &pos ));
	return pos;
}
void UF_API uf::Audio::setTime( ALfloat pos ) { 
	if ( !this->initialized() ) return;
    this->m_source.source("SEC_OFFSET", std::vector<ALfloat>{ pos } ); 
}

ALfloat UF_API uf::Audio::getPitch() {
	if ( !this->initialized() ) return 0;
	ALfloat pitch; 
	AL_CHECK_ERROR(alGetSourcef(this->m_source.getIndex(), AL_PITCH,  &pitch ));
	return pitch;
}
void UF_API uf::Audio::setPitch( ALfloat pitch ) { 
	if ( !this->initialized() ) return;
    this->m_source.source("PITCH", std::vector<ALfloat>{ pitch } ); 
}

ALfloat UF_API uf::Audio::getGain() {
	if ( !this->initialized() ) return 0;
	ALfloat gain; 
	AL_CHECK_ERROR(alGetSourcef(this->m_source.getIndex(), AL_GAIN,  &gain ));
	return gain;
}
void UF_API uf::Audio::setGain( ALfloat gain ) { 
	if ( !this->initialized() ) return;
    this->m_source.source("GAIN", std::vector<ALfloat>{ gain } ); 
}

ALfloat UF_API uf::Audio::getRolloffFactor() {
	if ( !this->initialized() ) return 0;
	ALfloat rolloffFactor; 
	AL_CHECK_ERROR(alGetSourcef(this->m_source.getIndex(), AL_ROLLOFF_FACTOR,  &rolloffFactor ));
	return rolloffFactor;
}
void UF_API uf::Audio::setRolloffFactor( ALfloat rolloffFactor ) { 
	if ( !this->initialized() ) return;
    this->m_source.source("ROLLOFF_FACTOR", std::vector<ALfloat>{ rolloffFactor } ); 
}

ALfloat UF_API uf::Audio::getMaxDistance() {
	if ( !this->initialized() ) return 0;
	ALfloat maxDistance; 
	AL_CHECK_ERROR(alGetSourcef(this->m_source.getIndex(), AL_MAX_DISTANCE,  &maxDistance ));
	return maxDistance;
}
void UF_API uf::Audio::setMaxDistance( ALfloat maxDistance ) { 
	if ( !this->initialized() ) return;
    this->m_source.source("MAX_DISTANCE", std::vector<ALfloat>{ maxDistance } ); 
}

void UF_API uf::Audio::setPosition( const pod::Vector3& position ) {
	if ( !this->initialized() ) return;
	this->m_source.source("POSITION", std::vector<ALfloat>{position.x, position.y, position.z} );
}
void UF_API uf::Audio::setOrientation( const pod::Quaternion<>& orientation ) {

}

void UF_API uf::Audio::setVolume( float volume ) {
	this->m_source.source("GAIN", std::vector<ALfloat>{volume} );
}
float UF_API uf::Audio::getVolume() const {
	ALfloat pos; 
	AL_CHECK_ERROR(alGetSourcef(this->m_source.getIndex(), AL_GAIN,  &pos ));	
	return pos;
}
//

uf::SoundEmitter::~SoundEmitter() {
	this->cleanup(true);
}
bool uf::SoundEmitter::has( const std::string& filename ) const {
	for ( auto* pointer : this->m_container ) {
		if ( pointer->getFilename() == filename ) return true;
	}
	return false;
}
uf::Audio& UF_API uf::SoundEmitter::add( const std::string& filename, bool unique ) {
	if ( unique && this->has(filename) ) return this->get(filename);

	uf::Audio& sound = *this->m_container.emplace_back(new uf::Audio);
	sound.load(filename);
	return sound;
}
uf::Audio& UF_API uf::SoundEmitter::add( const uf::Audio& audio, bool unique ) {
	return this->add(audio.getFilename(), unique);
}
uf::Audio& UF_API uf::SoundEmitter::get( const std::string& filename ) {
	if ( !this->has(filename) ) return this->add(filename);
	for ( auto* pointer : this->m_container ) {
		if ( pointer->getFilename() == filename ) return *pointer;
	}
	return uf::Audio::null;
}
const uf::Audio& UF_API uf::SoundEmitter::get( const std::string& filename ) const {
	for ( auto* pointer : this->m_container ) {
		if ( pointer->getFilename() == filename ) return *pointer;
	}
	return uf::Audio::null;
}
uf::SoundEmitter::container_t& UF_API uf::SoundEmitter::get() {
	return this->m_container;
}
const uf::SoundEmitter::container_t& UF_API uf::SoundEmitter::get() const {
	return this->m_container;
}
void UF_API uf::SoundEmitter::cleanup( bool purge ) {
	for ( size_t i = 0; i < this->m_container.size(); ++i ) {
		if ( !purge && this->m_container[i]->playing() ) continue;
		this->m_container[i]->destroy();
		delete this->m_container[i];
		this->m_container.erase(this->m_container.begin() + i);
	}
}

//
uf::MappedSoundEmitter::~MappedSoundEmitter() {
	this->cleanup(true);
}
bool uf::MappedSoundEmitter::has( const std::string& filename ) const {
	return this->m_container.count(filename) > 0;
}
uf::Audio& UF_API uf::MappedSoundEmitter::add( const std::string& filename ) {
	if ( this->has(filename) ) return this->get(filename);

	this->m_container.emplace(filename, new uf::Audio);
	uf::Audio& sound = this->get(filename);
	sound.load(filename);
	return sound;
}
uf::Audio& UF_API uf::MappedSoundEmitter::add( const uf::Audio& audio ) {
	return this->add(audio.getFilename());
}
uf::Audio& UF_API uf::MappedSoundEmitter::get( const std::string& filename ) {
	if ( !this->has(filename) ) return this->add(filename);
	return *this->m_container[filename];
}
const uf::Audio& UF_API uf::MappedSoundEmitter::get( const std::string& filename ) const {
	return *this->m_container.at(filename);
}
uf::MappedSoundEmitter::container_t& UF_API uf::MappedSoundEmitter::get() {
	return this->m_container;
}
const uf::MappedSoundEmitter::container_t& UF_API uf::MappedSoundEmitter::get() const {
	return this->m_container;
}
void UF_API uf::MappedSoundEmitter::cleanup( bool purge ) {
	for ( auto& pair : this->m_container ) {
		if ( purge || !pair.second->playing() ) {
			pair.second->stop();
			pair.second->destroy();
			delete pair.second;
			this->m_container.erase(pair.first);
		}
	}
}