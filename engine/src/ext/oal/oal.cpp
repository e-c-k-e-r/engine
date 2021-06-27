#include <uf/config.h>
#if defined(UF_USE_OPENAL)

#include <uf/ext/oal/oal.h>
#include <uf/utils/mempool/mempool.h>
#include <uf/utils/string/io.h>
#include <uf/ext/vorbis/vorbis.h>
#include <iostream>

namespace {
	ALCdevice* device = NULL;
	ALCcontext* context = NULL;
}

void ext::al::initialize() {
/*
	this->m_device = alcOpenDevice(NULL);
	if ( !this->m_device ) { this->checkError(__LINE__);
		return false;
	}

	ALboolean enumeration = alcIsExtensionPresent(NULL, "ALC_ENUMERATION_EXT");
	if (enumeration == AL_FALSE) { this->checkError(__LINE__);
		// do something
		std::cout << "Device enumeration not available" << std::endl;
	}

	this->m_context = alcCreateContext(this->m_device, NULL);
	if ( !alcMakeContextCurrent(this->m_context) ) {
		this->checkError(__LINE__);
		return false;
	}
	return this->m_initialized = true;
*/
	if ( alutInit(NULL, NULL) != AL_TRUE ) {
		UF_EXCEPTION("AL error: " << alutGetErrorString( alutGetError() ) );
	}
}
void ext::al::destroy() {
/*
	this->m_device = alcGetContextsDevice(this->m_context);
	alcMakeContextCurrent(NULL);
	alcDestroyContext(this->m_context);
	alcCloseDevice(this->m_device);
*/
	alutExit();
}
#if 0
void ext::al::listener( ALenum name, ALfloat x ) { AL_CHECK_RESULT(alListenerf( name, x )); }
void ext::al::listener( ALenum name, ALfloat x, ALfloat y, ALfloat z ) { AL_CHECK_RESULT(alListener3f( name, x, y, z )); }
void ext::al::listener( ALenum name, const ALfloat* f ) { AL_CHECK_RESULT(alListenerfv( name, f )); }

void ext::al::listener( const std::string& string, ALfloat x ) {
	// alSourcef
	if ( string == "GAIN" ) return ext::al::listener( AL_GAIN, x );
	UF_MSG_ERROR("AL error: Invalid enum requested: " << string);
}
void ext::al::listener( const std::string& string, ALfloat x, ALfloat y, ALfloat z ) {
	// alSourcefv
	if ( string == "POSITION" ) return ext::al::listener( AL_POSITION, x, y, z );
	if ( string == "VELOCITY" ) return ext::al::listener( AL_VELOCITY, x, y, z );
	UF_MSG_ERROR("AL error: Invalid enum requested: " << string);
}
void ext::al::listener( const std::string& string, const ALfloat* f ) {
	// alSourcefv
	if ( string == "POSITION" ) return ext::al::listener( AL_POSITION, f );
	if ( string == "VELOCITY" ) return ext::al::listener( AL_VELOCITY, f );
	if ( string == "ORIENTATION" ) return ext::al::listener( AL_DIRECTION, f );
	UF_MSG_ERROR("AL error: Invalid enum requested: " << string);
}
#endif
std::string ext::al::getError( ALCenum error ) {
	if ( !error ) error = alGetError();
	switch (error) {
		case AL_NO_ERROR: return "AL_NO_ERROR";
		case AL_INVALID_NAME: return "AL_INVALID_NAME"; // a bad name (ID) was passed to an OpenAL function
		case AL_INVALID_ENUM: return "AL_INVALID_ENUM"; // an invalid enum value was passed to an OpenAL function
		case AL_INVALID_VALUE: return "AL_INVALID_VALUE"; // an invalid value was passed to an OpenAL function
		case AL_INVALID_OPERATION: return "AL_INVALID_OPERATION"; // the requested operation is not valid
		case AL_OUT_OF_MEMORY: return "AL_OUT_OF_MEMORY"; // the requested operation resulted in OpenAL running out ofmemory 
	}
	return "AL_UNKNOWN(" + std::to_string(error) + ")";
}
uf::audio::Metadata* ext::al::create( const std::string& filename, bool streamed, uint8_t buffers ) {
#if UF_MEMORYPOOL_INVALID_MALLOC
	uf::audio::Metadata* pointer = &uf::MemoryPool::global.alloc<uf::audio::Metadata>();
#else
	uf::MemoryPool* memoryPool = uf::MemoryPool::global.size() > 0 ? &uf::MemoryPool::global : NULL;
	uf::audio::Metadata* pointer = (memoryPool) ? &memoryPool->alloc<uf::audio::Metadata>() : new uf::audio::Metadata;
#endif
	uf::audio::Metadata& metadata = *pointer;
	metadata.filename = filename;
	metadata.settings.streamed = streamed;
	metadata.settings.buffers = buffers;
	metadata.extension = uf::io::extension( metadata.filename );

	metadata.al.buffer.initialize( metadata.settings.buffers );
	metadata.al.source.initialize();
	metadata.al.source.set( AL_PITCH, 1.0f );
	metadata.al.source.set( AL_GAIN, 1.0f );
	metadata.al.source.set( AL_LOOPING, metadata.settings.loop ? AL_TRUE : AL_FALSE );
	return pointer;
}
uf::audio::Metadata* ext::al::open( const std::string& filename ) {
	return ext::al::open( filename, uf::audio::streamsByDefault );
}
uf::audio::Metadata* ext::al::open( const std::string& filename, bool streams ) {
	return streams ? ext::al::stream( filename ) : ext::al::load( filename );
}
uf::audio::Metadata* ext::al::load( const std::string& filename ) {
	uf::audio::Metadata* pointer = ext::al::create( filename, false, 1 );
	uf::audio::Metadata& metadata = *pointer;

	if ( metadata.extension == "ogg" ) ext::vorbis::open( metadata );
	return pointer;
}

