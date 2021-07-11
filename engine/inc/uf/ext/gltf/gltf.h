#pragma once

#include <uf/engine/graph/graph.h>
#include <uf/engine/object/object.h>

namespace ext {
	namespace gltf {		
		pod::Graph UF_API load( const uf::stl::string&, const uf::Serializer& = {} );
		void UF_API save( const uf::stl::string&, const pod::Graph& );
	}
}