#pragma once 	

#include <uf/ext/vulkan/vulkan.h>
#include <uf/ext/vulkan/device.h>
#include <uf/ext/vulkan/swapchain.h>
#include <uf/ext/vulkan/initializers.h>
#include <uf/ext/vulkan/texture.h>
#include <uf/utils/graphic/descriptor.h>

namespace ext {
	namespace vulkan {
		struct /*UF_API*/ VertexDescriptor {
			VkFormat format; // VK_FORMAT_R32G32B32_SFLOAT
			std::size_t offset; // offsetof(Vertex, position)
		};
	}
}

namespace uf {
	struct /*UF_API*/ BaseGeometry {
	public:
		struct {
			size_t vertex;
			size_t indices;
		} sizes;
		std::vector<ext::vulkan::VertexDescriptor> attributes;
	};
}

namespace ext {
	namespace vulkan {
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

			VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			VkPolygonMode fill = VK_POLYGON_MODE_FILL;
			VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
			float lineWidth = 1.0f;
			VkFrontFace frontFace = VK_FRONT_FACE_CLOCKWISE;

			struct {
				VkBool32 test = true;
				VkBool32 write = true;
				VkCompareOp operation = VK_COMPARE_OP_GREATER_OR_EQUAL;
				struct {
					VkBool32 enable = false;
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