uf::audio::Metadata* ext::al::stream( const std::string& filename ) {
	uf::audio::Metadata* pointer = ext::al::create( filename, true, uf::audio::buffers );
	uf::audio::Metadata& metadata = *pointer;

	if ( metadata.extension == "ogg" ) ext::vorbis::open( metadata );
	return pointer;
}
void ext::al::update( uf::audio::Metadata& metadata ) {
	if ( metadata.extension == "ogg" ) return ext::vorbis::update( metadata );
}
void ext::al::close( uf::audio::Metadata* metadata ) {
	if ( !metadata ) return;

	ext::al::close( *metadata );

#if UF_MEMORYPOOL_INVALID_FREE
	uf::MemoryPool::global.free( metadata );
#else
	uf::MemoryPool* memoryPool = uf::MemoryPool::global.size() > 0 ? &uf::MemoryPool::global : NULL;	
	if ( memoryPool ) memoryPool->free( metadata );
	else delete metadata;
#endif
}
void ext::al::close( uf::audio::Metadata& metadata ) {
	metadata.al.source.destroy();
	metadata.al.buffer.destroy();
	if ( metadata.extension == "ogg" ) return ext::vorbis::close( metadata );
}

void ext::al::listener( const pod::Transform<>& transform ) {
	float o[6] = { transform.forward.x, transform.forward.y, transform.forward.z, transform.up.x, transform.up.y, transform.up.z };
	AL_CHECK_RESULT(alListener3f( AL_POSITION, transform.position.x, transform.position.y, transform.position.z ));
	AL_CHECK_RESULT(alListener3f( AL_VELOCITY, 0, 0, 0 ));
	AL_CHECK_RESULT(alListenerfv( AL_ORIENTATION, &o[0] ));
//	ext::al::listener( AL_POSITION, transform.position.x, transform.position.y, transform.position.z );
//	ext::al::listener( AL_VELOCITY, 0, 0, 0 );
//	ext::al::listener( AL_ORIENTATION, &o[0] );
}

////////
/*
ext::al::Source::Source() : m_index(0) {
}
ext::al::Source::~Source() {
	this->destroy();
}
*/
void ext::al::Source::initialize() {
	if ( this->m_index ) this->destroy();
	AL_CHECK_RESULT(alGenSources(1, &this->m_index));
}
void ext::al::Source::destroy() {
	if ( this->m_index && alIsSource(this->m_index) ) {
	//	if ( this->playing() ) this->stop();
	//	AL_CHECK_RESULT(alSourcei(this->m_index, AL_BUFFER, 0));
		AL_CHECK_RESULT(alDeleteSources(1, &this->m_index));
	}
	this->m_index = 0;
}
ALuint& ext::al::Source::getIndex() { return this->m_index; }
ALuint ext::al::Source::getIndex() const { return this->m_index; }

