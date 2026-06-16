#pragma once

#include <uf/config.h>
#if UF_USE_OPENAL

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alut.h>
#define AL_ALEXT_PROTOTYPES 1
#include <AL/efx.h>
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