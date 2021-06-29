#pragma once

#include <uf/ext/vulkan/rendermode.h>

namespace ext {
	namespace vulkan {
		struct UF_API BaseRenderMode : RenderMode {
			// virtual ~RenderMode();
			// RAII
			virtual const uf::stl::string getType() const;
			
			virtual void initialize( Device& device );
			virtual void createCommandBuffers( const uf::stl::vector<ext::vulkan::Graphic*>& graphics );
			virtual void tick();
			virtual void render();
			virtual void destroy();
		};
	}
}