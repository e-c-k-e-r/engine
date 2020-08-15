#pragma once

#include <uf/ext/vulkan/device.h>
#include <uf/ext/vulkan/swapchain.h>
#include <uf/ext/vulkan/initializers.h>
#include <uf/ext/vulkan/graphic.old.h>
#include <uf/ext/vulkan/texture.h>
#include <uf/utils/userdata/userdata.h>

#include <uf/utils/math/matrix.h>

namespace ext {
	namespace vulkan {
		struct UF_API GraphicDescriptor {
			std::size_t size; // sizeof(Vertex)
			std::vector<VertexDescriptor> attributes;
			uf::Userdata uniforms;

			bool defaultedPushConstants = false;
			uf::Userdata pushConstants;
			struct {
				VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
				VkPolygonMode fill = VK_POLYGON_MODE_FILL;
				float lineWidth = 1.0f;
				VkFrontFace frontFace = ext::vulkan::GraphicOld::DEFAULT_WINDING_ORDER;
			} rasterMode;

		};
		struct UF_API BaseGraphic : public GraphicOld {
			uint32_t indices = 0;
			ext::vulkan::GraphicDescriptor description;
			ext::vulkan::Texture2D texture;
			//
			void describeVertex( const std::vector<VertexDescriptor>& );
			template<typename U>
			void bindUniform() {
				description.uniforms.create<U>();
			}
			template<typename U>
			void bindPushConstants() {
				description.pushConstants.create<U>();
			}
			template<typename U>
			U& uniforms() {
				return description.uniforms.get<U>();
			}
			template<typename U>
			U& pushConstants() {
				return description.pushConstants.get<U>();
			}

			virtual void createCommandBuffer( VkCommandBuffer );
			virtual bool autoAssignable() const;
			virtual std::string name() const;
			// RAII
			virtual void initialize( const std::string& = "" );
			virtual void initialize( Device& device, RenderMode& renderMode );
			virtual void destroy();
		};
	}
}