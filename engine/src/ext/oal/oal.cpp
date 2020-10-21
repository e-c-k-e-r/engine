#include <uf/config.h>
#if defined(UF_USE_OPENAL)

#include <uf/ext/oal/oal.h>
#include <iostream>

ext::AL ext::oal;
UF_API_CALL ext::AL::AL() : m_initialized(false), m_device(NULL) {
}
ext::AL::~AL() {
	this->terminate();
}

bool UF_API_CALL ext::AL::initialize() {
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
	this->m_initialized = alutInit(NULL, NULL)  == AL_TRUE;
	auto error = alutGetError();
	std::cout << alutGetErrorString(error) << std::endl;
	this->checkError(__FUNCTION__, __LINE__);
	return this->m_initialized;
}
bool UF_API_CALL ext::AL::terminate() {
/*
	this->m_device = alcGetContextsDevice(this->m_context);
	alcMakeContextCurrent(NULL);
	alcDestroyContext(this->m_context);
	alcCloseDevice(this->m_device);
*/
	return alutExit();
}
void UF_API_CALL ext::AL::checkError( const std::string& function, int line, const std::string& aux ) const {
	std::string error = this->getError();
	if ( error != "AL_NO_ERROR" ) std::cerr << "AL error @ " << function << ":" << line << ": "<<(aux!=""?"("+aux+") ":"")<< error << std::endl;
	error = alutGetErrorString(alutGetError());
	if ( error != "No ALUT error found" ) std::cerr << "AL error @ " << function << ":" << line << ": "<<(aux!=""?"("+aux+") ":"")<< error << std::endl;
}
std::string UF_API_CALL ext::AL::getError() const {
	ALCenum error = alGetError();
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
void UF_API_CALL ext::AL::listener( ALenum name, std::vector<ALfloat> parameters ) {
	switch ( parameters.size() ) {
		case 1: alListenerf( name, parameters[0] ); break;
		case 3: alListener3f( name, parameters[0], parameters[1], parameters[2] ); break;
		default: alListenerfv( name, &parameters[0] ); break;
	}
	ext::oal.checkError(__FUNCTION__, __LINE__);
}
void UF_API_CALL ext::AL::listener( const std::string string, std::vector<ALfloat> parameters ) {
	if ( string == "GAIN" ) return this->listener( AL_GAIN, parameters );

	if ( string == "POSITION" ) return this->listener( AL_POSITION, parameters );
	if ( string == "VELOCITY" ) return this->listener( AL_VELOCITY, parameters );

	if ( string == "ORIENTATION" ) return this->listener( AL_ORIENTATION, parameters );
}

////////
UF_API_CALL ext::al::Source::Source() : m_index(0) {
}
ext::al::Source::~Source() {
	this->destroy();
}

void UF_API_CALL ext::al::Source::generate() {
	if ( this->m_index ) this->destroy();
	alGenSources(1, &this->m_index); ext::oal.checkError(__FUNCTION__, __LINE__);
}
void UF_API_CALL ext::al::Source::destroy() {
	if ( this->m_index && alIsSource(this->m_index) ) {
		if ( this->playing() ) this->stop();
		alDeleteSources(1, &this->m_index); ext::oal.checkError(__FUNCTION__, __LINE__);
	}
	this->m_index = 0;
}
ALuint& UF_API_CALL ext::al::Source::getIndex() { return this->m_index; }
ALuint UF_API_CALL ext::al::Source::getIndex() const { return this->m_index; }

void UF_API_CALL ext::al::Source::source( ALenum name, std::vector<ALfloat> parameters ) {
	if ( !this->m_index ) this->generate();
	switch ( parameters.size() ) {
		case 1: alSourcef( this->m_index, name, parameters[0] ); break;
		case 3: alSource3f( this->m_index, name, parameters[0], parameters[1], parameters[2] ); break;
		default: alSourcefv( this->m_index, name, &parameters[0] ); break;
	}
	ext::oal.checkError(__FUNCTION__, __LINE__, std::to_string(name));
}
void UF_API_CALL ext::al::Source::source( const std::string string, std::vector<ALfloat> parameters ) {
	// alSourcef
	if ( string == "PITCH" ) return this->source( AL_PITCH, parameters );
	if ( string == "GAIN" ) return this->source( AL_GAIN, parameters );
	if ( string == "MIN_GAIN" ) return this->source( AL_MIN_GAIN, parameters );
	if ( string == "MAX_GAIN" ) return this->source( AL_MAX_GAIN, parameters );
	if ( string == "MAX_DISTANCE" ) return this->source( AL_MAX_DISTANCE, parameters );
	if ( string == "ROLLOFF_FACTOR" ) return this->source( AL_ROLLOFF_FACTOR, parameters );
	if ( string == "CONE_OUTER_GAIN" ) return this->source( AL_CONE_OUTER_GAIN, parameters );
	if ( string == "CONE_INNER_ANGLE" ) return this->source( AL_CONE_INNER_ANGLE, parameters );
	if ( string == "CONE_OUTER_ANGLE" ) return this->source( AL_CONE_OUTER_ANGLE, parameters );
	if ( string == "REFERENCE_DISTANCE" ) return this->source( AL_REFERENCE_DISTANCE, parameters );
	if ( string == "SEC_OFFSET" ) return this->source( AL_SEC_OFFSET, parameters );
	// alSource3f
	// alSourcefv
	if ( string == "POSITION" ) return this->source( AL_POSITION, parameters );
	if ( string == "VELOCITY" ) return this->source( AL_VELOCITY, parameters );
	if ( string == "DIRECTION" ) return this->source( AL_DIRECTION, parameters );
}
void UF_API_CALL ext::al::Source::source( ALenum name, std::vector<ALint> parameters ) {
	if ( !this->m_index ) this->generate();
	switch ( parameters.size() ) {
		case 1: alSourcei( this->m_index, name, parameters[0] ); break;
		case 3: alSource3i( this->m_index, name, parameters[0], parameters[1], parameters[2] ); break;
		default: alSourceiv( this->m_index, name, &parameters[0] ); break;
	}
	ext::oal.checkError(__FUNCTION__, __LINE__);
}
void UF_API_CALL ext::al::Source::source( const std::string string, std::vector<ALint> parameters ) {
	// alSourcei
	if ( string == "SOURCE_RELATIVE" ) return this->source( AL_SOURCE_RELATIVE, parameters );
	if ( string == "CONE_INNER_ANGLE" ) return this->source( AL_CONE_INNER_ANGLE, parameters );
	if ( string == "CONE_OUTER_ANGLE" ) return this->source( AL_CONE_OUTER_ANGLE, parameters );
	if ( string == "LOOPING" ) return this->source( AL_LOOPING, parameters );
	if ( string == "BUFFER" ) return this->source( AL_BUFFER, parameters );
	if ( string == "SOURCE_STATE" ) return this->source( AL_SOURCE_STATE, parameters );	
	// alSource3i
	// alSourceiv
	if ( string == "POSITION" ) return this->source( AL_POSITION, parameters );
	if ( string == "VELOCITY" ) return this->source( AL_VELOCITY, parameters );
	if ( string == "DIRECTION" ) return this->source( AL_DIRECTION, parameters );
}
void UF_API_CALL ext::al::Source::play() {
	alSourcePlay(this->m_index); ext::oal.checkError(__FUNCTION__, __LINE__, std::to_string(this->m_index));
}
void UF_API_CALL ext::al::Source::stop() {
	alSourceStop(this->m_index); ext::oal.checkError(__FUNCTION__, __LINE__, std::to_string(this->m_index));
}
bool UF_API_CALL ext::al::Source::playing() {
	ALCenum state; alGetSourcei(this->m_index, AL_SOURCE_STATE, &state);
	return state == AL_PLAYING;
}

////////
UF_API_CALL ext::al::Buffer::Buffer() : m_index(0) {
}
ext::al::Buffer::~Buffer() {
	this->destroy();
}

void UF_API_CALL ext::al::Buffer::generate() {
	if ( this->m_index ) this->destroy();
	alGenBuffers(1, &this->m_index); ext::oal.checkError(__FUNCTION__, __LINE__);
}
void UF_API_CALL ext::al::Buffer::destroy() {
	if ( this->m_index && alIsBuffer(this->m_index) ) {
		alDeleteBuffers(1, &this->m_index); ext::oal.checkError(__FUNCTION__, __LINE__);
	}
	this->m_index = 0;
}
ALuint& UF_API_CALL ext::al::Buffer::getIndex() { return this->m_index; }
ALuint UF_API_CALL ext::al::Buffer::getIndex() const { return this->m_index; }

void UF_API_CALL ext::al::Buffer::buffer(ALenum format, const ALvoid* data, ALsizei size, ALsizei frequency) {
	if ( !this->m_index ) this->generate();
	alBufferData(this->m_index, format, data, size, frequency); ext::oal.checkError(__FUNCTION__, __LINE__, std::to_string(format) + " @ " + std::to_string(size) + " @ " + std::to_string(frequency));
}


#endif