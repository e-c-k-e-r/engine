#pragma once

#include <uf/ext/opengl/rendermode.h>

namespace ext {
	namespace opengl {
		struct UF_API BaseRenderMode : public RenderMode {
			virtual const uf::stl::string getType() const;
			
			virtual void initialize( Device& device );
			virtual void createCommandBuffers( const uf::stl::vector<ext::opengl::Graphic*>& graphics );
			virtual void tick();
			virtual void render();
			virtual void destroy();
		};
	}
}