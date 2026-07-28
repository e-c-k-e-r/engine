#pragma once

#if !UF_USE_ADP
#define UF_USE_adp 0
#else
#define UF_USE_adp 1
#include <uf/config.h>

#include <uf/utils/audio/audio.h>

namespace ext {
	namespace adp {
		void UF_API load( pod::AudioClip& );
		void UF_API open( pod::AudioSource& );
		void UF_API update( pod::AudioSource& );
		void UF_API close( pod::AudioClip& );
		void UF_API close( pod::AudioSource& );

		bool UF_API decode( const uf::stl::string& filename, pod::PCM& pcm );
		uf::stl::vector<uint8_t> UF_API encode( const pod::PCM& pcm );
	}
}
#endif