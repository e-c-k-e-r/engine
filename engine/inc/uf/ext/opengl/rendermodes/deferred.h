#pragma once

#include <uf/ext/opengl/rendermode.h>
#include <uf/ext/opengl/graphic.h>

namespace ext {
	namespace opengl {
		struct UF_API DeferredRenderMode : public RenderMode {
			// RAII
			virtual const uf::stl::string getType() const;

			virtual void createCommandBuffers( const uf::stl::vector<ext::opengl::Graphic*>& graphics );
			virtual void initialize( Device& device );
			virtual void tick();
			virtual void render();
			virtual void destroy();
		};
	}
}