void ext::al::Source::get( ALenum name, ALfloat& x ) { AL_CHECK_RESULT(alGetSourcef( this->m_index, name, &x )); }
void ext::al::Source::get( ALenum name, ALfloat& x, ALfloat& y, ALfloat& z ) { AL_CHECK_RESULT(alGetSource3f( this->m_index, name, &x, &y, &z )); }
void ext::al::Source::get( ALenum name, ALfloat* f ) { AL_CHECK_RESULT(alGetSourcefv( this->m_index, name, f )); }

void ext::al::Source::get( ALenum name, ALint& x ) { AL_CHECK_RESULT(alGetSourcei( this->m_index, name, &x )); }
void ext::al::Source::get( ALenum name, ALint& x, ALint& y, ALint& z ) { AL_CHECK_RESULT(alGetSource3i( this->m_index, name, &x, &y, &z )); }
void ext::al::Source::get( ALenum name, ALint* f ) { AL_CHECK_RESULT(alGetSourceiv( this->m_index, name, f )); }

void ext::al::Source::get( const std::string& string, ALfloat& x ) {
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
	UF_MSG_ERROR("AL error: Invalid enum requested: " << string);
}
void ext::al::Source::get( const std::string& string, ALfloat& x, ALfloat& y, ALfloat& z ) {
	// alSourcefv
	if ( string == "POSITION" ) return this->get( AL_POSITION, x, y, z );
	if ( string == "VELOCITY" ) return this->get( AL_VELOCITY, x, y, z );
	if ( string == "DIRECTION" ) return this->get( AL_DIRECTION, x, y, z );
	UF_MSG_ERROR("AL error: Invalid enum requested: " << string);
}
void ext::al::Source::get( const std::string& string, ALfloat* f ) {
	// alSourcefv
	if ( string == "POSITION" ) return this->get( AL_POSITION, f );
	if ( string == "VELOCITY" ) return this->get( AL_VELOCITY, f );
	if ( string == "DIRECTION" ) return this->get( AL_DIRECTION, f );
	UF_MSG_ERROR("AL error: Invalid enum requested: " << string);
}
void ext::al::Source::get( const std::string& string, ALint& x ) {
	// alSourcei
	if ( string == "SOURCE_RELATIVE" ) return this->get( AL_SOURCE_RELATIVE, x );
	if ( string == "CONE_INNER_ANGLE" ) return this->get( AL_CONE_INNER_ANGLE, x );
	if ( string == "CONE_OUTER_ANGLE" ) return this->get( AL_CONE_OUTER_ANGLE, x );
	if ( string == "LOOPING" ) return this->get( AL_LOOPING, x );
	if ( string == "BUFFER" ) return this->get( AL_BUFFER, x );
	if ( string == "SOURCE_STATE" ) return this->get( AL_SOURCE_STATE, x );	
	UF_MSG_ERROR("AL error: Invalid enum requested: " << string);
}
void ext::al::Source::get( const std::string& string, ALint& x, ALint& y, ALint& z ) {
	// alSourceiv
	if ( string == "POSITION" ) return this->get( AL_POSITION, x, y, z );
	if ( string == "VELOCITY" ) return this->get( AL_VELOCITY, x, y, z );
	if ( string == "DIRECTION" ) return this->get( AL_DIRECTION, x, y, z );
	UF_MSG_ERROR("AL error: Invalid enum requested: " << string);
}
void ext::al::Source::get( const std::string& string, ALint* f ) {
	// alSourceiv
	if ( string == "POSITION" ) return this->get( AL_POSITION, f );
	if ( string == "VELOCITY" ) return this->get( AL_VELOCITY, f );
	if ( string == "DIRECTION" ) return this->get( AL_DIRECTION, f );
	UF_MSG_ERROR("AL error: Invalid enum requested: " << string);
}
void ext::al::Source::set( ALenum name, ALfloat x ) { AL_CHECK_RESULT(alSourcef( this->m_index, name, x )); }
void ext::al::Source::set( ALenum name, ALfloat x, ALfloat y, ALfloat z ) { AL_CHECK_RESULT(alSource3f( this->m_index, name, x, y, z )); }
void ext::al::Source::set( ALenum name, const ALfloat* f ) { AL_CHECK_RESULT(alSourcefv( this->m_index, name, f )); }

