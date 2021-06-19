#pragma once

#include <uf/engine/graph/mesh.h>
#include <uf/engine/graph/graph.h>
#include <uf/engine/object/object.h>

namespace ext {
	namespace gltf {		
		pod::Graph UF_API load( const std::string&, uf::graph::load_mode_t = 0, const uf::Serializer& = {} );
		void UF_API save( const std::string&, const pod::Graph& );
	}
}