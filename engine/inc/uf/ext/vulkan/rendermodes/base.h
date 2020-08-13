#pragma once

#include <uf/ext/vulkan/rendermode.h>

namespace ext {
	namespace vulkan {
		struct UF_API BaseRenderMode : RenderMode {
			// virtual ~RenderMode();
			// RAII
			virtual std::string getType() const;
			const std::string& getName() const;
			
			virtual void initialize( Device& device );
			virtual void createCommandBuffers( const std::vector<ext::vulkan::Graphic*>& graphics );
			virtual void tick();
			virtual void render();
			virtual void destroy();
		};
	}
}