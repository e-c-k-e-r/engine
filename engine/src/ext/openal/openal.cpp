#include <uf/config.h>

#if UF_USE_OPENAL && !UF_USE_AICA
#include <uf/ext/openal/openal.h>
#include <uf/utils/memory/pool.h>
#include <uf/utils/string/io.h>
#include <uf/utils/audio/audio.h>

namespace {
	ALCdevice* device = NULL;
	ALCcontext* context = NULL;
}

void ext::al::initialize() {
#if UF_USE_ALUT
	::device = alcOpenDevice(alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER));
	assert(::device);

	::context = alcCreateContext(::device, NULL);
	assert(::context);
	alcMakeContextCurrent(::context);

	if ( alutInit(NULL, NULL) != AL_TRUE ) {
		uf::audio::muted = true;
		UF_EXCEPTION("AL error: {}", alutGetErrorString( alutGetError() ) );
	}
#else
	::device = alcOpenDevice(NULL);
	if ( !::device ) {
		UF_EXCEPTION("{}", ext::al::getError());
	}

	ALboolean enumeration = alcIsExtensionPresent(NULL, "ALC_ENUMERATION_EXT");
	if ( enumeration == AL_FALSE ) {
		// do something
		UF_EXCEPTION("Device enumeration not available");
	}

	::context = alcCreateContext(::device, NULL);
	if ( !alcMakeContextCurrent(::context) ) {
		UF_EXCEPTION(ext::al::getError());
	}
#endif

	ALboolean hasEfx = alcIsExtensionPresent(::device, "ALC_EXT_EFX");
	if ( hasEfx == AL_FALSE ) {
		UF_MSG_WARNING("AL_EXT_EFX not supported! Spatial audio will lack occlusion.");
	}

	UF_MSG_DEBUG("AL initialized.");
}
void ext::al::destroy() {
#if UF_USE_ALUT
	::context = alcGetCurrentContext();
	alcMakeContextCurrent(NULL);
	alcDestroyContext(::context);
	alcCloseDevice(::device);


	alutExit();
#else
	::device = alcGetContextsDevice(::context);
	alcMakeContextCurrent(NULL);
	alcDestroyContext(::context);
	alcCloseDevice(::device);
#endif
}

uf::stl::string ext::al::getError( ALCenum error ) {
	if ( !error ) error = alGetError();
	switch (error) {
		case AL_NO_ERROR: return "AL_NO_ERROR";
		case AL_INVALID_NAME: return "AL_INVALID_NAME"; // a bad name (ID) was passed to an OpenAL function
		case AL_INVALID_ENUM: return "AL_INVALID_ENUM"; // an invalid enum value was passed to an OpenAL function
		case AL_INVALID_VALUE: return "AL_INVALID_VALUE"; // an invalid value was passed to an OpenAL function
		case AL_INVALID_OPERATION: return "AL_INVALID_OPERATION"; // the requested operation is not valid
		case AL_OUT_OF_MEMORY: return "AL_OUT_OF_MEMORY"; // the requested operation resulted in OpenAL running out ofmemory 
	}
	return FMT_FORMAT("AL_UNKNOWN({})", error);
}
//
void ext::al::Listener::set( ALenum name, ALfloat x, ALfloat y, ALfloat z ) {
	AL_CHECK_RESULT_ENUM(alListener3f, name, x, y, z);
}
void ext::al::Listener::set( ALenum name, const ALfloat* values ) {
	alListenerfv(name, values);
}
//
void ext::al::Source::initialize() {
	if ( this->m_index ) this->destroy();
	AL_CHECK_RESULT(alGenSources(1, &this->m_index));
}
void ext::al::Source::destroy() {
	if ( this->m_index && alIsSource(this->m_index) ) {
		AL_CHECK_RESULT(alDeleteSources(1, &this->m_index));
	}
	this->m_index = 0;
}
ALuint& ext::al::Source::getIndex() { return this->m_index; }
ALuint ext::al::Source::getIndex() const { return this->m_index; }

