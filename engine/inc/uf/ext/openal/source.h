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

			void get( ALenum name, ALfloat& x ) const;
			void get( ALenum name, ALfloat& x, ALfloat& y, ALfloat& z ) const;
			void get( ALenum name, ALfloat* f ) const;

			void get( ALenum name, ALint& x ) const;
			void get( ALenum name, ALint& x, ALint& y, ALint& z ) const;
			void get( ALenum name, ALint* f ) const;
			
			void set( ALenum name, ALfloat x );
			void set( ALenum name, ALfloat x, ALfloat y, ALfloat z );
			void set( ALenum name, const ALfloat* f );

			void set( ALenum name, ALint x );
			void set( ALenum name, ALint x, ALint y, ALint z );
			void set( ALenum name, const ALint* f );

			void get( const uf::stl::string& name, ALfloat& x ) const;
			void get( const uf::stl::string& name, ALfloat& x, ALfloat& y, ALfloat& z ) const;
			void get( const uf::stl::string& name, ALfloat* f ) const;

			void get( const uf::stl::string& name, ALint& x ) const;
			void get( const uf::stl::string& name, ALint& x, ALint& y, ALint& z ) const;
			void get( const uf::stl::string& name, ALint* f ) const;

			void set( const uf::stl::string& name, ALfloat x );
			void set( const uf::stl::string& name, ALfloat x, ALfloat y, ALfloat z );
			void set( const uf::stl::string& name, const ALfloat* f );

			void set( const uf::stl::string& name, ALint x );
			void set( const uf::stl::string& name, ALint x, ALint y, ALint z );
			void set( const uf::stl::string& name, const ALint* f );

			void play();
			void stop();
			bool playing() const;
		};
	}
}
#endif