void ext::al::Source::set( ALenum name, ALint x ) { AL_CHECK_RESULT(alSourcei( this->m_index, name, x )); }
void ext::al::Source::set( ALenum name, ALint x, ALint y, ALint z ) { AL_CHECK_RESULT(alSource3i( this->m_index, name, x, y, z )); }
void ext::al::Source::set( ALenum name, const ALint* f ) { AL_CHECK_RESULT(alSourceiv( this->m_index, name, f )); }

void ext::al::Source::set( const std::string& string, ALfloat x ) {
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
	UF_MSG_ERROR("AL error: Invalid enum requested: " << string);
}
void ext::al::Source::set( const std::string& string, ALfloat x, ALfloat y, ALfloat z ) {
	// alSourcefv
	if ( string == "POSITION" ) return this->set( AL_POSITION, x, y, z );
	if ( string == "VELOCITY" ) return this->set( AL_VELOCITY, x, y, z );
	if ( string == "DIRECTION" ) return this->set( AL_DIRECTION, x, y, z );
	UF_MSG_ERROR("AL error: Invalid enum requested: " << string);
}
void ext::al::Source::set( const std::string& string, const ALfloat* f ) {
	// alSourcefv
	if ( string == "POSITION" ) return this->set( AL_POSITION, f );
	if ( string == "VELOCITY" ) return this->set( AL_VELOCITY, f );
	if ( string == "DIRECTION" ) return this->set( AL_DIRECTION, f );
	UF_MSG_ERROR("AL error: Invalid enum requested: " << string);
}
void ext::al::Source::set( const std::string& string, ALint x ) {
	// alSourcei
	if ( string == "SOURCE_RELATIVE" ) return this->set( AL_SOURCE_RELATIVE, x );
	if ( string == "CONE_INNER_ANGLE" ) return this->set( AL_CONE_INNER_ANGLE, x );
	if ( string == "CONE_OUTER_ANGLE" ) return this->set( AL_CONE_OUTER_ANGLE, x );
	if ( string == "LOOPING" ) return this->set( AL_LOOPING, x );
	if ( string == "BUFFER" ) return this->set( AL_BUFFER, x );
	if ( string == "SOURCE_STATE" ) return this->set( AL_SOURCE_STATE, x );	
	UF_MSG_ERROR("AL error: Invalid enum requested: " << string);
}
void ext::al::Source::set( const std::string& string, ALint x, ALint y, ALint z ) {
	// alSourceiv
	if ( string == "POSITION" ) return this->set( AL_POSITION, x, y, z );
	if ( string == "VELOCITY" ) return this->set( AL_VELOCITY, x, y, z );
	if ( string == "DIRECTION" ) return this->set( AL_DIRECTION, x, y, z );
	UF_MSG_ERROR("AL error: Invalid enum requested: " << string);
}
void ext::al::Source::set( const std::string& string, const ALint* f ) {
	// alSourceiv
	if ( string == "POSITION" ) return this->set( AL_POSITION, f );
	if ( string == "VELOCITY" ) return this->set( AL_VELOCITY, f );
	if ( string == "DIRECTION" ) return this->set( AL_DIRECTION, f );
	UF_MSG_ERROR("AL error: Invalid enum requested: " << string);
}
#if 0
void ext::al::Source::get( ALenum name, std::vector<ALfloat>& parameters ) {
	if ( !this->m_index ) this->initialize();
	switch ( parameters.size() ) {
		case 1: AL_CHECK_RESULT(alGetSourcef( this->m_index, name, &parameters[0] )); break;
		case 3: AL_CHECK_RESULT(alGetSource3f( this->m_index, name, &parameters[0], &parameters[1], &parameters[2] )); break;
		default: AL_CHECK_RESULT(alGetSourcefv( this->m_index, name, &parameters[0] )); break;
	}
}
void ext::al::Source::get( const std::string& string, std::vector<ALfloat>& parameters ) {
	// alSourcef
	if ( string == "PITCH" ) return this->get( AL_PITCH, parameters );
	if ( string == "GAIN" ) return this->get( AL_GAIN, parameters );
	if ( string == "MIN_GAIN" ) return this->get( AL_MIN_GAIN, parameters );
	if ( string == "MAX_GAIN" ) return this->get( AL_MAX_GAIN, parameters );
	if ( string == "MAX_DISTANCE" ) return this->get( AL_MAX_DISTANCE, parameters );
	if ( string == "ROLLOFF_FACTOR" ) return this->get( AL_ROLLOFF_FACTOR, parameters );
	if ( string == "CONE_OUTER_GAIN" ) return this->get( AL_CONE_OUTER_GAIN, parameters );
	if ( string == "CONE_INNER_ANGLE" ) return this->get( AL_CONE_INNER_ANGLE, parameters );
	if ( string == "CONE_OUTER_ANGLE" ) return this->get( AL_CONE_OUTER_ANGLE, parameters );
	if ( string == "REFERENCE_DISTANCE" ) return this->get( AL_REFERENCE_DISTANCE, parameters );
	if ( string == "SEC_OFFSET" ) return this->get( AL_SEC_OFFSET, parameters );
	// alSource3f
	// alSourcefv
	if ( string == "POSITION" ) return this->get( AL_POSITION, parameters );
	if ( string == "VELOCITY" ) return this->get( AL_VELOCITY, parameters );
	if ( string == "DIRECTION" ) return this->get( AL_DIRECTION, parameters );
}
void ext::al::Source::get( ALenum name, std::vector<ALint>& parameters ) {
	if ( !this->m_index ) this->initialize();
	switch ( parameters.size() ) {
		case 1: AL_CHECK_RESULT(alGetSourcei( this->m_index, name, &parameters[0] )); break;
		case 3: AL_CHECK_RESULT(alGetSource3i( this->m_index, name, &parameters[0], &parameters[1], &parameters[2] )); break;
		default: AL_CHECK_RESULT(alGetSourceiv( this->m_index, name, &parameters[0] )); break;
	}
}
void ext::al::Source::get( const std::string& string, std::vector<ALint>& parameters ) {
	// alSourcei
	if ( string == "SOURCE_RELATIVE" ) return this->get( AL_SOURCE_RELATIVE, parameters );
	if ( string == "CONE_INNER_ANGLE" ) return this->get( AL_CONE_INNER_ANGLE, parameters );
	if ( string == "CONE_OUTER_ANGLE" ) return this->get( AL_CONE_OUTER_ANGLE, parameters );
	if ( string == "LOOPING" ) return this->get( AL_LOOPING, parameters );
	if ( string == "BUFFER" ) return this->get( AL_BUFFER, parameters );
	if ( string == "SOURCE_STATE" ) return this->get( AL_SOURCE_STATE, parameters );	
	// alSource3i
	// alSourceiv
	if ( string == "POSITION" ) return this->get( AL_POSITION, parameters );
	if ( string == "VELOCITY" ) return this->get( AL_VELOCITY, parameters );
	if ( string == "DIRECTION" ) return this->get( AL_DIRECTION, parameters );
}

