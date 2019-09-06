#pragma once

#include <uf/ext/vulkan/device.h>
#include <uf/ext/vulkan/swapchain.h>
#include <uf/ext/vulkan/initializers.h>
#include <uf/ext/vulkan/graphic.h>
#include <uf/ext/vulkan/texture.h>

#include <uf/utils/math/matrix.h>

namespace ext {
	namespace vulkan {
		struct UF_API MeshGraphic : public Graphic {
			struct Vertex {
				alignas(16) pod::Vector3f position;
				alignas(16) pod::Vector2f uv;
				alignas(16) pod::Vector3f normal;
			};

			struct {
				struct {
					alignas(16) pod::Matrix4f model;
					alignas(16) pod::Matrix4f view;
					alignas(16) pod::Matrix4f projection;
				} matrices;
			} uniforms;

			uint32_t indices = 0;
			ext::vulkan::Texture2D texture;
			
			virtual void createCommandBuffer( VkCommandBuffer, uint32_t = 0 );
			virtual bool autoAssignable() const;
			virtual std::string name() const;
			// RAII
			virtual void initialize( Device& device, Swapchain& swapchain );
			virtual void destroy();
		};
	}
}