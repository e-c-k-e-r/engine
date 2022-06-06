#pragma once

#include <uf/config.h>
#include <uf/utils/mesh/mesh.h>
#include <uf/ext/meshopt/meshopt.h>

namespace ext {
	namespace meshopt {
		bool UF_API optimize( uf::Mesh&, float simplify = 1.0f, size_t = SIZE_MAX );
	}
}