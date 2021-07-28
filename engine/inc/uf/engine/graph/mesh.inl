namespace uf {
	namespace graph {
		namespace mesh {
			struct Base {
				pod::Vector3f position;
				pod::ColorRgba color;
				pod::Vector2f uv;
				pod::Vector2f st;
				pod::Vector3f normal;

				static UF_API uf::stl::vector<uf::renderer::AttributeDescriptor> descriptor;
			};
			struct Skinned {
				pod::Vector3f position;
				pod::ColorRgba color;
				pod::Vector2f uv;
				pod::Vector2f st;
				pod::Vector3f normal;
				pod::Vector3f tangent;
				pod::Vector<uint16_t, 4> joints;
				pod::Vector4f weights;

				static UF_API uf::stl::vector<uf::renderer::AttributeDescriptor> descriptor;
			};
		}
	}
}