#pragma once

#include <uf/config.h>

#include <uf/utils/audio/audio.h>

namespace ext {
	namespace pcm {
		void UF_API open( uf::Audio::Metadata&, const pod::PCM& );
		void UF_API load( uf::Audio::Metadata& );
		void UF_API stream( uf::Audio::Metadata& );
		void UF_API update( uf::Audio::Metadata& );
		void UF_API close( uf::Audio::Metadata& );
	}
}