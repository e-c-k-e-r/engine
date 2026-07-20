#pragma once

#include <uf/config.h>
#include <uf/engine/graph/graph.h>

namespace ext {
	namespace lgs {
		void UF_API loadMis( pod::Graph& graph, const uf::stl::string& filename, const uf::Serializer& metadata );
	}
}