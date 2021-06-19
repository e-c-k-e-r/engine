#pragma once

#include <uf/config.h>
#if UF_USE_OPENAL

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alut.h>

namespace ext {
	namespace al {
		class UF_API Source {
		protected:
			ALuint m_index = 0;
		public:
		//	Source();
		//	~Source();

			void initialize();
			void destroy();

			ALuint& getIndex();
			ALuint getIndex() const;

			void get( ALenum name, ALfloat& x );
			void get( ALenum name, ALfloat& x, ALfloat& y, ALfloat& z );
			void get( ALenum name, ALfloat* f );

			void get( ALenum name, ALint& x );
			void get( ALenum name, ALint& x, ALint& y, ALint& z );
			void get( ALenum name, ALint* f );
			
			void set( ALenum name, ALfloat x );
			void set( ALenum name, ALfloat x, ALfloat y, ALfloat z );
			void set( ALenum name, const ALfloat* f );

			void set( ALenum name, ALint x );
			void set( ALenum name, ALint x, ALint y, ALint z );
			void set( ALenum name, const ALint* f );

			void get( const std::string& name, ALfloat& x );
			void get( const std::string& name, ALfloat& x, ALfloat& y, ALfloat& z );
			void get( const std::string& name, ALfloat* f );

			void get( const std::string& name, ALint& x );
			void get( const std::string& name, ALint& x, ALint& y, ALint& z );
			void get( const std::string& name, ALint* f );

			void set( const std::string& name, ALfloat x );
			void set( const std::string& name, ALfloat x, ALfloat y, ALfloat z );
			void set( const std::string& name, const ALfloat* f );

			void set( const std::string& name, ALint x );
			void set( const std::string& name, ALint x, ALint y, ALint z );
			void set( const std::string& name, const ALint* f );

			void play();
			void stop();
			bool playing();
		};
	}
}
#endif