void ext::al::Source::set( ALenum name, const std::vector<ALfloat>& parameters ) {
	if ( !this->m_index ) this->initialize();
	switch ( parameters.size() ) {
		case 1: AL_CHECK_RESULT(alSourcei( this->m_index, name, parameters[0] )); break;
		case 3: AL_CHECK_RESULT(alSource3i( this->m_index, name, parameters[0], parameters[1], parameters[2] )); break;
		default: AL_CHECK_RESULT(alSourceiv( this->m_index, name, &parameters[0] )); break;
	}
}
void ext::al::Source::set( const std::string& string, const std::vector<ALfloat>& parameters ) {
	// alSourcef
	if ( string == "PITCH" ) return this->set( AL_PITCH, parameters );
	if ( string == "GAIN" ) return this->set( AL_GAIN, parameters );
	if ( string == "MIN_GAIN" ) return this->set( AL_MIN_GAIN, parameters );
	if ( string == "MAX_GAIN" ) return this->set( AL_MAX_GAIN, parameters );
	if ( string == "MAX_DISTANCE" ) return this->set( AL_MAX_DISTANCE, parameters );
	if ( string == "ROLLOFF_FACTOR" ) return this->set( AL_ROLLOFF_FACTOR, parameters );
	if ( string == "CONE_OUTER_GAIN" ) return this->set( AL_CONE_OUTER_GAIN, parameters );
	if ( string == "CONE_INNER_ANGLE" ) return this->set( AL_CONE_INNER_ANGLE, parameters );
	if ( string == "CONE_OUTER_ANGLE" ) return this->set( AL_CONE_OUTER_ANGLE, parameters );
	if ( string == "REFERENCE_DISTANCE" ) return this->set( AL_REFERENCE_DISTANCE, parameters );
	if ( string == "SEC_OFFSET" ) return this->set( AL_SEC_OFFSET, parameters );
	// alSource3f
	// alSourcefv
	if ( string == "POSITION" ) return this->set( AL_POSITION, parameters );
	if ( string == "VELOCITY" ) return this->set( AL_VELOCITY, parameters );
	if ( string == "DIRECTION" ) return this->set( AL_DIRECTION, parameters );
}
void ext::al::Source::set( ALenum name, const std::vector<ALint>& parameters ) {
	if ( !this->m_index ) this->initialize();
	switch ( parameters.size() ) {
		case 1: AL_CHECK_RESULT(alSourcei( this->m_index, name, parameters[0] )); break;
		case 3: AL_CHECK_RESULT(alSource3i( this->m_index, name, parameters[0], parameters[1], parameters[2] )); break;
		default: AL_CHECK_RESULT(alSourceiv( this->m_index, name, &parameters[0] )); break;
	}
}
void ext::al::Source::set( const std::string& string, const std::vector<ALint>& parameters ) {
	// alSourcei
	if ( string == "SOURCE_RELATIVE" ) return this->set( AL_SOURCE_RELATIVE, parameters );
	if ( string == "CONE_INNER_ANGLE" ) return this->set( AL_CONE_INNER_ANGLE, parameters );
	if ( string == "CONE_OUTER_ANGLE" ) return this->set( AL_CONE_OUTER_ANGLE, parameters );
	if ( string == "LOOPING" ) return this->set( AL_LOOPING, parameters );
	if ( string == "BUFFER" ) return this->set( AL_BUFFER, parameters );
	if ( string == "SOURCE_STATE" ) return this->set( AL_SOURCE_STATE, parameters );	
	// alSource3i
	// alSourceiv
	if ( string == "POSITION" ) return this->set( AL_POSITION, parameters );
	if ( string == "VELOCITY" ) return this->set( AL_VELOCITY, parameters );
	if ( string == "DIRECTION" ) return this->set( AL_DIRECTION, parameters );
}
#endif

