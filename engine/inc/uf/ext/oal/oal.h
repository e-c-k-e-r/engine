#pragma once

#include <uf/config.h>
#if defined(UF_USE_OPENAL)

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alut.h>

#include <string>
#include <vector>

#include <uf/utils/math/vector.h>

namespace ext {
	class UF_API AL {
	protected:
		bool m_initialized;
		ALCdevice* m_device;
		ALCcontext* m_context;
	public:
		AL();
		~AL();

		bool initialize();
		bool terminate();

		void listener( ALenum, std::vector<ALfloat> );
		void listener( const std::string, std::vector<ALfloat> );

		void checkError( const std::string& = "", int = 0, const std::string& = "" ) const;
		std::string getError() const;
	};
	namespace al {
		class UF_API Source {
		protected:
			ALuint m_index;
		public:
			Source();
			~Source();

			void generate();
			void destroy();

			ALuint& getIndex();
			ALuint getIndex() const;
			
			void source( ALenum, std::vector<ALfloat> );
			void source( const std::string, std::vector<ALfloat> );
			
			void source( ALenum, std::vector<ALint> );
			void source( const std::string, std::vector<ALint> );

			void play();
			void stop();
			bool playing();
		};
		class UF_API Buffer {
		protected:
			ALuint m_index;
		public:
			Buffer();
			~Buffer();

			ALuint& getIndex();
			ALuint getIndex() const;

			void buffer(ALenum, const ALvoid*, ALsizei, ALsizei);

			void generate();
			void destroy();
		};
	}

	extern UF_API ext::AL oal;
}

#endif