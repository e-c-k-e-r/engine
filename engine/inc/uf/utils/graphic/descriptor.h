#pragma once 	

#if UF_USE_VULKAN
#include <uf/ext/vulkan/enums.h>
#define RENDERER vulkan
#elif UF_USE_OPENGL
#include <uf/ext/opengl/enums.h>
#define RENDERER opengl
#endif

namespace ext {
	namespace RENDERER {
		struct /*UF_API*/ VertexDescriptor {
			ext::RENDERER::enums::Format::type_t format = 0; // VK_FORMAT_R32G32B32_SFLOAT
			size_t offset = 0; // offsetof(Vertex, position)
			std::string name = "";
		};
	}
}

namespace uf {
	struct /*UF_API*/ BaseGeometry {
	public:
		struct {
			size_t vertex = 0;
			size_t indices = 0;
		} sizes;
		std::vector<ext::RENDERER::VertexDescriptor> attributes;
	};
}

namespace ext {
	namespace RENDERER {
		struct UF_API GraphicDescriptor {
			std::string renderMode = "";
			uint32_t renderTarget = 0;
			uint32_t subpass = 0;

			uf::BaseGeometry geometry;
			size_t indices = 0;

			struct {
				size_t vertex = 0;
				size_t index = 0;
			} offsets;

			ext::RENDERER::enums::PrimitiveTopology::type_t topology = ext::RENDERER::enums::PrimitiveTopology::TRIANGLE_LIST;
			ext::RENDERER::enums::PolygonMode::type_t fill = ext::RENDERER::enums::PolygonMode::FILL;
			ext::RENDERER::enums::CullMode::type_t cullMode = ext::RENDERER::enums::CullMode::BACK;
			ext::RENDERER::enums::Face::type_t frontFace = ext::RENDERER::enums::Face::CW;
			float lineWidth = 1.0f;

			struct {
				bool test = true;
				bool write = true;
				ext::RENDERER::enums::Compare::type_t operation = ext::RENDERER::enums::Compare::GREATER_OR_EQUAL;
				struct {
					bool enable = false;
					float constant = 0;
					float slope = 0;
					float clamp = 0;
				} bias;
			} depth;

			std::string hash() const;
			void parse( ext::json::Value& );
			bool operator==( const GraphicDescriptor& right ) const { return this->hash() == right.hash(); }
		};
	}
}

#undef UF_RENDERER
#define UF_VERTEX_DESCRIPTION( TYPE, FORMAT, ATTRIBUTE ) {\
		uf::renderer::enums::Format::FORMAT,\
		offsetof(TYPE, ATTRIBUTE),\
		#ATTRIBUTE\
	},

#define UF_VERTEX_DESCRIPTOR( TYPE, ... )\
	std::vector<uf::renderer::VertexDescriptor> TYPE::descriptor = { __VA_ARGS__ };
	