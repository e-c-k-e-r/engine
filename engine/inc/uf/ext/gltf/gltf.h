#pragma once

#include "mesh.h"
#include <uf/engine/object/object.h>

namespace ext {
	namespace gltf {		
	//	typedef uf::BaseMesh<ext::gltf::mesh::ID, uint32_t> mesh_t;
		typedef uf::BaseMesh<ext::gltf::mesh::Skinned, uint32_t> mesh_t;
		typedef uf::BaseMesh<ext::gltf::mesh::Skinned, uint32_t> skinned_mesh_t;

		enum LoadMode {
			GENERATE_NORMALS 		= 0x1 << 0,
			APPLY_TRANSFORMS 		= 0x1 << 1,
			SEPARATE_MESHES 		= 0x1 << 2,
			RENDER 					= 0x1 << 3,
			COLLISION 				= 0x1 << 4,
			AABB 					= 0x1 << 5,
			DEFER_INIT 				= 0x1 << 6,
			USE_ATLAS 				= 0x1 << 7,
			SKINNED 				= 0x1 << 8,
		};
		typedef uint16_t load_mode_t;

		bool UF_API load( uf::Object&, const std::string&, ext::gltf::load_mode_t = LoadMode::GENERATE_NORMALS | LoadMode::RENDER );
	}
}