void ext::al::Source::get( ALenum name, ALfloat& x ) const { AL_CHECK_RESULT_ENUM(alGetSourcef, this->m_index, name, &x ); }
void ext::al::Source::get( ALenum name, ALfloat& x, ALfloat& y, ALfloat& z ) const { AL_CHECK_RESULT_ENUM(alGetSource3f, this->m_index, name, &x, &y, &z ); }
void ext::al::Source::get( ALenum name, ALfloat* f ) const { AL_CHECK_RESULT_ENUM(alGetSourcefv, this->m_index, name, f ); }

void ext::al::Source::get( ALenum name, ALint& x ) const { AL_CHECK_RESULT_ENUM(alGetSourcei, this->m_index, name, &x ); }
void ext::al::Source::get( ALenum name, ALint& x, ALint& y, ALint& z ) const { AL_CHECK_RESULT_ENUM(alGetSource3i, this->m_index, name, &x, &y, &z ); }
void ext::al::Source::get( ALenum name, ALint* f ) const { AL_CHECK_RESULT_ENUM(alGetSourceiv, this->m_index, name, f ); }

// string=>enum not used internally at the moment
void ext::al::Source::get( const uf::stl::string& string, ALfloat& x ) const {
	// alSourcef
	if ( string == "PITCH" ) return this->get( AL_PITCH, x );
	if ( string == "GAIN" ) return this->get( AL_GAIN, x );
	if ( string == "MIN_GAIN" ) return this->get( AL_MIN_GAIN, x );
	if ( string == "MAX_GAIN" ) return this->get( AL_MAX_GAIN, x );
	if ( string == "MAX_DISTANCE" ) return this->get( AL_MAX_DISTANCE, x );
	if ( string == "ROLLOFF_FACTOR" ) return this->get( AL_ROLLOFF_FACTOR, x );
	if ( string == "CONE_OUTER_GAIN" ) return this->get( AL_CONE_OUTER_GAIN, x );
	if ( string == "CONE_INNER_ANGLE" ) return this->get( AL_CONE_INNER_ANGLE, x );
	if ( string == "CONE_OUTER_ANGLE" ) return this->get( AL_CONE_OUTER_ANGLE, x );
	if ( string == "REFERENCE_DISTANCE" ) return this->get( AL_REFERENCE_DISTANCE, x );
	if ( string == "SEC_OFFSET" ) return this->get( AL_SEC_OFFSET, x );
	UF_MSG_ERROR("AL error: Invalid enum requested: {}", string);
}
void ext::al::Source::get( const uf::stl::string& string, ALfloat& x, ALfloat& y, ALfloat& z ) const {
	// alSourcefv
	if ( string == "POSITION" ) return this->get( AL_POSITION, x, y, z );
	if ( string == "VELOCITY" ) return this->get( AL_VELOCITY, x, y, z );
	if ( string == "DIRECTION" ) return this->get( AL_DIRECTION, x, y, z );
	UF_MSG_ERROR("AL error: Invalid enum requested: {}", string);
}
void ext::al::Source::get( const uf::stl::string& string, ALfloat* f ) const {
	// alSourcefv
	if ( string == "POSITION" ) return this->get( AL_POSITION, f );
	if ( string == "VELOCITY" ) return this->get( AL_VELOCITY, f );
	if ( string == "DIRECTION" ) return this->get( AL_DIRECTION, f );
	UF_MSG_ERROR("AL error: Invalid enum requested: {}", string);
}
void ext::al::Source::get( const uf::stl::string& string, ALint& x ) const {
	// alSourcei
	if ( string == "SOURCE_RELATIVE" ) return this->get( AL_SOURCE_RELATIVE, x );
	if ( string == "CONE_INNER_ANGLE" ) return this->get( AL_CONE_INNER_ANGLE, x );
	if ( string == "CONE_OUTER_ANGLE" ) return this->get( AL_CONE_OUTER_ANGLE, x );
	if ( string == "LOOPING" ) return this->get( AL_LOOPING, x );
	if ( string == "BUFFER" ) return this->get( AL_BUFFER, x );
	if ( string == "SOURCE_STATE" ) return this->get( AL_SOURCE_STATE, x );	
	UF_MSG_ERROR("AL error: Invalid enum requested: {}", string);
}
void ext::al::Source::get( const uf::stl::string& string, ALint& x, ALint& y, ALint& z ) const {
	// alSourceiv
	if ( string == "POSITION" ) return this->get( AL_POSITION, x, y, z );
	if ( string == "VELOCITY" ) return this->get( AL_VELOCITY, x, y, z );
	if ( string == "DIRECTION" ) return this->get( AL_DIRECTION, x, y, z );
	UF_MSG_ERROR("AL error: Invalid enum requested: {}", string);
}
void ext::al::Source::get( const uf::stl::string& string, ALint* f ) const {
	// alSourceiv
	if ( string == "POSITION" ) return this->get( AL_POSITION, f );
	if ( string == "VELOCITY" ) return this->get( AL_VELOCITY, f );
	if ( string == "DIRECTION" ) return this->get( AL_DIRECTION, f );
	UF_MSG_ERROR("AL error: Invalid enum requested: {}", string);
}
void ext::al::Source::set( ALenum name, ALfloat x ) { AL_CHECK_RESULT_ENUM( alSourcef, this->m_index, name, x ); }
void ext::al::Source::set( ALenum name, ALfloat x, ALfloat y, ALfloat z ) { AL_CHECK_RESULT_ENUM( alSource3f, this->m_index, name, x, y, z ); }
void ext::al::Source::set( ALenum name, const ALfloat* f ) { AL_CHECK_RESULT_ENUM( alSourcefv, this->m_index, name, f ); }

