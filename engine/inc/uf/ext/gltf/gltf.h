#pragma once

#include <uf/config.h>

#if UF_USE_GLTF

#include <uf/engine/graph/graph.h>
#include <uf/engine/object/object.h>

namespace ext {
	namespace gltf {		
		void UF_API load( pod::Graph&, const uf::stl::string&, const uf::Serializer& = {} );
		inline pod::Graph load( const uf::stl::string& filename, const uf::Serializer& metadata = ext::json::null() ) {
			// do some deprecation warning or something because this actually is bad for doing a copy + dealloc
			pod::Graph graph;
			load( graph, filename, metadata );
			return graph;
		}
		void UF_API save( const uf::stl::string&, const pod::Graph& );
	}
}
#endif