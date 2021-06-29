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
uf::stl::vector<ext::opengl::Graphic*> ext::opengl::RenderMode::getBlitters() {
	return {};
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
	auto& graph = scene.getGraph();
	for ( auto entity : graph ) {
		if ( !entity->hasComponent<uf::Graphic>() ) continue;
		ext::opengl::Graphic& graphic = entity->getComponent<uf::Graphic>();
		if ( !graphic.initialized || !graphic.process ) continue;
		graphics.push_back(&graphic);
	}

	this->synchronize();
//	bindPipelines( graphics );
	createCommandBuffers( graphics );
	this->mostRecentCommandPoolId = std::this_thread::get_id();
	this->rebuild = false;
}
ext::opengl::CommandBuffer& ext::opengl::RenderMode::getCommands() {
	return getCommands( std::this_thread::get_id() );
}
ext::opengl::CommandBuffer& ext::opengl::RenderMode::getCommands( std::thread::id id ) {
	bool exists = this->commands.has(id); //this->commands.count(id) > 0;
	auto& commands = this->commands.get(id); //this->commands[id];
	if ( !exists ) {
		commands.initialize( *device );
	}
	return commands;
}
void ext::opengl::RenderMode::createCommandBuffers( const uf::stl::vector<ext::opengl::Graphic*>& graphics ) {

}
void ext::opengl::RenderMode::bindPipelines() {
	this->execute = true;

	uf::stl::vector<ext::opengl::Graphic*> graphics;
	auto& scene = uf::scene::getCurrentScene(); 
	auto& graph = scene.getGraph();
	for ( auto entity : graph ) {
		if ( !entity->hasComponent<uf::Graphic>() ) continue;
		ext::opengl::Graphic& graphic = entity->getComponent<uf::Graphic>();
		if ( !graphic.initialized || !graphic.process ) continue;
		graphics.push_back(&graphic);
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

	if ( ext::opengl::renderModes.back() != this ) return;

#if UF_ENV_DREAMCAST
	#if UF_USE_OPENGL_GLDC
		glKosSwapBuffers();
	#else
		glutSwapBuffers();
	#endif
#else
	if ( device ) device->activateContext().display();
#endif
	auto& scene = uf::scene::getCurrentScene();
	auto& sceneMetadata = scene.getComponent<uf::Serializer>();

	CommandBuffer::InfoClear clearCommandInfo = {};
	clearCommandInfo.type = enums::Command::CLEAR;
	clearCommandInfo.color = {0.0f, 0.0f, 0.0f, 0.0f};
	clearCommandInfo.depth = uf::matrix::reverseInfiniteProjection ? 0.0f : 1.0f;
	clearCommandInfo.bits = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;
	if ( !ext::json::isNull( sceneMetadata["system"]["renderer"]["clear values"][0] ) ) {
		clearCommandInfo.color = uf::vector::decode( sceneMetadata["system"]["renderer"]["clear values"][0], pod::Vector4f{0,0,0,0} );
	}
	if ( !ext::json::isNull( sceneMetadata["light"]["ambient"] ) ) {
		clearCommandInfo.color = uf::vector::decode( sceneMetadata["light"]["ambient"], pod::Vector4f{0,0,0,0} );
	//	auto ambient = uf::vector::decode( sceneMetadata["light"]["ambient"], pod::Vector4f{1,1,1,1} );
	//	GL_ERROR_CHECK(glLightfv(GL_LIGHT0, GL_AMBIENT, &ambient[0]));
	}

	GL_ERROR_CHECK(glClearColor(clearCommandInfo.color[0], clearCommandInfo.color[1], clearCommandInfo.color[2], clearCommandInfo.color[3]));
	GL_ERROR_CHECK(glClearDepth(clearCommandInfo.depth));
	GL_ERROR_CHECK(glClear(clearCommandInfo.bits));

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