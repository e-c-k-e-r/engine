#pragma once

#include <uf/ext/vulkan/rendermode.h>

namespace ext {
	namespace vulkan {
		struct UF_API DeferredRenderMode : public ext::vulkan::RenderMode {			
			virtual const uf::stl::string getType() const;

			virtual GraphicDescriptor bindGraphicDescriptor( const GraphicDescriptor&, size_t = 0 );
			
			virtual void createCommandBuffers( const uf::stl::vector<ext::vulkan::Graphic*>& graphics );
			virtual void initialize( Device& device );
			virtual void tick();
			virtual VkSubmitInfo queue();
			virtual void render();
			virtual void destroy();
		};
	}
}