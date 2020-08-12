#pragma once

#include <uf/ext/vulkan/device.h>
#include <uf/ext/vulkan/swapchain.h>
#include <uf/ext/vulkan/initializers.h>
#include <uf/ext/vulkan/graphic.h>
#include <uf/ext/vulkan/texture.h>
#include <uf/ext/vulkan/rendertarget.h>

#include <uf/utils/math/matrix.h>

namespace ext {
	namespace vulkan {
		struct UF_API RenderTargetGraphic : public Graphic {
			struct Vertex {
				alignas(8) pod::Vector2f position;
				alignas(8) pod::Vector2f uv;
			};

			struct {
				struct {
					alignas(16) pod::Matrix4f models[2];
				} matrices;
				struct {
					alignas(8) pod::Vector2f position = { 0.5f, 0.5f };
					alignas(8) pod::Vector2f radius = { 0.1f, 0.1f };
					alignas(16) pod::Vector4f color = { 1, 1, 1, 1 };
				} cursor;
				alignas(4) float alpha;
			} uniforms;
			struct {
				uint32_t pass = 0;
			} pushConstants;

			uint32_t indices = 0;
			VkSampler sampler;
			ext::vulkan::RenderTarget* renderTarget;
			
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