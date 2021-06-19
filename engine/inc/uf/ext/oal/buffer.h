#pragma once

#include <uf/config.h>
#if UF_USE_OPENAL

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alut.h>

namespace ext {
	namespace al {
		class UF_API Buffer {
		protected:
			std::vector<ALuint> m_indices;
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