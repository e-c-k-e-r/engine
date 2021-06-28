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
#include <uf/engine/graph/graph.h>
#include <uf/utils/camera/camera.h>

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
	
	auto& commands = getCommands();	
	commands.start(); {
	#if 0
		CommandBuffer::InfoClear clearCommandInfo = {};
		clearCommandInfo.type = enums::Command::CLEAR;
		clearCommandInfo.color = {0.0f, 0.0f, 0.0f, 0.0f};
		clearCommandInfo.depth = uf::Camera::USE_REVERSE_INFINITE_PROJECTION ? 0.0f : 1.0f;
		clearCommandInfo.bits = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;
		commands.record(clearCommandInfo);
	#endif
		CommandBuffer::InfoViewport viewportCommandInfo = {};
		viewportCommandInfo.type = enums::Command::VIEWPORT;
		viewportCommandInfo.corner = pod::Vector2ui{0, 0};
		viewportCommandInfo.size = pod::Vector2ui{width, height};
		commands.record(viewportCommandInfo);
		
		size_t currentSubpass = 0;
		size_t currentPass = 0;
		size_t currentDraw = 0;
		for ( auto graphic : graphics ) {
			if ( graphic->descriptor.renderMode != this->getName() ) continue;
			GraphicDescriptor descriptor = bindGraphicDescriptor(graphic->descriptor, currentSubpass);
			graphic->record( commands, descriptor, currentPass, currentDraw++ );
		}
	} commands.end();
}

#endif