#pragma once

#if UF_ENV_DREAMCAST && UF_USE_AICA
#include <uf/config.h>

#include <uf/utils/memory/string.h>
#include <uf/utils/memory/vector.h>

#include "al.h"
#include "source.h"
#include "buffer.h"
#include "filter.h"
#include "listener.h"

#include <uf/utils/audio/metadata.h>
#include <uf/utils/math/transform.h>

namespace ext {
	namespace al {
		void UF_API initialize();
		void UF_API destroy();

		uf::stl::string UF_API getError( ALenum = 0 );
	}
}

#endif