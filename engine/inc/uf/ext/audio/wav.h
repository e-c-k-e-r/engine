#pragma once

#include <uf/config.h>
#if UF_USE_WAV

#include <uf/utils/audio/audio.h>

namespace ext {
	namespace wav {
		void UF_API load( pod::AudioClip& );
		void UF_API open( pod::AudioSource& );
		void UF_API update( pod::AudioSource& );
		void UF_API close( pod::AudioClip& );
		void UF_API close( pod::AudioSource& );
	}
}
#endif