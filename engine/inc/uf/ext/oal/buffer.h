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

namespace ext {
	namespace al {
		class UF_API Buffer {
		protected:
			uf::stl::vector<ALuint> m_indices;
		public:
		//	Buffer( size_t = 1 );
		//	~Buffer();
			
			bool initialized() const;

			ALuint& getIndex( size_t = 0 );
			ALuint getIndex( size_t = 0 ) const;

			void buffer(ALenum, const ALvoid*, ALsizei, ALsizei, size_t = 0);

			void initialize( size_t = 1 );
			void destroy();
		};
	}
}
#endif