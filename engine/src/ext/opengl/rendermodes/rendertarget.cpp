#if UF_USE_OPENGL

#include <uf/ext/opengl/opengl.h>
#include <uf/ext/opengl/rendermodes/rendertarget.h>
#include <uf/ext/opengl/initializers.h>
#include <uf/utils/window/window.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/ext/opengl/graphic.h>
#include <uf/utils/camera/camera.h>


const uf::stl::string ext::opengl::RenderTargetRenderMode::getTarget() const {
//	auto& metadata = *const_cast<uf::Serializer*>(&this->metadata);
//	return metadata["target"].as<uf::stl::string>();
	return metadata.target;
}
void ext::opengl::RenderTargetRenderMode::setTarget( const uf::stl::string& target ) {
//	this->metadata["target"] = target;
	metadata.target = target;
}

const uf::stl::string ext::opengl::RenderTargetRenderMode::getType() const {
	return "RenderTarget";
}
const size_t ext::opengl::RenderTargetRenderMode::blitters() const {
	return 1;
}
ext::opengl::Graphic* ext::opengl::RenderTargetRenderMode::getBlitter( size_t i ) {
	return &this->blitter;
}
uf::stl::vector<ext::opengl::Graphic*> ext::opengl::RenderTargetRenderMode::getBlitters() {
	return { &this->blitter };
}

ext::opengl::GraphicDescriptor ext::opengl::RenderTargetRenderMode::bindGraphicDescriptor( const ext::opengl::GraphicDescriptor& reference, size_t pass ) {
	ext::opengl::GraphicDescriptor descriptor = ext::opengl::RenderMode::bindGraphicDescriptor(reference, pass);
	descriptor.parse(metadata.json["descriptor"]);
	if ( 0 <= pass && pass < metadata.subpasses && metadata.type == "vxgi" ) {
	//	descriptor.cullMode = GL_CULL_MODE_NONE;
		descriptor.depth.test = false;
		descriptor.depth.write = false;
		descriptor.pipeline = "vxgi";
	} else if ( metadata.type == "depth" ) {
	//	descriptor.cullMode = GL_CULL_MODE_NONE;
	}
	// invalidate
	if ( metadata.target != "" && descriptor.renderMode != this->getName() && descriptor.renderMode != metadata.target ) {
		descriptor.invalidated = true;
	} else {
		descriptor.renderMode = this->getName();
	}
	return descriptor;
}

void ext::opengl::RenderTargetRenderMode::initialize( Device& device ) {
	ext::opengl::RenderMode::initialize( device );
	this->setTarget( this->getName() );
}

void ext::opengl::RenderTargetRenderMode::tick() {
	ext::opengl::RenderMode::tick();
}
void ext::opengl::RenderTargetRenderMode::destroy() {
	ext::opengl::RenderMode::destroy();
}
void ext::opengl::RenderTargetRenderMode::render() {
	ext::opengl::RenderMode::render();
}
void ext::opengl::RenderTargetRenderMode::pipelineBarrier( GLhandle(VkCommandBuffer) commandBuffer, uint8_t state ) {
}
void ext::opengl::RenderTargetRenderMode::createCommandBuffers( const uf::stl::vector<ext::opengl::Graphic*>& graphics ) {
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
	#if 0
		CommandBuffer::InfoViewport viewportCommandInfo = {};
		viewportCommandInfo.type = enums::Command::VIEWPORT;
		viewportCommandInfo.corner = pod::Vector2ui{0, 0};
		viewportCommandInfo.size = pod::Vector2ui{width, height};
		commands.record(viewportCommandInfo);
	#endif

		size_t currentSubpass = 0;
		size_t currentPass = 0;
		size_t currentDraw = 0;
		for ( auto graphic : graphics ) {
			if ( graphic->descriptor.renderMode != this->getTarget() ) continue;
			GraphicDescriptor descriptor = bindGraphicDescriptor(graphic->descriptor, currentSubpass);
			if ( descriptor.invalidated ) continue;
			graphic->record( commands, descriptor, currentPass, currentDraw++ );
		}
	} commands.end();
}

#endif