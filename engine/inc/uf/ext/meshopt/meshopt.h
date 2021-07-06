#pragma once

#include <uf/config.h>
#include <uf/utils/mesh/mesh.h>

namespace ext {
	namespace meshopt {
		void UF_API optimize( pod::Mesh&, size_t = SIZE_MAX );
	}
}