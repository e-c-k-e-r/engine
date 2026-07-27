#pragma once

#include <uf/config.h>
#if UF_USE_OPENAL


#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alut.h>

namespace ext {
	namespace al {
		class UF_API Listener {
		public:
			static void set( ALenum name, ALfloat x, ALfloat y, ALfloat z );
			static void set( ALenum name, const ALfloat* values );
		};
	}
}
#endif