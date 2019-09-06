#pragma once

#include <uf/ext/vulkan/buffer.h>
#include <uf/ext/vulkan/texture.h>
#include <uf/ext/vulkan/device.h>
#include <uf/ext/vulkan/swapchain.h>
#include <uf/ext/vulkan/graphic.h>
#include <uf/utils/math/rayt.h>
#include <uf/utils/math/vector.h>
#include <uf/utils/math/matrix.h>

namespace ext {
	namespace vulkan {
		struct UF_API ComputeGraphic : public Graphic {
			// alias typedefs
			typedef pod::Primitive Cube;
			typedef pod::Light Light;
			typedef pod::Tree Tree;

			// Uniform buffer
			struct {
				struct {
					alignas(16) pod::Vector3f position = { 0.0f, 0.0f, 0.0f };
					alignas(16) pod::Matrix4f view;
					float aspectRatio;						// Aspect ratio of the viewport
				} camera;
				struct {
					struct {
						uint start = 0;
						uint end = std::numeric_limits<uint>::max();
					} cubes;
					struct {
						uint start = 0;
						uint end = std::numeric_limits<uint>::max();
					} lights;
					uint root = std::numeric_limits<uint>::max();
				} ssbo;
			} uniforms;

			VkQueue queue;								// Separate queue for compute commands (queue family may differ from the one used for graphics)
			VkCommandPool commandPool;					// Use a separate command pool (queue family may differ from the one used for graphics)
			VkCommandBuffer commandBuffer;				// Command buffer storing the dispatch commands and barriers
			VkFence fence;								// Synchronization fence to avoid rewriting compute CB if still in use
			Texture2D renderTarget; 					// 
			Texture2D diffuseTexture; 					// 

			void setStorageBuffers( Device& device, std::vector<Cube>& cubes, std::vector<Light>& lights, std::vector<Tree>& );
			void updateStorageBuffers( Device& device, std::vector<Cube>& cubes, std::vector<Light>& lights, std::vector<Tree>& );
			void updateUniformBuffer();
			virtual void createCommandBuffer( VkCommandBuffer, uint32_t = 0 );
			virtual std::string name() const;
			// RAII
			void initialize( Device& device, Swapchain& swapchain, uint32_t width = 0, uint32_t height = 0 );
			virtual void destroy();
		};
	}
}

namespace ext {
	namespace vulkan {
		struct UF_API RTGraphic : public Graphic {
			ComputeGraphic compute;

			uint32_t width = 0;
			uint32_t height = 0;

			virtual void createCommandBuffer( VkCommandBuffer, uint32_t = 0 );
			virtual void createImageMemoryBarrier( VkCommandBuffer );
			virtual void updateUniformBuffer();
			virtual bool autoAssignable() const;
			virtual std::string name() const;
			// RAII
			virtual void initialize( Device& device, Swapchain& swapchain );
			virtual void render();
			virtual void destroy();
		};
	}
}