void ext::al::Source::set( ALenum name, ALint x ) { AL_CHECK_RESULT_ENUM( alSourcei, this->m_index, name, x ); }
void ext::al::Source::set( ALenum name, ALint x, ALint y, ALint z ) { AL_CHECK_RESULT_ENUM( alSource3i, this->m_index, name, x, y, z ); }
void ext::al::Source::set( ALenum name, const ALint* f ) { AL_CHECK_RESULT_ENUM( alSourceiv, this->m_index, name, f ); }

void ext::al::Source::set( const uf::stl::string& string, ALfloat x ) {
	// alSourcef
	if ( string == "PITCH" ) return this->set( AL_PITCH, x );
	if ( string == "GAIN" ) return this->set( AL_GAIN, x );
	if ( string == "MIN_GAIN" ) return this->set( AL_MIN_GAIN, x );
	if ( string == "MAX_GAIN" ) return this->set( AL_MAX_GAIN, x );
	if ( string == "MAX_DISTANCE" ) return this->set( AL_MAX_DISTANCE, x );
	if ( string == "ROLLOFF_FACTOR" ) return this->set( AL_ROLLOFF_FACTOR, x );
	if ( string == "CONE_OUTER_GAIN" ) return this->set( AL_CONE_OUTER_GAIN, x );
	if ( string == "CONE_INNER_ANGLE" ) return this->set( AL_CONE_INNER_ANGLE, x );
	if ( string == "CONE_OUTER_ANGLE" ) return this->set( AL_CONE_OUTER_ANGLE, x );
	if ( string == "REFERENCE_DISTANCE" ) return this->set( AL_REFERENCE_DISTANCE, x );
	if ( string == "SEC_OFFSET" ) return this->set( AL_SEC_OFFSET, x );
	UF_MSG_ERROR("AL error: Invalid enum requested: {}", string);
}
void ext::al::Source::set( const uf::stl::string& string, ALfloat x, ALfloat y, ALfloat z ) {
	// alSourcefv
	if ( string == "POSITION" ) return this->set( AL_POSITION, x, y, z );
	if ( string == "VELOCITY" ) return this->set( AL_VELOCITY, x, y, z );
	if ( string == "DIRECTION" ) return this->set( AL_DIRECTION, x, y, z );
	UF_MSG_ERROR("AL error: Invalid enum requested: {}", string);
}
void ext::al::Source::set( const uf::stl::string& string, const ALfloat* f ) {
	// alSourcefv
	if ( string == "POSITION" ) return this->set( AL_POSITION, f );
	if ( string == "VELOCITY" ) return this->set( AL_VELOCITY, f );
	if ( string == "DIRECTION" ) return this->set( AL_DIRECTION, f );
	UF_MSG_ERROR("AL error: Invalid enum requested: {}", string);
}
void ext::al::Source::set( const uf::stl::string& string, ALint x ) {
	// alSourcei
	if ( string == "SOURCE_RELATIVE" ) return this->set( AL_SOURCE_RELATIVE, x );
	if ( string == "CONE_INNER_ANGLE" ) return this->set( AL_CONE_INNER_ANGLE, x );
	if ( string == "CONE_OUTER_ANGLE" ) return this->set( AL_CONE_OUTER_ANGLE, x );
	if ( string == "LOOPING" ) return this->set( AL_LOOPING, x );
	if ( string == "BUFFER" ) return this->set( AL_BUFFER, x );
	if ( string == "SOURCE_STATE" ) return this->set( AL_SOURCE_STATE, x );	
	UF_MSG_ERROR("AL error: Invalid enum requested: {}", string);
}
void ext::al::Source::set( const uf::stl::string& string, ALint x, ALint y, ALint z ) {
	// alSourceiv
	if ( string == "POSITION" ) return this->set( AL_POSITION, x, y, z );
	if ( string == "VELOCITY" ) return this->set( AL_VELOCITY, x, y, z );
	if ( string == "DIRECTION" ) return this->set( AL_DIRECTION, x, y, z );
	UF_MSG_ERROR("AL error: Invalid enum requested: {}", string);
}
void ext::al::Source::set( const uf::stl::string& string, const ALint* f ) {
	// alSourceiv
	if ( string == "POSITION" ) return this->set( AL_POSITION, f );
	if ( string == "VELOCITY" ) return this->set( AL_VELOCITY, f );
	if ( string == "DIRECTION" ) return this->set( AL_DIRECTION, f );
	UF_MSG_ERROR("AL error: Invalid enum requested: {}", string);
}

