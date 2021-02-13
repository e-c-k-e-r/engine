#if UF_USE_OPENGL

#include <uf/ext/opengl/opengl.h>
#include <uf/ext/opengl/rendermode.h>
#include <uf/ext/opengl/initializers.h>
#include <uf/ext/opengl/graphic.h>
#include <uf/utils/window/window.h>

#include <uf/ext/opengl/rendertarget.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/utils/serialize/serializer.h>
#include <uf/engine/scene/scene.h>

#include <uf/ext/openvr/openvr.h>

ext::opengl::RenderMode::~RenderMode() {
	this->destroy();
}
const std::string ext::opengl::RenderMode::getType() const {
	return "";
}
const std::string ext::opengl::RenderMode::getName() const {
	auto& metadata = *const_cast<uf::Serializer*>(&this->metadata);
	return metadata["name"].as<std::string>();
}
ext::opengl::RenderTarget& ext::opengl::RenderMode::getRenderTarget( size_t i ) {
	return renderTarget;
}
const ext::opengl::RenderTarget& ext::opengl::RenderMode::getRenderTarget( size_t i ) const {
	return renderTarget;
}

const size_t ext::opengl::RenderMode::blitters() const {
	return 0;
}
ext::opengl::Graphic* ext::opengl::RenderMode::getBlitter( size_t i ) {
	return NULL;
}
std::vector<ext::opengl::Graphic*> ext::opengl::RenderMode::getBlitters() {
	return {};
}

ext::opengl::GraphicDescriptor ext::opengl::RenderMode::bindGraphicDescriptor( const ext::opengl::GraphicDescriptor& reference, size_t pass ) {
	ext::opengl::GraphicDescriptor descriptor = reference;
	descriptor.renderMode = this->getName();
	descriptor.subpass = pass;
	descriptor.parse( metadata );
	return descriptor;
}
void ext::opengl::RenderMode::bindGraphicPushConstants( ext::opengl::Graphic* pointer, size_t pass ) {
}

void ext::opengl::RenderMode::createCommandBuffers() {
	this->execute = true;

	std::vector<ext::opengl::Graphic*> graphics;
	std::function<void(uf::Entity*)> filter = [&]( uf::Entity* entity ) {
		if ( !entity->hasComponent<uf::Graphic>() ) return;
		ext::opengl::Graphic& graphic = entity->getComponent<uf::Graphic>();
		if ( !graphic.initialized || !graphic.process ) return;
		graphics.push_back(&graphic);
	};
	for ( uf::Scene* scene : uf::scene::scenes ) {
		if ( !scene ) continue;
		scene->process(filter);
	}

	this->synchronize();
//	bindPipelines( graphics );
	createCommandBuffers( graphics );
	this->mostRecentCommandPoolId = std::this_thread::get_id();
	this->rebuild = false;
}
ext::opengl::RenderMode::commands_container_t& ext::opengl::RenderMode::getCommands() {
	return getCommands( std::this_thread::get_id() );
}
ext::opengl::RenderMode::commands_container_t& ext::opengl::RenderMode::getCommands( std::thread::id id ) {
	bool exists = this->commands.has(id); //this->commands.count(id) > 0;
	auto& commands = this->commands.get(id); //this->commands[id];
	return commands;
}
void ext::opengl::RenderMode::createCommandBuffers( const std::vector<ext::opengl::Graphic*>& graphics ) {

}
void ext::opengl::RenderMode::bindPipelines() {
	this->execute = true;

	std::vector<ext::opengl::Graphic*> graphics;
	std::function<void(uf::Entity*)> filter = [&]( uf::Entity* entity ) {
		if ( !entity->hasComponent<uf::Graphic>() ) return;
		ext::opengl::Graphic& graphic = entity->getComponent<uf::Graphic>();
		if ( !graphic.initialized ) return;
		if ( !graphic.process ) return;
		graphics.push_back(&graphic);
	};
	for ( uf::Scene* scene : uf::scene::scenes ) {
		if ( !scene ) continue;
		scene->process(filter);
	}

	this->synchronize();
	this->bindPipelines( graphics );
}
void ext::opengl::RenderMode::bindPipelines( const std::vector<ext::opengl::Graphic*>& graphics ) {
	for ( auto* pointer : graphics ) {
		auto& graphic = *pointer;
		for ( size_t currentPass = 0; currentPass < renderTarget.passes.size(); ++currentPass ) {
			auto& subpass = renderTarget.passes[currentPass];
			if ( !subpass.autoBuildPipeline ) continue;
			// bind to this render mode
			ext::opengl::GraphicDescriptor descriptor = bindGraphicDescriptor(graphic.descriptor, currentPass);
			// ignore if pipeline exists for this render mode
			if ( graphic.hasPipeline( descriptor ) ) continue;
			graphic.initializePipeline( descriptor );
		}
	}
}

void ext::opengl::RenderMode::render() {
}

void ext::opengl::RenderMode::initialize( Device& device ) {
	this->device = &device;

	// this->width = 0; //ext::opengl::width;
	// this->height = 0; //ext::opengl::height;
	{
		if ( this->width > 0 ) renderTarget.width = this->width;
		if ( this->height > 0 ) renderTarget.height = this->height;
	}
}

void ext::opengl::RenderMode::tick() {
	this->synchronize();
}

void ext::opengl::RenderMode::destroy() {
	this->synchronize();

	renderTarget.destroy();
}
void ext::opengl::RenderMode::synchronize( uint64_t timeout ) {
}
void ext::opengl::RenderMode::pipelineBarrier( GLhandle(VkCommandBuffer) command, uint8_t stage ) {
}

#endif