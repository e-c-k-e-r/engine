#pragma once

#include "mesh.h"
#include "graph.h"
#include <uf/engine/object/object.h>

namespace ext {
	namespace gltf {		
		pod::Graph UF_API load( const std::string&, ext::gltf::load_mode_t, const uf::Serializer& );
	}
}