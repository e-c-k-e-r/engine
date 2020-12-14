#pragma once

#include <uf/ext/vulkan/rendermode.h>
#include <uf/ext/vulkan/graphic.h>

namespace ext {
	namespace vulkan {
		struct UF_API RenderTargetRenderMode : public ext::vulkan::RenderMode {
			ext::vulkan::Graphic blitter;		
			std::string target;
			uf::Serializer metadata;

			// RAII
			virtual std::string getType() const;
			virtual const std::string& getName( bool ) const;
			virtual const size_t blitters() const;
			virtual ext::vulkan::Graphic* getBlitter(size_t = 0);
			virtual std::vector<ext::vulkan::Graphic*> getBlitters();
			
			virtual void createCommandBuffers( const std::vector<ext::vulkan::Graphic*>& graphics );
			virtual void initialize( Device& device );
			virtual void tick();
			virtual void destroy();
			virtual void render();
			virtual void pipelineBarrier( VkCommandBuffer, uint8_t = -1 );
		};
	}
}