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

const uf::stl::string ext::opengl::DeferredRenderMode::getType() const {
	return "Deferred";
}

void ext::opengl::DeferredRenderMode::initialize( Device& device ) {
	ext::opengl::RenderMode::initialize( device );
}
void ext::opengl::DeferredRenderMode::tick() {
	ext::opengl::RenderMode::tick();
}
void ext::opengl::DeferredRenderMode::render() {
	ext::opengl::RenderMode::render();
}
void ext::opengl::DeferredRenderMode::destroy() {
	ext::opengl::RenderMode::destroy();
}
ext::opengl::GraphicDescriptor ext::opengl::DeferredRenderMode::bindGraphicDescriptor( const ext::opengl::GraphicDescriptor& reference, size_t pass ) {
	ext::opengl::GraphicDescriptor descriptor = ext::opengl::RenderMode::bindGraphicDescriptor(reference, pass);
	if ( descriptor.renderMode != "" ) descriptor.invalidated = true;
	descriptor.depth.min = 0.0f;
	descriptor.depth.max = 1.0f;
	descriptor.depth.operation = GL_LEQUAL;
	descriptor.depth.write = true;
	descriptor.depth.test = true;
	return descriptor;
}
void ext::opengl::DeferredRenderMode::createCommandBuffers( const uf::stl::vector<ext::opengl::Graphic*>& graphics ) {
	float width = this->width > 0 ? this->width : ext::opengl::settings::width;
	float height = this->height > 0 ? this->height : ext::opengl::settings::height;

	auto& commands = getCommands();	
	commands.start(); {
		{
			CommandBuffer::InfoViewport viewportCommandInfo = {};
			viewportCommandInfo.type = enums::Command::VIEWPORT;
			viewportCommandInfo.corner = pod::Vector2ui{0, 0};
			viewportCommandInfo.size = pod::Vector2ui{width, height};
			commands.record(viewportCommandInfo);
		}
		{
			CommandBuffer::InfoClear clearCommandInfo = {};
			clearCommandInfo.type = enums::Command::CLEAR;
			clearCommandInfo.color = {0.0f, 0.0f, 0.0f, 0.0f};
			clearCommandInfo.bits = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;
			clearCommandInfo.depth = 1.0f;
			commands.record(clearCommandInfo);
		}
		
		size_t currentSubpass = 0;
		size_t currentPass = 0;
		size_t currentDraw = 0;
		// draw skybox'd geometry
		for ( auto graphic : graphics ) {
			if ( graphic->descriptor.renderMode != this->getName() ) continue;
			if ( graphic->descriptor.aux != 1 ) continue;
			GraphicDescriptor descriptor = bindGraphicDescriptor(graphic->descriptor, currentSubpass);
			graphic->record( commands, descriptor, currentPass, currentDraw++ );
		}
		// clear depth
		if ( currentDraw > 0 ) {
			CommandBuffer::InfoClear clearCommandInfo = {};
			clearCommandInfo.type = enums::Command::CLEAR;
			clearCommandInfo.bits = GL_DEPTH_BUFFER_BIT;
			clearCommandInfo.depth = 1.0f;
			commands.record(clearCommandInfo);
		}
		// draw normal geometry
		for ( auto graphic : graphics ) {
			if ( graphic->descriptor.renderMode != this->getName() ) continue;
			if ( graphic->descriptor.aux != 0 ) continue;
			if ( graphic->descriptor.renderTarget != 0 ) continue;
			GraphicDescriptor descriptor = bindGraphicDescriptor(graphic->descriptor, currentSubpass);
			graphic->record( commands, descriptor, currentPass, currentDraw++ );
		}
		// draw transparency
		for ( auto graphic : graphics ) {
			if ( graphic->descriptor.renderMode != this->getName() ) continue;
			if ( graphic->descriptor.aux != 0 ) continue;
			if ( graphic->descriptor.renderTarget != 1 ) continue;
			GraphicDescriptor descriptor = bindGraphicDescriptor(graphic->descriptor, currentSubpass);
			graphic->record( commands, descriptor, currentPass, currentDraw++ );
		}
	} commands.end();
}

#endif