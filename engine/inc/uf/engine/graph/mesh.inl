namespace uf {
	namespace graph {
		namespace mesh {
			typedef uint16_t id_t;

			struct Base {
				pod::Vector3f position;
				pod::Vector2f uv;
				pod::Vector2f st;

				static UF_API uf::stl::vector<uf::renderer::AttributeDescriptor> descriptor;
			};
			struct ID {
				pod::Vector3f position;
				pod::Vector2f uv;
				pod::Vector2f st;
				pod::Vector3f normal;
				pod::Vector3f tangent;

				static UF_API uf::stl::vector<uf::renderer::AttributeDescriptor> descriptor;
			};
			struct Skinned {
				pod::Vector3f position;
				pod::Vector2f uv;
				pod::Vector2f st;
				pod::Vector3f normal;
				pod::Vector3f tangent;
				pod::Vector<id_t, 4> joints;
				pod::Vector4f weights;

				static UF_API uf::stl::vector<uf::renderer::AttributeDescriptor> descriptor;
			};
		}
	/*
		typedef uf::Mesh<uf::graph::mesh::Base> base_mesh_t;
		typedef uf::Mesh<uf::graph::mesh::ID> id_mesh_t;
		typedef uf::Mesh<uf::graph::mesh::Skinned> skinned_mesh_t;
	*/
	}
}