#if UF_USE_OPENGL

#include <uf/ext/opengl/opengl.h>
#include <uf/ext/opengl/rendermodes/deferred.h>
#include <uf/ext/opengl/rendermodes/rendertarget.h>
#include <uf/ext/opengl/initializers.h>
#include <uf/utils/window/window.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/ext/opengl/graphic.h>
#include <uf/engine/scene/scene.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/math/transform.h>

#include <uf/ext/opengl/graphic.h>
#include <uf/ext/gltf/graph.h>

const std::string ext::opengl::DeferredRenderMode::getType() const {
	return "Deferred";
}
const size_t ext::opengl::DeferredRenderMode::blitters() const {
	return 1;
}
ext::opengl::Graphic* ext::opengl::DeferredRenderMode::getBlitter( size_t i ) {
	return &this->blitter;
}
std::vector<ext::opengl::Graphic*> ext::opengl::DeferredRenderMode::getBlitters() {
	return { &this->blitter };
}

void ext::opengl::DeferredRenderMode::initialize( Device& device ) {
	auto windowSize = device.window->getSize();
	this->width = windowSize.x;
	this->height = windowSize.y;

	ext::opengl::RenderMode::initialize( device );
}
void ext::opengl::DeferredRenderMode::tick() {
	ext::opengl::RenderMode::tick();
}
void ext::opengl::DeferredRenderMode::destroy() {
	ext::opengl::RenderMode::destroy();
}
void ext::opengl::DeferredRenderMode::createCommandBuffers( const std::vector<ext::opengl::Graphic*>& graphics ) {
	float width = this->width > 0 ? this->width : ext::opengl::settings::width;
	float height = this->height > 0 ? this->height : ext::opengl::settings::height;
}

#endif