void ext::al::Source::play() {
	AL_CHECK_RESULT(alSourcePlay(this->m_index));
}
void ext::al::Source::stop() {
	AL_CHECK_RESULT(alSourceStop(this->m_index));
}
bool ext::al::Source::playing() {
	ALCenum state;
	AL_CHECK_RESULT(alGetSourcei(this->m_index, AL_SOURCE_STATE, &state));
	return state == AL_PLAYING;
}

////////
/*
ext::al::Buffer::Buffer( size_t size ) {
	this->m_indices.resize(size, 0);
}
ext::al::Buffer::~Buffer() {
	this->destroy();
}
*/
bool ext::al::Buffer::initialized() const {
//	for ( auto i : this->m_indices ) if ( i > 0 && alIsBuffer(i) ) return true;
//	for ( auto i : this->m_indices ) if ( i > 0 ) return true;
//	return false;
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
//	if ( size > 0 ) this->m_indices.resize( size, 0 );
}
ALuint& ext::al::Buffer::getIndex( size_t i ) { return this->m_indices[i]; }
ALuint ext::al::Buffer::getIndex( size_t i ) const { return this->m_indices[i]; }

void ext::al::Buffer::buffer(ALenum format, const ALvoid* data, ALsizei size, ALsizei frequency, size_t i ) {
	if ( !this->initialized() ) this->initialize();
	AL_CHECK_RESULT(alBufferData(this->m_indices[i], format, data, size, frequency));
}
#endif