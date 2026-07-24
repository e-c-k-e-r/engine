#pragma once
#if UF_USE_LGS
#include <uf/config.h>
#include <uf/engine/graph/graph.h>

namespace ext {
	namespace lgs {
		bool UF_API loadBin( pod::Graph& graph, const uf::stl::string& filename );
	}
}
#endif