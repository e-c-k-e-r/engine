#pragma once

#include <uf/config.h>
#include <uf/engine/graph/graph.h>

namespace ext {
	namespace ttlg {
		bool UF_API loadBin( pod::Graph& graph, const uf::stl::string& filename );
	}
}