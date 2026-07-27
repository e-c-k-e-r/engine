#pragma once

#include <uf/config.h>
#if UF_ENV_DREAMCAST

#include "al.h"

namespace ext {
	namespace al {
		class UF_API Buffer {
		protected:
			uf::stl::vector<ALuint> m_indices;
		public:
			bool initialized() const;

			ALuint& getIndex( size_t = 0 );
			ALuint getIndex( size_t = 0 ) const;

			void buffer( ALenum, const ALvoid*, ALsizei, ALsizei, size_t = 0 );
			static void buffer( ALuint index, ALenum, const ALvoid*, ALsizei, ALsizei );

			void set( ALenum, ALint*, size_t = 0 );
			void set( ALenum name, ALint value, size_t i = 0 );
			void get( ALenum name, ALint& value, size_t i = 0 ) const;
			void poll( size_t = 0 );

			void initialize( size_t = 1 );
			void destroy();
		};
	}
}
#endif