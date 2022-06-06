namespace uf {
	namespace graph {
		namespace mesh {
			struct Base {
				pod::Vector3f position{};
				pod::Vector2f uv{};
				pod::ColorRgba color{ (uint8_t) ~0, (uint8_t) ~0, (uint8_t) ~0, (uint8_t) ~0 };
				pod::Vector2f st{};
				pod::Vector3f normal{};
				pod::Vector<uint16_t, 2> id{};

				static UF_API uf::stl::vector<uf::renderer::AttributeDescriptor> descriptor;
			};
			struct Skinned {
				pod::Vector3f position{};
				pod::Vector2f uv{};
				pod::ColorRgba color{ (uint8_t) ~0, (uint8_t) ~0, (uint8_t) ~0, (uint8_t) ~0 };
				pod::Vector2f st{};
				pod::Vector3f normal{};
				pod::Vector3f tangent{};
				pod::Vector<uint16_t, 4> joints{};
				pod::Vector4f weights{};
				pod::Vector<uint16_t, 2> id{};

				static UF_API uf::stl::vector<uf::renderer::AttributeDescriptor> descriptor;
			};
		}
	}
}