#pragma once

#include <uf/config.h>

#if UF_USE_OPENAL
#include <uf/ext/oal/oal.h>
#else
	namespace ext {
		namespace al {
			typedef size_t Source;
			typedef size_t Buffer;
		}
	}
	typedef float ALfloat;
#endif

#include <vector>
#include <unordered_map>

#include <uf/utils/math/vector.h>
#include <uf/utils/math/quaternion.h>
#include <uf/utils/serialize/serializer.h>

#include "metadata.h"

namespace uf {
	namespace audio {
		extern UF_API bool muted;
		extern UF_API bool streamsByDefault;
		extern UF_API uint8_t buffers;
		extern UF_API size_t bufferSize;
	}
	class UF_API Audio {
	public:
		typedef uf::audio::Metadata Metadata;
	protected:
		Metadata* m_metadata = NULL;
	public:
		bool initialized() const;
		bool playing() const;

		void open( const std::string& );
		void open( const std::string&, bool );
		void load( const std::string& );
		void stream( const std::string& );
		void update();
		void destroy();

		void play();
		void stop();
		void loop( bool = true );
		bool loops() const;

		float getTime() const;
		void setTime( float );

		void setPosition( const pod::Vector3f& );
		void setOrientation( const pod::Quaternion<>& );

		float getPitch() const;
		void setPitch( float );

		float getGain() const;
		void setGain( float );

		float getRolloffFactor() const;
		void setRolloffFactor( float );

		float getMaxDistance() const;
		void setMaxDistance( float );

		float getVolume() const;
		void setVolume( float );

		const std::string& getFilename() const;
		float getDuration() const;
	};
	namespace audio {
		extern uf::Audio null;
	}
}

#include "emitter.h"