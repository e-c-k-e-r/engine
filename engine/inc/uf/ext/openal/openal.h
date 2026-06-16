#pragma once

#include <uf/config.h>
#if UF_USE_OPENAL

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alut.h>

#include <uf/utils/memory/string.h>
#include <uf/utils/memory/vector.h>

#define AL_CHECK_RESULT(f) {\
	(f);\
	ALCenum error = alGetError();\
	if ( error != AL_NO_ERROR ) UF_MSG_ERROR("AL error: {}: {}", ext::al::getError(error), #f);\
}

#define AL_CHECK_RESULT_ENUM( fun, id, e, ... ) {\
	(fun(id, e, __VA_ARGS__));\
	ALCenum error = alGetError();\
	if ( error != AL_NO_ERROR ) UF_MSG_ERROR("AL error: {}: {} ({}, {})", ext::al::getError(error), #fun, id, e);\
}

//	uf::stl::string errorString = alutGetErrorString(alutGetError());
//	if ( errorString != "No ALUT error found" ) UF_MSG_ERROR("AL error: {}", errorString);

#include "source.h"
#include "buffer.h"
#include "filter.h"
#include <uf/utils/audio/metadata.h>
#include <uf/utils/math/transform.h>

namespace ext {
	namespace al {
		void UF_API initialize();
		void UF_API destroy();

		uf::stl::string UF_API getError( ALenum = 0 );
	}
}
#endif