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
		void UF_API open( uf::Audio::Metadata& );
		void UF_API load( uf::Audio::Metadata& );
		void UF_API stream( uf::Audio::Metadata& );
		void UF_API update( uf::Audio::Metadata& );
		void UF_API close( uf::Audio::Metadata& );
	}
}
#endif