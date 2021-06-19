#include <uf/utils/audio/audio.h>
#include <uf/utils/string/ext.h>

#if defined(UF_USE_OPENAL)
#include <uf/ext/vorbis/vorbis.h>
#include <uf/ext/oal/oal.h>
#endif

bool uf::audio::muted = false;
bool uf::audio::streamsByDefault = true;
uint8_t uf::audio::buffers = 4;
size_t uf::audio::bufferSize = 1024 * 16;
uf::Audio uf::audio::null;

bool uf::Audio::initialized() const {
	return this->m_metadata && this->m_metadata->al.source.getIndex();
}
bool uf::Audio::playing() const {
	return this->m_metadata && this->m_metadata->al.source.playing();
}

void uf::Audio::open( const std::string& filename ) {
	this->open( filename, uf::audio::streamsByDefault );
}
void uf::Audio::open( const std::string& filename, bool streamed ) {
	streamed ? stream( filename ) : load( filename );
}
void uf::Audio::load( const std::string& filename ) {
	if ( uf::audio::muted ) return;
	if ( this->m_metadata ) ext::al::close( *this->m_metadata );
	this->m_metadata = ext::al::load( filename );
}
void uf::Audio::stream( const std::string& filename ) {
	if ( uf::audio::muted ) return;
	if ( this->m_metadata ) ext::al::close( *this->m_metadata );
	this->m_metadata = ext::al::stream( filename );
}
void uf::Audio::update() {
	if ( !this->m_metadata ) return;
	ext::al::update( *this->m_metadata );
}
void uf::Audio::destroy() {
	if ( !this->m_metadata ) return;
	ext::al::close( this->m_metadata );
	this->m_metadata = NULL;
}

void uf::Audio::play() {
	if ( !this->m_metadata ) return;
	this->m_metadata->al.source.play();
}
void uf::Audio::stop() {
	if ( !this->m_metadata ) return;
	this->m_metadata->al.source.stop();
}
void uf::Audio::loop( bool x ) {
	if ( !this->m_metadata ) return;
	this->m_metadata->settings.loop = x;
	if ( !this->m_metadata->settings.streamed ) {
	}
	this->m_metadata->al.source.set( AL_LOOPING, x ? AL_TRUE : AL_FALSE );
}
bool uf::Audio::loops() const {
	if ( !this->m_metadata ) return false;
	return this->m_metadata->settings.loop;
}

float uf::Audio::getTime() const {
	if ( !this->playing() ) return 0;

	float v;
	this->m_metadata->al.source.get( AL_SEC_OFFSET, v );
	return v;
}
void uf::Audio::setTime( float v ) {
	if ( !this->m_metadata ) return;
	this->m_metadata->al.source.set( AL_SEC_OFFSET, v ); 
}

void uf::Audio::setPosition( const pod::Vector3f& v ) {
	if ( !this->m_metadata ) return;
	this->m_metadata->al.source.set( AL_POSITION, &v[0] );
}
void uf::Audio::setOrientation( const pod::Quaternion<>& v ) {
	if ( !this->m_metadata ) return;

}

float uf::Audio::getPitch() const {
	if ( !this->m_metadata ) return 0;
	float v;
	this->m_metadata->al.source.get( AL_PITCH, v );
	return v;
}
void uf::Audio::setPitch( float v ) {
	if ( !this->m_metadata ) return;
	this->m_metadata->al.source.set( AL_PITCH, v );
}

float uf::Audio::getGain() const {
	if ( !this->m_metadata ) return 0;
	float v;
	this->m_metadata->al.source.get( AL_GAIN, v );
	return v;
}
void uf::Audio::setGain( float v ) {
	if ( !this->m_metadata ) return;
	this->m_metadata->al.source.set( AL_GAIN, v );
}

float uf::Audio::getRolloffFactor() const {
	if ( !this->m_metadata ) return 0;
	float v;
	this->m_metadata->al.source.get( AL_ROLLOFF_FACTOR, v );
	return v;
}
void uf::Audio::setRolloffFactor( float v ) {
	if ( !this->m_metadata ) return;
	this->m_metadata->al.source.set( AL_ROLLOFF_FACTOR, v );
}

float uf::Audio::getMaxDistance() const {
	if ( !this->m_metadata ) return 0;
	float v;
	this->m_metadata->al.source.get( AL_MAX_DISTANCE, v );
	return v;
}
void uf::Audio::setMaxDistance( float v ) {
	if ( !this->m_metadata ) return;
	this->m_metadata->al.source.set( AL_MAX_DISTANCE, v );
}

float uf::Audio::getVolume() const { return this->getGain(); }
void uf::Audio::setVolume( float v ) { return this->setGain( v ); }
const std::string& uf::Audio::getFilename() const {
	const std::string null = "";
	return this->m_metadata ? this->m_metadata->filename : null;
}
float uf::Audio::getDuration() const {
	return this->m_metadata ? this->m_metadata->info.duration : 0;
}