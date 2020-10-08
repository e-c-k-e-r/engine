#pragma once

#include <uf/engine/object/object.h>
#include <uf/utils/graphic/mesh.h>

namespace ext {
	namespace gltf {
		typedef uf::BaseMesh<pod::Vertex_3F2F3F1UI, uint32_t> mesh_t;
		enum LoadMode {
			GENERATE_NORMALS 		= 0x1 << 0,
			APPLY_TRANSFORMS 		= 0x1 << 1,
			SEPARATE_MESHES 		= 0x1 << 2,
			RENDER 					= 0x1 << 3,
			COLLISION 				= 0x1 << 4,
			AABB 					= 0x1 << 5,
			DEFER_INIT 				= 0x1 << 6,
			USE_ATLAS 				= 0x1 << 7,
		};
		bool UF_API load( uf::Object&, const std::string&, uint8_t = LoadMode::GENERATE_NORMALS | LoadMode::RENDER );
	}
}