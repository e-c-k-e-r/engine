#include <uf/utils/audio/audio.h>
#include <uf/utils/string/ext.h>

#if UF_USE_OPENAL
	#include <uf/ext/vorbis/vorbis.h>
	#include <uf/ext/oal/oal.h>
#endif

#if UF_USE_OPENAL
	bool uf::audio::muted = false;
#else
	bool uf::audio::muted = true;
#endif

bool uf::audio::streamsByDefault = true;
uint8_t uf::audio::buffers = 4;
size_t uf::audio::bufferSize = 1024 * 16;
uf::Audio uf::audio::null;

#if UF_AUDIO_MAPPED_VOLUMES
	uf::stl::unordered_map<uf::stl::string, float> uf::audio::volumes;
#else
	float uf::audio::volumes::bgm = 1.0f;
	float uf::audio::volumes::sfx = 1.0f;
	float uf::audio::volumes::voice = 1.0f;
#endif

bool uf::Audio::initialized() const {
#if UF_USE_OPENAL
	return this->m_metadata && this->m_metadata->al.source.getIndex();
#else
	return false;
#endif
}
bool uf::Audio::playing() const {
#if UF_USE_OPENAL
	return this->m_metadata && this->m_metadata->al.source.playing();
#else
	return false;
#endif
}

void uf::Audio::open( const uf::stl::string& filename ) {
	this->open( filename, uf::audio::streamsByDefault );
}
void uf::Audio::open( const uf::stl::string& filename, bool streamed ) {
	streamed ? stream( filename ) : load( filename );
}
void uf::Audio::load( const uf::stl::string& filename ) {
	if ( uf::audio::muted ) return;
#if UF_USE_OPENAL
	if ( this->m_metadata ) ext::al::close( *this->m_metadata );
	this->m_metadata = ext::al::load( filename );
#endif
}
void uf::Audio::stream( const uf::stl::string& filename ) {
	if ( uf::audio::muted ) return;
#if UF_USE_OPENAL
	if ( this->m_metadata ) ext::al::close( *this->m_metadata );
	this->m_metadata = ext::al::stream( filename );
#endif
}
void uf::Audio::update() {
#if UF_USE_OPENAL
	if ( !this->m_metadata ) return;
	ext::al::update( *this->m_metadata );
#endif
}
void uf::Audio::destroy() {
#if UF_USE_OPENAL
	if ( !this->m_metadata ) return;
	ext::al::close( this->m_metadata );
	this->m_metadata = NULL;
#endif
}

void uf::Audio::play() {
#if UF_USE_OPENAL
	if ( !this->m_metadata ) return;
	if ( !this->playing() ) {
		this->m_metadata->info.elapsed += this->m_metadata->info.timer.elapsed().asDouble();
		this->m_metadata->info.timer.start();
	}
	this->m_metadata->al.source.play();
#endif

}
void uf::Audio::stop() {
#if UF_USE_OPENAL
	if ( !this->m_metadata ) return;
	this->m_metadata->al.source.stop();
	this->m_metadata->info.timer.stop();
#endif
}
void uf::Audio::loop( bool x ) {
#if UF_USE_OPENAL
	if ( !this->m_metadata ) return;
	this->m_metadata->settings.loop = x;
	if ( !this->m_metadata->settings.streamed ) {
	}
	this->m_metadata->al.source.set( AL_LOOPING, x ? AL_TRUE : AL_FALSE );
#endif
}
bool uf::Audio::loops() const {
#if UF_USE_OPENAL
	if ( !this->m_metadata ) return false;
	return this->m_metadata->settings.loop;
#else
	return false;
#endif
}

float uf::Audio::getTime() const {
#if UF_USE_OPENAL
	if ( !this->m_metadata ) return 0;
	return this->m_metadata->info.elapsed + this->m_metadata->info.timer.elapsed().asDouble();
/*
	if ( !this->playing() ) return 0;

	auto& metadata = *this->m_metadata;

	float v;
	metadata.al.source.get( AL_SEC_OFFSET, v );

	if ( metadata.info.duration >= 0 ) {
		ALint processed = 0;
		metadata.al.source.get(AL_BUFFERS_PROCESSED, processed);
		auto currentlyProcessed = (metadata.settings.buffers - processed) * uf::audio::bufferSize;
		UF_MSG_DEBUG("{} | {} | {}", processed, metadata.settings.buffers, uf::audio::bufferSize);
		auto consumed = metadata.stream.consumed - currentlyProcessed;
		v += (float) consumed / metadata.info.size * metadata.info.duration;
	}

	return v;
*/
#else
	return 0;
#endif
}
void uf::Audio::setTime( float v ) {
#if UF_USE_OPENAL
	if ( !this->m_metadata ) return;
#if UF_USE_OPENAL_ALDC
	if ( v <= 0 ) {
		this->stop();
		this->play();
	}
#else
	this->m_metadata->al.source.set( AL_SEC_OFFSET, v ); 
#endif
#endif
}

void uf::Audio::setPosition( const pod::Vector3f& v ) {
#if UF_USE_OPENAL
	if ( !this->m_metadata ) return;
	this->m_metadata->al.source.set( AL_POSITION, v[0], v[1], v[2] );
#endif
}
void uf::Audio::setOrientation( const pod::Quaternion<>& v ) {
	if ( !this->m_metadata ) return;
}

float uf::Audio::getPitch() const {
#if UF_USE_OPENAL
	if ( !this->m_metadata ) return 0;
	float v;
	this->m_metadata->al.source.get( AL_PITCH, v );
	return v;
#else
	return 0;
#endif
}
void uf::Audio::setPitch( float v ) {
#if UF_USE_OPENAL
	if ( !this->m_metadata ) return;
	this->m_metadata->al.source.set( AL_PITCH, v );
#endif
}

float uf::Audio::getGain() const {
#if UF_USE_OPENAL
	if ( !this->m_metadata ) return 0;
	float v;
	this->m_metadata->al.source.get( AL_GAIN, v );
	return v;
#endif
	return 0;
}
void uf::Audio::setGain( float v ) {
#if UF_USE_OPENAL
	if ( !this->m_metadata ) return;
	this->m_metadata->al.source.set( AL_GAIN, v );
#endif
}

float uf::Audio::getRolloffFactor() const {
#if UF_USE_OPENAL
	if ( !this->m_metadata ) return 0;
	float v;
	this->m_metadata->al.source.get( AL_ROLLOFF_FACTOR, v );
	return v;
#endif
	return 0;
}
void uf::Audio::setRolloffFactor( float v ) {
#if UF_USE_OPENAL
	if ( !this->m_metadata ) return;
	this->m_metadata->al.source.set( AL_ROLLOFF_FACTOR, v );
#endif
}

float uf::Audio::getMaxDistance() const {
#if UF_USE_OPENAL
	if ( !this->m_metadata ) return 0;
	float v;
	this->m_metadata->al.source.get( AL_MAX_DISTANCE, v );
	return v;
#endif
	return 0;
}
void uf::Audio::setMaxDistance( float v ) {
#if UF_USE_OPENAL
	if ( !this->m_metadata ) return;
	this->m_metadata->al.source.set( AL_MAX_DISTANCE, v );
#endif
}

float uf::Audio::getVolume() const { return this->getGain(); }
void uf::Audio::setVolume( float v ) { return this->setGain( v ); }
const uf::stl::string& uf::Audio::getFilename() const {
	static const uf::stl::string null = "";
	return this->m_metadata ? this->m_metadata->filename : null;
}
float uf::Audio::getDuration() const {
	return this->m_metadata ? this->m_metadata->info.duration : 0;
}