#pragma once

#include <uf/config.h>
#include <uf/utils/mesh/mesh.h>
#include <uf/ext/meshopt/meshopt.h>

namespace ext {
	namespace meshopt {
		void UF_API optimize( uf::Mesh&, size_t = SIZE_MAX );
	}
}