#pragma once

#include <uf/ext/opengl/rendermode.h>
#include <uf/ext/opengl/graphic.h>

namespace ext {
	namespace opengl {
		struct UF_API ComputeRenderMode : public RenderMode {
			ext::opengl::Graphic blitter, compute;
			pod::Vector2ui dispatchSize = { 32, 32 };

			// RAII
			virtual const uf::stl::string getType() const;
			virtual const size_t blitters() const;
			virtual ext::opengl::Graphic* getBlitter(size_t = 0);
			virtual uf::stl::vector<ext::opengl::Graphic*> getBlitters();
			
			virtual void createCommandBuffers();
			virtual void initialize( Device& device );
			virtual void tick();
			virtual void render();
			virtual void destroy();
			virtual void bindPipelines();
			virtual void pipelineBarrier( GLhandle(VkCommandBuffer), uint8_t = -1 );
		};
	}
}