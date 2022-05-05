#pragma once

#include <uf/engine/graph/graph.h>

#if UF_USE_XATLAS
namespace ext {
	namespace xatlas {
		pod::Vector2ui UF_API unwrap( uf::stl::vector<uf::graph::mesh::Skinned>& vertices, uf::stl::vector<uint32_t>& indices );
		pod::Vector2ui UF_API unwrap( pod::Graph& );
	}
}
#endif