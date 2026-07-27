#pragma once

#include <uf/config.h>
#if UF_ENV_DREAMCAST

#include "al.h"
#include <uf/utils/math/vector.h>

namespace ext {
	namespace al {
		class UF_API Listener {
		public:
			static void set( ALenum name, ALfloat x, ALfloat y, ALfloat z );
			static void set( ALenum name, const ALfloat* values );
			static pod::Vector3f getPosition();
			static pod::Vector3f getRight();
		};
	}
}
#endif