#pragma once

#include <uf/ext/opengl/rendermode.h>
#include <uf/ext/opengl/graphic.h>

namespace ext {
	namespace opengl {
		struct UF_API RenderTargetRenderMode : public RenderMode {
			virtual const uf::stl::string getType() const;

			virtual GraphicDescriptor bindGraphicDescriptor( const GraphicDescriptor&, size_t = 0 );
			
			virtual void createCommandBuffers( const uf::stl::vector<ext::opengl::Graphic*>& graphics );
			virtual void initialize( Device& device );
			virtual void tick();
			virtual void destroy();
			virtual void render();
			virtual void pipelineBarrier( GLhandle(VkCommandBuffer), uint8_t = -1 );
		};
	}
}