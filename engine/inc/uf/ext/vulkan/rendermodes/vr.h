#pragma once

#include <uf/ext/vulkan/rendermode.h>

namespace ext {
	namespace vulkan {
		struct UF_API VrRenderMode : public ext::vulkan::RenderMode {
			virtual const uf::stl::string getType() const;
			
			virtual void createCommandBuffers( const uf::stl::vector<ext::vulkan::Graphic*>& graphics );
			virtual void initialize( Device& device );
			virtual void build( bool = true );
			virtual void tick();
			virtual void destroy();
			virtual void render();
		};
	}
}