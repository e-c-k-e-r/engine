#pragma once

#include <uf/ext/vulkan/device.h>
#include <uf/ext/vulkan/swapchain.h>
#include <uf/ext/vulkan/initializers.h>
#include <uf/ext/vulkan/graphic.h>
#include <uf/ext/vulkan/texture.h>
#include <uf/utils/userdata/userdata.h>

#include <uf/utils/math/matrix.h>

namespace ext {
	namespace vulkan {
		struct UF_API VertexDescriptor {
			VkFormat format; // VK_FORMAT_R32G32B32_SFLOAT
			std::size_t offset; // offsetof(Vertex, position)
		};
		struct UF_API GraphicDescriptor {
			std::size_t size; // sizeof(Vertex)
			std::vector<VertexDescriptor> attributes;
			uf::Userdata uniforms;
		};
		struct UF_API BaseGraphic : public Graphic {
			uint32_t indices = 0;
			ext::vulkan::GraphicDescriptor description;
			ext::vulkan::Texture2D texture;
			
			//
			void describeVertex( const std::vector<VertexDescriptor>& );
			template<typename U>
			void bindUniform() {
				U tmp;
				description.uniforms.create( tmp );
			}
			template<typename U>
			U& uniforms() {
				return description.uniforms.get<U>();
			}

			virtual void createCommandBuffer( VkCommandBuffer, uint32_t = 0 );
			virtual bool autoAssignable() const;
			virtual std::string name() const;
			// RAII
			virtual void initialize( Device& device, Swapchain& swapchain );
			virtual void destroy();
		};
	}
}