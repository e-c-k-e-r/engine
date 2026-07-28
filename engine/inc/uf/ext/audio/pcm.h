#pragma once
#if !UF_USE_PCM
#define UF_USE_pcm 0
#else
#define UF_USE_pcm 1
#include <uf/config.h>
#include <uf/utils/audio/audio.h>

namespace ext {
	namespace pcm {
		void UF_API load( pod::AudioClip&, const pod::PCM& );
		void UF_API open( pod::AudioSource& );
		void UF_API update( pod::AudioSource& );
		void UF_API close( pod::AudioClip& );
		void UF_API close( pod::AudioSource& );

		uf::stl::vector<int16_t> UF_API convertTo16bit( const uf::stl::vector<float>& );
		uf::stl::vector<int16_t> UF_API convertTo16bit( const float*, size_t );
	}
}
#endif