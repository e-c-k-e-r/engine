namespace uf {
	namespace graph {
		namespace mesh {
			struct Base {
				pod::Vector3f position{};
				pod::Vector2f uv{};
				pod::ColorRgba color{ (uint8_t) ~0, (uint8_t) ~0, (uint8_t) ~0, (uint8_t) ~0 };
				pod::Vector2f st{};
				pod::Vector3f normal{};
				pod::Vector3f tangent{};
				pod::Vector<uint16_t, 2> id{};

				static UF_API uf::stl::vector<uf::renderer::AttributeDescriptor> descriptor;
				static UF_API Base interpolate( const Base& p1, const Base& p2, float t );
			};
			struct Skinned {
				pod::Vector3f position{};
				pod::Vector2f uv{};
				pod::ColorRgba color{ (uint8_t) ~0, (uint8_t) ~0, (uint8_t) ~0, (uint8_t) ~0 };
				pod::Vector2f st{};
				pod::Vector3f normal{};
				pod::Vector3f tangent{};
				pod::Vector<uint16_t, 2> id{};
				pod::Vector<uint16_t, 4> joints{};
				pod::Vector4f weights{};

				static UF_API uf::stl::vector<uf::renderer::AttributeDescriptor> descriptor;
				static UF_API Skinned interpolate( const Skinned& p1, const Skinned& p2, float t );
			};
		#if UF_USE_FLOAT16
			struct Base_16f {
				pod::Vector3f16 position{};
				pod::Vector2f16 uv{};
				pod::ColorRgba color{ (uint8_t) ~0, (uint8_t) ~0, (uint8_t) ~0, (uint8_t) ~0 };
				pod::Vector2f16 st{};
				pod::Vector3f16 normal{};
				pod::Vector3f16 tangent{};
				pod::Vector<uint16_t, 2> id{};

				static UF_API uf::stl::vector<uf::renderer::AttributeDescriptor> descriptor;
				static UF_API Base_16f interpolate( const Base_16f& p1, const Base_16f& p2, float t );
			};
			struct Skinned_16f {
				pod::Vector3f16 position{};
				pod::Vector2f16 uv{};
				pod::ColorRgba color{ (uint8_t) ~0, (uint8_t) ~0, (uint8_t) ~0, (uint8_t) ~0 };
				pod::Vector2f16 st{};
				pod::Vector3f16 normal{};
				pod::Vector3f16 tangent{};
				pod::Vector<uint16_t, 2> id{};
				pod::Vector<uint16_t, 4> joints{};
				pod::Vector3f16 weights{};

				static UF_API uf::stl::vector<uf::renderer::AttributeDescriptor> descriptor;
				static UF_API Skinned_16f interpolate( const Skinned_16f& p1, const Skinned_16f& p2, float t );
			};
		#endif
			struct Base_u16q {
				pod::Vector<uint16_t, 3> position{};
				pod::Vector<uint16_t, 2> uv{};
				pod::ColorRgba color{ (uint8_t) ~0, (uint8_t) ~0, (uint8_t) ~0, (uint8_t) ~0 };
				pod::Vector<uint16_t, 2> st{};
				pod::Vector<uint16_t, 3> normal{};
				pod::Vector<uint16_t, 3> tangent{};
				pod::Vector<uint16_t, 2> id{};

				static UF_API uf::stl::vector<uf::renderer::AttributeDescriptor> descriptor;
				static UF_API Base_u16q interpolate( const Base_u16q& p1, const Base_u16q& p2, float t );
			};
			struct Skinned_u16q {
				pod::Vector<uint16_t, 3> position{};
				pod::Vector<uint16_t, 2> uv{};
				pod::ColorRgba color{ (uint8_t) ~0, (uint8_t) ~0, (uint8_t) ~0, (uint8_t) ~0 };
				pod::Vector<uint16_t, 2> st{};
				pod::Vector<uint16_t, 3> normal{};
				pod::Vector<uint16_t, 3> tangent{};
				pod::Vector<uint16_t, 2> id{};
				pod::Vector<uint16_t, 4> joints{};
				pod::Vector<uint16_t, 3> weights{};

				static UF_API uf::stl::vector<uf::renderer::AttributeDescriptor> descriptor;
				static UF_API Skinned_u16q interpolate( const Skinned_u16q& p1, const Skinned_u16q& p2, float t );
			};
		}
	}
}