#pragma once

#include <uf/config.h>

#if UF_USE_OPENAL
#include <uf/ext/openal/openal.h>
#else
	namespace ext {
		namespace al {
			typedef size_t Source;
			typedef size_t Buffer;
		}
	}
	typedef float ALfloat;
#endif

#include <uf/utils/memory/vector.h>
#include <uf/utils/memory/unordered_map.h>

#include <uf/utils/math/vector.h>
#include <uf/utils/math/quaternion.h>
#include <uf/utils/serialize/serializer.h>

#include "metadata.h"

namespace uf {
	namespace audio {
		extern UF_API bool muted;
		extern UF_API bool asyncUpdate;
		extern UF_API bool streamsByDefault;
		extern UF_API uint8_t buffers;
		extern UF_API size_t bufferSize;

	#if UF_AUDIO_MAPPED_VOLUMES
		extern UF_API uf::stl::unordered_map<uf::stl::string, float> volumes;
	#else
		namespace volumes {
			extern UF_API float bgm;
			extern UF_API float sfx;
			extern UF_API float voice;
		};
	#endif

		void UF_API initialize( pod::AudioClip& clip, uint8_t buffers = uf::audio::buffers );
		void UF_API initialize( pod::AudioSource& source );
		bool UF_API load( pod::AudioClip& clip, const uf::stl::string& filename, bool streamed = uf::audio::streamsByDefault );
		void UF_API destroy( pod::AudioClip& clip );
		void UF_API bind( pod::AudioSource& source, pod::AudioClip* clip );
		void UF_API play( pod::AudioSource& source );
		void UF_API stop( pod::AudioSource& source );
		void UF_API update( pod::AudioSource& source );
		void UF_API update( pod::AudioSource& source, const pod::Vector3f& position, const pod::Quaternion<>& orientation );
		void UF_API destroy( pod::AudioSource& source );
		void UF_API destroy( pod::AudioClip& clip );
		void UF_API listener( const pod::Transform<>& transform );
		void UF_API loop( pod::AudioSource& source, bool state );
		void UF_API position( pod::AudioSource& source, const pod::Vector3f& v );
		void UF_API orientation( pod::AudioSource& source, const pod::Quaternion<>& q );
		float UF_API time( const pod::AudioSource& source );
		void UF_API time( pod::AudioSource& source, float v );
		float UF_API pitch( const pod::AudioSource& source );
		void UF_API pitch( pod::AudioSource& source, float v );
		float UF_API gain( const pod::AudioSource& source );
		void UF_API gain( pod::AudioSource& source, float v );
		float UF_API rolloff( const pod::AudioSource& source );
		void UF_API rolloff( pod::AudioSource& source, float v );
		float UF_API maxDistance( const pod::AudioSource& source );
		void UF_API maxDistance( pod::AudioSource& source, float v );

		float UF_API occlusion( const pod::Vector3f& position );
		void UF_API occlude( pod::AudioSource& source, float factor );
		void UF_API acoustics( const pod::Vector3f&, const pod::Quaternion<>&, float&, float&, int& );
	}
}

#include "emitter.h"