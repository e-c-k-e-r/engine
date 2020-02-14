#if UF_USE_OPENGL

#include <uf/ext/opengl/opengl.h>
#include <uf/ext/opengl/rendermodes/base.h>
#include <uf/ext/opengl/initializers.h>
#include <uf/utils/window/window.h>
#include <uf/ext/opengl/graphic.h>
#include <uf/ext/opengl/rendertarget.h>
#include <uf/utils/graphic/graphic.h>

const std::string ext::opengl::BaseRenderMode::getType() const {
	return "Swapchain";
}
void ext::opengl::BaseRenderMode::createCommandBuffers( const std::vector<ext::opengl::Graphic*>& graphics ) {
}

void ext::opengl::BaseRenderMode::tick() {
}
void ext::opengl::BaseRenderMode::render() {
#if UF_ENV_DREAMCAST
	//glKosSwapBuffers();
#elif UF_USE_GLEW

#endif
}

void ext::opengl::BaseRenderMode::initialize( Device& device ) {
}

void ext::opengl::BaseRenderMode::destroy() {
}

#endif