void ext::al::Source::queue( ALsizei n, ALuint* indices ) {
	AL_CHECK_RESULT(alSourceQueueBuffers(this->m_index, n, indices));
}
void ext::al::Source::unqueue( ALsizei n, ALuint* indices ) {
	AL_CHECK_RESULT(alSourceUnqueueBuffers(this->m_index, n, indices));
}

void ext::al::Source::play() {
	AL_CHECK_RESULT(alSourcePlay(this->m_index));
}
void ext::al::Source::pause() {
	AL_CHECK_RESULT(alSourcePause(this->m_index));
}
void ext::al::Source::stop() {
	AL_CHECK_RESULT(alSourceStop(this->m_index));
}
bool ext::al::Source::playing() const {
	ALCenum state;
	AL_CHECK_RESULT(alGetSourcei(this->m_index, AL_SOURCE_STATE, &state));
	return state == AL_PLAYING;
}

bool ext::al::Buffer::initialized() const {
	return !this->m_indices.empty();
}

void ext::al::Buffer::initialize( size_t size ) {
	if ( this->initialized() ) this->destroy();
	this->m_indices.resize(size, 0);
	AL_CHECK_RESULT(alGenBuffers(size, &this->m_indices[0]));
}
void ext::al::Buffer::destroy() {
	if ( this->initialized() ) AL_CHECK_RESULT(alDeleteBuffers(this->m_indices.size(), &this->m_indices[0]));
	this->m_indices.clear();
}
ALuint& ext::al::Buffer::getIndex( size_t i ) { return this->m_indices[i]; }
ALuint ext::al::Buffer::getIndex( size_t i ) const { return this->m_indices[i]; }

void ext::al::Buffer::set( ALenum name, ALint* iv, size_t i ) {
	AL_CHECK_RESULT(alBufferiv( this->m_indices[i], name, iv ));
}
void ext::al::Buffer::buffer(ALenum format, const ALvoid* data, ALsizei size, ALsizei frequency, size_t i ) {
	if ( !this->initialized() ) this->initialize();
	ext::al::Buffer::buffer( this->m_indices[i], format, data, size, frequency );
}
void ext::al::Buffer::buffer(ALuint index, ALenum format, const ALvoid* data, ALsizei size, ALsizei frequency ) {
	AL_CHECK_RESULT(alBufferData( index, format, data, size, frequency ));
}


