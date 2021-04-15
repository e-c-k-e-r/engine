#pragma once

#include <uf/ext/vulkan/rendermode.h>
#include <uf/ext/vulkan/graphic.h>

namespace ext {
	namespace vulkan {
		struct UF_API RenderTargetRenderMode : public ext::vulkan::RenderMode {
			ext::vulkan::Graphic blitter;
			
			typedef std::function<void(VkCommandBuffer)> callback_t;
			std::unordered_map<int32_t, callback_t> commandBufferCallbacks;

			//
			const std::string getTarget() const;
			void setTarget( const std::string& );

			void bindCallback( int32_t, const callback_t& );

			constexpr static int32_t CALLBACK_BEGIN = -1;
			constexpr static int32_t CALLBACK_END = -2;

			// RAII
			virtual const std::string getType() const;
			virtual const size_t blitters() const;
			virtual ext::vulkan::Graphic* getBlitter(size_t = 0);
			virtual std::vector<ext::vulkan::Graphic*> getBlitters();

			virtual GraphicDescriptor bindGraphicDescriptor( const GraphicDescriptor&, size_t = 0 );
			
			virtual void createCommandBuffers( const std::vector<ext::vulkan::Graphic*>& graphics );
			virtual void initialize( Device& device );
			virtual void tick();
			virtual void destroy();
			virtual void render();
			virtual void pipelineBarrier( VkCommandBuffer, uint8_t = -1 );
		};
	}
}