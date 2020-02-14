#if UF_USE_OPENGL

#include <uf/ext/opengl/opengl.h>
#include <uf/ext/opengl/rendermodes/compute.h>
#include <uf/ext/opengl/rendermodes/deferred.h>
#include <uf/ext/opengl/rendermodes/rendertarget.h>
#include <uf/ext/opengl/initializers.h>
#include <uf/ext/openvr/openvr.h>
#include <uf/utils/window/window.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/ext/opengl/graphic.h>
#include <uf/engine/scene/scene.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/math/transform.h>

#include <uf/ext/opengl/graphic.h>

const std::string ext::opengl::ComputeRenderMode::getType() const {
	return "Compute";
}
const size_t ext::opengl::ComputeRenderMode::blitters() const {
	return 1;
}
ext::opengl::Graphic* ext::opengl::ComputeRenderMode::getBlitter( size_t i ) {
	return &this->blitter;
}
std::vector<ext::opengl::Graphic*> ext::opengl::ComputeRenderMode::getBlitters() {
	return { &this->blitter };
}

void ext::opengl::ComputeRenderMode::initialize( Device& device ) {
	ext::opengl::RenderMode::initialize( device );
}
void ext::opengl::ComputeRenderMode::render() {
}
void ext::opengl::ComputeRenderMode::tick() {
	ext::opengl::RenderMode::tick();
}
void ext::opengl::ComputeRenderMode::destroy() {
	ext::opengl::RenderMode::destroy();
}
void ext::opengl::ComputeRenderMode::pipelineBarrier( GLhandle(VkCommandBuffer) commandBuffer, uint8_t state ) {
}
void ext::opengl::ComputeRenderMode::createCommandBuffers( ) {
}
void ext::opengl::ComputeRenderMode::bindPipelines() {
}

#endif