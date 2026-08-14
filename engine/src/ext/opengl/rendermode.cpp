#if UF_USE_OPENGL

#include <uf/ext/opengl/ogl.h>
#include <uf/ext/opengl/opengl.h>
#include <uf/ext/opengl/rendermode.h>
#include <uf/ext/opengl/initializers.h>
#include <uf/ext/opengl/graphic.h>
#include <uf/utils/window/window.h>

#include <uf/ext/opengl/rendertarget.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/utils/serialize/serializer.h>
#include <uf/engine/scene/scene.h>
#include <uf/utils/math/transform.h>
#include <uf/utils/camera/camera.h>

#include <uf/ext/openvr/openvr.h>

ext::opengl::RenderMode::~RenderMode() {
	this->destroy();
}
const uf::stl::string ext::opengl::RenderMode::getType() const {
	return "";
}
const uf::stl::string ext::opengl::RenderMode::getName() const {
//	auto& metadata = *const_cast<uf::Serializer*>(&this->metadata);
//	return metadata["name"].as<uf::stl::string>();
	return metadata.name;
}
ext::opengl::Graphic& ext::opengl::RenderMode::getBlitter() {
	return blitter;
}
ext::opengl::RenderTarget& ext::opengl::RenderMode::getRenderTarget( size_t i ) {
	return renderTarget;
}
const uf::stl::string ext::opengl::RenderMode::getTarget() const {
//	auto& metadata = *const_cast<uf::Serializer*>(&this->metadata);
//	return metadata["target"].as<uf::stl::string>();
	return metadata.target;
}
void ext::opengl::RenderMode::setTarget( const uf::stl::string& target ) {
//	this->metadata["target"] = target;
	metadata.target = target;
}

ext::opengl::GraphicDescriptor ext::opengl::RenderMode::bindGraphicDescriptor( const ext::opengl::GraphicDescriptor& reference, size_t pass ) {
	ext::opengl::GraphicDescriptor descriptor = reference;
//	descriptor.renderMode = this->getName();
	descriptor.subpass = pass;
	descriptor.parse( metadata.json["descriptor"] );
	return descriptor;
}
void ext::opengl::RenderMode::bindGraphicPushConstants( ext::opengl::Graphic* pointer, size_t pass ) {
}

void ext::opengl::RenderMode::createCommandBuffers() {
	this->execute = true;

	uf::stl::vector<ext::opengl::Graphic*> graphics;
	auto& scene = uf::scene::getCurrentScene(); 
	auto/*&*/ graph = scene.getGraph();
	for ( auto entity : graph ) {
		if ( entity->hasComponent<ext::opengl::Graphics>() ) {
			auto& g = entity->getComponent<ext::opengl::Graphics>();
			for ( auto& [ _, graphic ] : g ) {
				if ( !graphic.initialized || !graphic.process ) continue;
				graphics.emplace_back(&graphic);
			}
		}
		if ( entity->hasComponent<ext::opengl::Graphic>() ) {
			auto& graphic = entity->getComponent<ext::opengl::Graphic>();
			if ( !graphic.initialized || !graphic.process ) continue;
			graphics.emplace_back(&graphic);
		}
	}

	this->synchronize();
//	bindPipelines( graphics );
	createCommandBuffers( graphics );
	this->mostRecentCommandPoolId = uf::thread::current_id();
	this->rebuild = false;
}
ext::opengl::CommandBuffer& ext::opengl::RenderMode::getCommands( uf::thread::id_t id ) {
	bool exists = this->commands.has(id); //this->commands.count(id) > 0;
	auto& commands = this->commands.get(id); //this->commands[id];
	if ( !exists ) {
		commands.initialize( *device );
	}
	return commands;
}
void ext::opengl::RenderMode::createCommandBuffers( const uf::stl::vector<ext::opengl::Graphic*>& graphics ) {

}

bool ext::opengl::RenderMode::hasBuffer( const uf::stl::string& name ) const {
	return metadata.buffers.count(name) > 0;
}
const ext::opengl::Buffer& ext::opengl::RenderMode::getBuffer( const uf::stl::string& name ) const {
	UF_ASSERT_MSG( hasBuffer( name ), "attachment in `{}`: {} not found: {}", this->getName(), this->getType(), name );
	return this->buffers[metadata.buffers.at(name)];
}
size_t ext::opengl::RenderMode::getBufferIndex( const uf::stl::string& name ) const {
	return hasBuffer( name ) ? metadata.buffers.at(name) : SIZE_MAX;
}

void ext::opengl::RenderMode::bindPipelines() {
	this->execute = true;

	uf::stl::vector<ext::opengl::Graphic*> graphics;
	auto& scene = uf::scene::getCurrentScene(); 
	auto/*&*/ graph = scene.getGraph();
	for ( auto entity : graph ) {
		if ( !entity->hasComponent<uf::Graphic>() ) continue;
		ext::opengl::Graphic& graphic = entity->getComponent<uf::Graphic>();
		if ( !graphic.initialized || !graphic.process ) continue;
		graphics.emplace_back(&graphic);
	}

	this->synchronize();
	this->bindPipelines( graphics );
}
void ext::opengl::RenderMode::bindPipelines( const uf::stl::vector<ext::opengl::Graphic*>& graphics ) {
	for ( auto* pointer : graphics ) {
		auto& graphic = *pointer;
		for ( size_t currentPass = 0; currentPass < renderTarget.passes.size(); ++currentPass ) {
			auto& subpass = renderTarget.passes[currentPass];
			if ( !subpass.autoBuildPipeline ) continue;
			// bind to this render mode
			ext::opengl::GraphicDescriptor descriptor = bindGraphicDescriptor(graphic.descriptor, currentPass);
			// ignore invalidated descriptors
			if ( descriptor.invalidated ) continue;
			// ignore if pipeline exists for this render mode
			if ( graphic.hasPipeline( descriptor ) ) continue;
			// if pipeline name is specified for the rendermode, check if we have shaders for it
			size_t shaders = 0;
			for ( auto& shader : graphic.material.shaders ) {
				if ( shader.metadata.pipeline == descriptor.pipeline ) ++shaders;
			}
			if ( shaders == 0 ) continue;
			graphic.initializePipeline( descriptor );
		}
	}
}

void ext::opengl::RenderMode::render() {
	auto& commands = getCommands(this->mostRecentCommandPoolId);
	commands.submit();

	this->executed = true;
}

void ext::opengl::RenderMode::initialize( Device& device ) {
	this->device = &device;

	// this->width = 0; //ext::opengl::width;
	// this->height = 0; //ext::opengl::height;
	{
		if ( this->width > 0 ) renderTarget.width = this->width;
		if ( this->height > 0 ) renderTarget.height = this->height;
	}

/*
	if ( !this->hasBuffer("camera") ) {
		this->metadata.buffers["camera"] = this->initializeBuffer( (const void*) nullptr, sizeof(pod::Camera::Viewports), uf::renderer::enums::Buffer::UNIFORM );
	}
*/
}

void ext::opengl::RenderMode::tick() {
	this->synchronize();
}

void ext::opengl::RenderMode::destroy() {
	this->synchronize();
	for ( auto& pair : this->commands.container() ) {
		pair.second.flush();
	}
	renderTarget.destroy();
}
void ext::opengl::RenderMode::synchronize( uint64_t timeout ) {
}
void ext::opengl::RenderMode::pipelineBarrier( GLhandle(VkCommandBuffer) command, uint8_t stage ) {
}

#endif