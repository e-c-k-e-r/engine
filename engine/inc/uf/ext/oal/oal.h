#pragma once

#include <uf/config.h>
#if UF_USE_OPENAL

#if UF_ENV_DREAMCAST && UF_USE_OPENAL_ALDC
	#include <ALdc/al.h>
	#include <ALdc/alc.h>
	#include <ALdc/alut.h>
#else
	#include <AL/al.h>
	#include <AL/alc.h>
	#include <AL/alut.h>
#endif
#include <uf/utils/memory/string.h>
#include <uf/utils/memory/vector.h>

#define AL_CHECK_RESULT(f) {\
	(f);\
	ALCenum error = alGetError();\
	if ( error != AL_NO_ERROR ) UF_MSG_ERROR("AL error: " << ext::al::getError(error) << ": " << #f);\
}

#define AL_CHECK_RESULT_ENUM( fun, id, e, ... ) {\
	(fun(id, e, __VA_ARGS__));\
	ALCenum error = alGetError();\
	if ( error != AL_NO_ERROR ) UF_MSG_ERROR("AL error: " << ext::al::getError(error) << ": " #fun "(" << id << ", " << e << ")");\
}

//	uf::stl::string errorString = alutGetErrorString(alutGetError());
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

		void UF_API listener( const uf::stl::string& name, ALfloat x );
		void UF_API listener( const uf::stl::string& name, ALfloat x, ALfloat y, ALfloat z );
		void UF_API listener( const uf::stl::string& name, const ALfloat* f );
	*/

		uf::stl::string UF_API getError( ALenum = 0 );

		uf::audio::Metadata* UF_API create( const uf::stl::string&, bool, uint8_t );
		uf::audio::Metadata* UF_API open( const uf::stl::string& );
		uf::audio::Metadata* UF_API open( const uf::stl::string&, bool );
		uf::audio::Metadata* UF_API load( const uf::stl::string& );
		uf::audio::Metadata* UF_API stream( const uf::stl::string& );
		void UF_API update( uf::audio::Metadata& );

		void UF_API close( uf::audio::Metadata* );
		void UF_API close( uf::audio::Metadata& );
	}
}
#endif