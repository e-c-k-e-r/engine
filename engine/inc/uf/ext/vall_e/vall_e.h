#pragma once

#include <uf/config.h>

#if UF_USE_VALL_E

#include <vall_e.cpp/vall_e.h>
#include <uf/utils/audio/audio.h>

namespace ext {
	namespace vall_e {
		void UF_API initialize( const std::string& model_path = "", const std::string& encodec_path = "" );
		pod::PCM UF_API generate( const std::string& text, const std::string& prom, const std::string& lang = "en" );
		void UF_API terminate();
	}
}

#endif