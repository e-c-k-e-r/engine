#pragma once

#include <uf/config.h>
#include <uf/engine/graph/graph.h>

namespace ext {
	namespace valve {
		void UF_API loadBsp( pod::Graph& graph, const uf::stl::string& filename, const uf::Serializer& metadata );
	}
}