#if UF_USE_OPENGL

#include <uf/ext/opengl/opengl.h>
#include <uf/ext/opengl/rendermodes/base.h>
#include <uf/ext/opengl/initializers.h>
#include <uf/utils/window/window.h>
#include <uf/ext/opengl/graphic.h>
#include <uf/ext/opengl/rendertarget.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/utils/camera/camera.h>

const uf::stl::string ext::opengl::BaseRenderMode::getType() const {
	return "Swapchain";
}
void ext::opengl::BaseRenderMode::createCommandBuffers( const uf::stl::vector<ext::opengl::Graphic*>& graphics ) {

}

void ext::opengl::BaseRenderMode::initialize( Device& device ) {
	this->metadata.name = "Swapchain";
	auto windowSize = device.window->getSize();
	this->width = windowSize.x;
	this->height = windowSize.y;


#if 0
	GL_ERROR_CHECK(glDisable(GL_BLEND));
	GL_ERROR_CHECK(glDisable(GL_ALPHA_TEST));

	GL_ERROR_CHECK(glDisable(GL_CULL_FACE));
	GL_ERROR_CHECK(glCullFace(GL_BACK));
	GL_ERROR_CHECK(glFrontFace(GL_CCW));

	GL_ERROR_CHECK(glDisable(GL_DEPTH_TEST));
	GL_ERROR_CHECK(glDepthMask(GL_TRUE));
	GL_ERROR_CHECK(glDepthRange(0.0f, 1.0f));
	GL_ERROR_CHECK(glDepthFunc(GL_LEQUAL));

	GL_ERROR_CHECK(glColor4f(1.0f, 1.0f, 1.0f, 1.0f));
	GL_ERROR_CHECK(glLineWidth(1.0f));

	GL_ERROR_CHECK(glDisable(GL_LIGHTING));
	GL_ERROR_CHECK(glDisable(GL_COLOR_MATERIAL));

	GL_ERROR_CHECK(glActiveTexture(GL_TEXTURE0));
	GL_ERROR_CHECK(glClientActiveTexture(GL_TEXTURE0));
	GL_ERROR_CHECK(glDisable(GL_TEXTURE_2D));
	GL_ERROR_CHECK(glBindTexture(GL_TEXTURE_2D, 0));
	GL_ERROR_CHECK(glDisableClientState(GL_TEXTURE_COORD_ARRAY));

	GL_ERROR_CHECK(glActiveTexture(GL_TEXTURE1));
	GL_ERROR_CHECK(glClientActiveTexture(GL_TEXTURE1));
	GL_ERROR_CHECK(glDisable(GL_TEXTURE_2D));
	GL_ERROR_CHECK(glBindTexture(GL_TEXTURE_2D, 0));
	GL_ERROR_CHECK(glDisableClientState(GL_TEXTURE_COORD_ARRAY));

	GL_ERROR_CHECK(glDisableClientState(GL_NORMAL_ARRAY));
	GL_ERROR_CHECK(glDisableClientState(GL_COLOR_ARRAY));
	GL_ERROR_CHECK(glDisableClientState(GL_VERTEX_ARRAY));
#endif

	ext::opengl::RenderMode::initialize( device );
}

void ext::opengl::BaseRenderMode::tick() {
	if ( ext::opengl::states::resized ) {
		auto windowSize = device->window->getSize();
		this->width = windowSize.x;
		this->height = windowSize.y;
		this->rebuild = true;
	}
	ext::opengl::RenderMode::tick();
}
void ext::opengl::BaseRenderMode::render() {
#if 0
	ext::opengl::RenderMode::render();
#endif
}

void ext::opengl::BaseRenderMode::destroy() {
	ext::opengl::RenderMode::destroy();
}

#endif