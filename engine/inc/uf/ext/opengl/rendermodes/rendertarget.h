#pragma once

#include <uf/ext/opengl/rendermode.h>
#include <uf/ext/opengl/graphic.h>

namespace ext {
	namespace opengl {
		struct UF_API RenderTargetRenderMode : public RenderMode {
			ext::opengl::Graphic blitter;		
			//
			const uf::stl::string getTarget() const;
			void setTarget( const uf::stl::string& );

			// RAII
			virtual const uf::stl::string getType() const;
			virtual const size_t blitters() const;
			virtual ext::opengl::Graphic* getBlitter(size_t = 0);
			virtual uf::stl::vector<ext::opengl::Graphic*> getBlitters();

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