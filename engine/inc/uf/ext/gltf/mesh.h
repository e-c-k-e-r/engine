#pragma once

#include <uf/utils/graphic/mesh.h>

namespace ext {
	namespace gltf {
		enum LoadMode {
			RENDER 					= 0x1 << 0,
			COLLISION 				= 0x1 << 1,
			SEPARATE 				= 0x1 << 2,
			NORMALS 				= 0x1 << 3,
			LOAD 					= 0x1 << 4,
			ATLAS 					= 0x1 << 5,
			SKINNED 				= 0x1 << 6,
			INVERT 					= 0x1 << 7,
			TRANSFORM 				= 0x1 << 8,
		};
		typedef uint16_t load_mode_t;

		namespace mesh {
			struct ID {
				/*alignas(16)*/ pod::Vector3f position;
				/*alignas(8)*/ pod::Vector2f uv;
				/*alignas(16)*/ pod::Vector3f normal;
				/*alignas(16)*/ pod::Vector3f tangent;
				/*alignas(8)*/ pod::Vector2ui id;

				static UF_API std::vector<uf::renderer::VertexDescriptor> descriptor;
			};
			struct Skinned {
				/*alignas(16)*/ pod::Vector3f position;
				/*alignas(8)*/ pod::Vector2f uv;
				/*alignas(16)*/ pod::Vector3f normal;
				/*alignas(16)*/ pod::Vector3f tangent;
				/*alignas(8)*/ pod::Vector2ui id;
				/*alignas(16)*/ pod::Vector4ui joints;
				/*alignas(16)*/ pod::Vector4f weights;

				static UF_API std::vector<uf::renderer::VertexDescriptor> descriptor;
			};
		}
		typedef uf::BaseMesh<ext::gltf::mesh::Skinned> mesh_t;
		typedef uf::BaseMesh<ext::gltf::mesh::Skinned> skinned_mesh_t;
	}
}