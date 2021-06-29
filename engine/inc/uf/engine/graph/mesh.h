#pragma once

#include <uf/utils/graphic/mesh.h>

namespace uf {
	namespace graph {
		enum LoadMode {
			ATLAS 					= 0x1 << 1,
			INVERT 					= 0x1 << 2,
			TRANSFORM 				= 0x1 << 3,
		};
		typedef uint16_t load_mode_t;

		namespace mesh {
			struct ID {
				/*alignas(16)*/ pod::Vector3f position;
				/*alignas(8)*/ pod::Vector2f uv;
				/*alignas(8)*/ pod::Vector2f st;
				/*alignas(16)*/ pod::Vector3f normal;
				/*alignas(16)*/ pod::Vector3f tangent;
				/*alignas(8)*/ pod::Vector2ui id;

				static UF_API uf::stl::vector<uf::renderer::VertexDescriptor> descriptor;
			};
			struct Skinned {
				/*alignas(16)*/ pod::Vector3f position;
				/*alignas(8)*/ pod::Vector2f uv;
				/*alignas(8)*/ pod::Vector2f st;
				/*alignas(16)*/ pod::Vector3f normal;
				/*alignas(16)*/ pod::Vector3f tangent;
				/*alignas(8)*/ pod::Vector2ui id;
				/*alignas(16)*/ pod::Vector4ui joints;
				/*alignas(16)*/ pod::Vector4f weights;

				static UF_API uf::stl::vector<uf::renderer::VertexDescriptor> descriptor;
			};
		}
		typedef uf::BaseMesh<uf::graph::mesh::Skinned> mesh_t;
		typedef uf::BaseMesh<uf::graph::mesh::Skinned> skinned_mesh_t;
	}
}