#pragma once

#include <uf/utils/mesh/mesh.h>

namespace uf {
	namespace graph {
		enum LoadMode {
			ATLAS 					= 0x1 << 1,
			INVERT 					= 0x1 << 2,
			TRANSFORM 				= 0x1 << 3,
			SKINNED 				= 0x1 << 4,
		};
		typedef uint16_t load_mode_t;
		typedef uint16_t id_t;

		namespace mesh {
			struct Base {
				/*alignas(16)*/ pod::Vector3f position;
				/*alignas(8)*/ pod::Vector2f uv;
				/*alignas(8)*/ pod::Vector2f st;
				/*alignas(4)*/ pod::Vector<id_t, 2> id;

				static UF_API uf::stl::vector<uf::renderer::VertexDescriptor> descriptor;
			};
			struct ID {
				/*alignas(16)*/ pod::Vector3f position;
				/*alignas(8)*/ pod::Vector2f uv;
				/*alignas(8)*/ pod::Vector2f st;
				/*alignas(16)*/ pod::Vector3f normal;
				/*alignas(16)*/ pod::Vector3f tangent;
				/*alignas(4)*/ pod::Vector<id_t, 2> id;

				static UF_API uf::stl::vector<uf::renderer::VertexDescriptor> descriptor;
			};
			struct Skinned {
				/*alignas(16)*/ pod::Vector3f position;
				/*alignas(8)*/ pod::Vector2f uv;
				/*alignas(8)*/ pod::Vector2f st;
				/*alignas(16)*/ pod::Vector3f normal;
				/*alignas(16)*/ pod::Vector3f tangent;
				/*alignas(4)*/ pod::Vector<id_t, 2> id;
				/*alignas(8)*/ pod::Vector<id_t, 4> joints;
				/*alignas(16)*/ pod::Vector4f weights;

				static UF_API uf::stl::vector<uf::renderer::VertexDescriptor> descriptor;
			};
		}
		typedef uf::Mesh<uf::graph::mesh::Base> base_mesh_t;
		typedef uf::Mesh<uf::graph::mesh::ID> id_mesh_t;
		typedef uf::Mesh<uf::graph::mesh::Skinned> skinned_mesh_t;
	}
}