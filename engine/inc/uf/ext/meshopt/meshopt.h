#pragma once

#include <uf/config.h>
#include <uf/utils/graphic/descriptor.h>

namespace ext {
	namespace meshopt {
		void UF_API optimize( uf::BaseGeometry&, size_t = SIZE_MAX );
	}
}