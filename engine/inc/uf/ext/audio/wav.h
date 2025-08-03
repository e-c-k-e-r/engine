#pragma once

#include <uf/config.h>
#if UF_USE_WAV

#include <uf/utils/audio/audio.h>

namespace ext {
	namespace wav {
		void UF_API open( uf::Audio::Metadata& );
		void UF_API load( uf::Audio::Metadata& );
		void UF_API stream( uf::Audio::Metadata& );
		void UF_API update( uf::Audio::Metadata& );
		void UF_API close( uf::Audio::Metadata& );
	}
}
#endif