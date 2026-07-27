#pragma once

#include <uf/config.h>
#if UF_ENV_DREAMCAST

#include "al.h"

namespace ext {
	namespace al {
		class Filter {
		protected:
			ALuint m_index = 0;
		public:
			void initialize();
			void destroy();
			ALuint getIndex() const;
			void set( ALenum name, ALfloat x );
			void set( ALenum name, ALint x );
		};
		class Effect {
			ALuint m_index = 0;
		public:
			void initialize();
			void destroy();
			ALuint getIndex() const;

			void set( ALenum name, ALfloat x );
			void set( ALenum name, ALint x );
		};

		class EffectSlot {
			ALuint m_index = 0;
		public:
			void initialize();
			void destroy();
			ALuint getIndex() const;

			void set( ALenum name, ALfloat x );
			void set( ALenum name, ALint x );
		};
	}
}
#endif