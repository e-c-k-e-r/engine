#pragma once

#include <uf/ext/vulkan/rendermode.h>
#include <uf/ext/vulkan/graphic.h>

namespace ext {
	namespace vulkan {
		struct UF_API ComputeRenderMode : public ext::vulkan::RenderMode {
			ext::vulkan::Graphic blitter;

			//
			const uf::stl::string getTarget() const;
			void setTarget( const uf::stl::string& );

			//
			virtual const uf::stl::string getType() const;
			virtual const size_t blitters() const;
			virtual ext::vulkan::Graphic* getBlitter(size_t = 0);
			virtual uf::stl::vector<ext::vulkan::Graphic*> getBlitters();

			virtual GraphicDescriptor bindGraphicDescriptor( const GraphicDescriptor&, size_t = 0 );
			
			virtual void createCommandBuffers( const uf::stl::vector<ext::vulkan::Graphic*>& graphics );
			virtual void initialize( Device& device );
			virtual void tick();
			virtual void destroy();
			virtual void render();
			virtual void pipelineBarrier( VkCommandBuffer, uint8_t = -1 );
		};
	}
}