#pragma once

#include <uf/engine/graph/graph.h>

#if UF_USE_XATLAS
namespace ext {
	namespace xatlas {
		size_t UF_API unwrap( pod::Graph&, bool = false );
	}
}
#endif