#if UF_ENV_DREAMCAST
void ext::al::Filter::initialize() {
}
void ext::al::Filter::destroy() {
}
ALuint ext::al::Filter::getIndex() const {
	return this->m_index;
}
void ext::al::Filter::set( ALenum name, ALfloat x ) {
}
void ext::al::Filter::set( ALenum name, ALint x ) {
}

void ext::al::Effect::initialize() {
}

void ext::al::Effect::destroy() {
}

ALuint ext::al::Effect::getIndex() const { return this->m_index; }
void ext::al::Effect::set( ALenum name, ALfloat x ) {
}
void ext::al::Effect::set( ALenum name, ALint x ) {
}

void ext::al::EffectSlot::initialize() {
}
void ext::al::EffectSlot::destroy() {
}
ALuint ext::al::EffectSlot::getIndex() const { return this->m_index; }
void ext::al::EffectSlot::set( ALenum name, ALfloat x ) {
}
void ext::al::EffectSlot::set( ALenum name, ALint x ) {
}
#else
void ext::al::Filter::initialize() {
	if ( this->m_index ) this->destroy();
	AL_CHECK_RESULT(alGenFilters(1, &this->m_index));
	AL_CHECK_RESULT(alFilteri(this->m_index, AL_FILTER_TYPE, AL_FILTER_LOWPASS));
}
void ext::al::Filter::destroy() {
	if ( this->m_index && alIsFilter(this->m_index) ) {
		AL_CHECK_RESULT(alDeleteFilters(1, &this->m_index));
	}
	this->m_index = 0;
}
ALuint ext::al::Filter::getIndex() const {
	return this->m_index;
}
void ext::al::Filter::set( ALenum name, ALfloat x ) {
	AL_CHECK_RESULT_ENUM( alFilterf, this->m_index, name, x );
}
void ext::al::Filter::set( ALenum name, ALint x ) {
	AL_CHECK_RESULT_ENUM( alFilteri, this->m_index, name, x );
}

void ext::al::Effect::initialize() {
	if ( this->m_index ) this->destroy();

	AL_CHECK_RESULT(alGenEffects(1, &this->m_index));
	AL_CHECK_RESULT(alEffecti(this->m_index, AL_EFFECT_TYPE, AL_EFFECT_REVERB));
}

void ext::al::Effect::destroy() {
	if ( this->m_index && alIsEffect(this->m_index) ) {
		AL_CHECK_RESULT(alDeleteEffects(1, &this->m_index));
	}
	this->m_index = 0;
}

ALuint ext::al::Effect::getIndex() const { return this->m_index; }
void ext::al::Effect::set( ALenum name, ALfloat x ) {
	AL_CHECK_RESULT_ENUM( alEffectf, this->m_index, name, x );
}
void ext::al::Effect::set( ALenum name, ALint x ) {
	AL_CHECK_RESULT_ENUM( alEffecti, this->m_index, name, x );
}

void ext::al::EffectSlot::initialize() {
	if ( this->m_index ) this->destroy();
	AL_CHECK_RESULT(alGenAuxiliaryEffectSlots(1, &this->m_index));
}
void ext::al::EffectSlot::destroy() {
	if ( this->m_index && alIsAuxiliaryEffectSlot(this->m_index) ) {
		AL_CHECK_RESULT(alDeleteAuxiliaryEffectSlots(1, &this->m_index));
	}
	this->m_index = 0;
}
ALuint ext::al::EffectSlot::getIndex() const { return this->m_index; }
void ext::al::EffectSlot::set( ALenum name, ALfloat x ) {
	AL_CHECK_RESULT_ENUM( alAuxiliaryEffectSlotf, this->m_index, name, x );
}
void ext::al::EffectSlot::set( ALenum name, ALint x ) {
	AL_CHECK_RESULT_ENUM( alAuxiliaryEffectSloti, this->m_index, name, x );
}
#endif

#endif