#pragma once

#include <uf/ext/opengl/rendermode.h>
#include <uf/ext/opengl/graphic.h>

namespace ext {
	namespace opengl {
		struct UF_API RenderTargetRenderMode : RenderMode {
			ext::opengl::Graphic blitter;		
			//
			const std::string getTarget() const;
			void setTarget( const std::string& );

			// RAII
			virtual const std::string getType() const;
			virtual const size_t blitters() const;
			virtual ext::opengl::Graphic* getBlitter(size_t = 0);
			virtual std::vector<ext::opengl::Graphic*> getBlitters();

			virtual GraphicDescriptor bindGraphicDescriptor( const GraphicDescriptor&, size_t = 0 );
			
			virtual void createCommandBuffers( const std::vector<ext::opengl::Graphic*>& graphics );
			virtual void initialize( Device& device );
			virtual void tick();
			virtual void destroy();
			virtual void render();
			virtual void pipelineBarrier( GLhandle(VkCommandBuffer), uint8_t = -1 );
		};
	}
}