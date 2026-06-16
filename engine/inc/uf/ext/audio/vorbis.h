#pragma once

#include <uf/config.h>
#if UF_USE_VORBIS

#if UF_USE_TREMOR
	#include <tremor/ivorbisfile.h>
#else
	#include <vorbis/vorbisfile.h>
#endif

#include <uf/utils/audio/audio.h>

namespace ext {
	namespace vorbis {
		void UF_API load( pod::AudioClip& );
		void UF_API open( pod::AudioSource& );
		void UF_API update( pod::AudioSource& );
		void UF_API close( pod::AudioClip& );
		void UF_API close( pod::AudioSource& );
	}
}
#endif