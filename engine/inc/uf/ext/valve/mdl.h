#pragma once

#include <uf/config.h>
#include <uf/engine/graph/graph.h>

namespace ext {
	namespace valve {
		bool UF_API loadMdl( pod::Graph& graph, const uf::stl::string& filename );
	}
}