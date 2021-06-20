#pragma once

#include <uf/config.h>
#if UF_USE_OPENAL

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alut.h>

#include <string>
#include <vector>

#define AL_CHECK_RESULT(f) {\
	(f);\
	ALCenum error = alGetError();\
	if ( error != AL_NO_ERROR ) UF_MSG_ERROR("AL error: " << ext::al::getError(error) << ": " << #f);\
}

//	std::string errorString = alutGetErrorString(alutGetError());
//	if ( errorString != "No ALUT error found" ) UF_MSG_ERROR("AL error: " << errorString);

#include "source.h"
#include "buffer.h"
#include <uf/utils/audio/metadata.h>
#include <uf/utils/math/transform.h>

namespace ext {
	namespace al {
		void UF_API initialize();
		void UF_API destroy();

		void UF_API listener( const pod::Transform<>& );
	/*	
		void UF_API listener( ALenum name, ALfloat x );
		void UF_API listener( ALenum name, ALfloat x, ALfloat y, ALfloat z );
		void UF_API listener( ALenum name, const ALfloat* f );

		void UF_API listener( const std::string& name, ALfloat x );
		void UF_API listener( const std::string& name, ALfloat x, ALfloat y, ALfloat z );
		void UF_API listener( const std::string& name, const ALfloat* f );
	*/

		std::string UF_API getError( ALenum = 0 );

		uf::audio::Metadata* UF_API create( const std::string&, bool, uint8_t );
		uf::audio::Metadata* UF_API open( const std::string& );
		uf::audio::Metadata* UF_API open( const std::string&, bool );
		uf::audio::Metadata* UF_API load( const std::string& );
		uf::audio::Metadata* UF_API stream( const std::string& );
		void UF_API update( uf::audio::Metadata& );

		void UF_API close( uf::audio::Metadata* );
		void UF_API close( uf::audio::Metadata& );
	}
}
#endif