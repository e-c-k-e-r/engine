#pragma once

#include <uf/engine/object/object.h>

namespace ext {
	namespace gltf {
		enum LoadMode {
			GENERATE_NORMALS 		= 0x1 << 0,
			APPLY_TRANSFORMS 		= 0x1 << 1,
			SEPARATE_MESHES 		= 0x1 << 2,
			RENDER 					= 0x1 << 3,
			COLLISION 				= 0x1 << 4,
			AABB 					= 0x1 << 5,
		};
		bool UF_API load( uf::Object&, const std::string&, uint8_t = LoadMode::GENERATE_NORMALS | LoadMode::RENDER );
	}
}