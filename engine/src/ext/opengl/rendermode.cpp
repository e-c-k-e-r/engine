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
#include <uf/utils/math/transform.h>
#include <uf/utils/camera/camera.h>

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
	// if ( this->getName() != "" ) return;
	this->execute = true;

	this->synchronize();

	float width = this->width > 0 ? this->width : ext::opengl::settings::width;
	float height = this->height > 0 ? this->height : ext::opengl::settings::height;

	auto& commands = getCommands();
	commands.flush();
#if 1
	commands.record([]{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	});
	commands.record([]{
		glViewport(0, 0, ext::opengl::settings::width, ext::opengl::settings::height);	

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(90.0f,(GLfloat) ext::opengl::settings::width / (GLfloat) ext::opengl::settings::height, 0.1f, 100.0f);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
	});
#endif
#if 0
	commands.record([]{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	});
	commands.record([]{
		glViewport(0, 0, ext::opengl::settings::width, ext::opengl::settings::height);	

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(90.0f,(GLfloat) ext::opengl::settings::width / (GLfloat) ext::opengl::settings::height, 0.1f, 100.0f);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
	});
	commands.record([]{
		glTranslatef(-1.5f,0.0f,-6.0f);
		glBegin(GL_POLYGON);
			glColor3f(1.0f,0.0f,0.0f);
			glVertex3f( 0.0f, 1.0f, 0.0f);
			glColor3f(0.0f,1.0f,0.0f);
			glVertex3f( 1.0f,-1.0f, 0.0f);
			glColor3f(0.0f,0.0f,1.0f);
			glVertex3f(-1.0f,-1.0f, 0.0f);
		glEnd();

		glTranslatef(3.0f,0.0f,0.0f);
		glBegin(GL_QUADS);
			glColor3f(0.5f,0.5f,1.0f);
			glVertex3f(-1.0f, 1.0f, 0.0f);
			glColor3f(0.5f,0.5f,1.0f);
			glVertex3f( 1.0f, 1.0f, 0.0f);
			glColor3f(0.5f,0.5f,1.0f);
			glVertex3f( 1.0f,-1.0f, 0.0f);
			glColor3f(0.5f,0.5f,1.0f);
			glVertex3f(-1.0f,-1.0f, 0.0f);
		glEnd();
	});
#endif
#if 0
	{
		struct GLvertex {
			pod::Vector3f position = {};
			pod::Vector3f normal = {};
			pod::Vector3f color = {};
			pod::Vector2f uv = {};
		};
		std::vector<GLvertex> glMesh;
		glMesh.push_back({
			.position = pod::Vector3f{ 0.0f, 1.0f, 0.0f},
			.normal = pod::Vector3f{0, 0, 0},
			.color = pod::Vector3f{1.0f,0.0f,0.0f},
			.uv = pod::Vector2f{0, 0},
		});
		glMesh.push_back({
			.position = pod::Vector3f{ 1.0f,-1.0f, 0.0f},
			.normal = pod::Vector3f{0, 0, 0},
			.color = pod::Vector3f{0.0f,1.0f,0.0f},
			.uv = pod::Vector2f{0, 0},
		});
		glMesh.push_back({
			.position = pod::Vector3f{-1.0f,-1.0f, 0.0f},
			.normal = pod::Vector3f{0, 0, 0},
			.color = pod::Vector3f{0.0f,0.0f,1.0f},
			.uv = pod::Vector2f{0, 0},
		});
		commands.record([=]{
			glTranslatef(-1.5f,0.0f,-6.0f);
			glBegin(GL_POLYGON);
			for ( auto& glVertex : glMesh ) {
				glColor3f(glVertex.color[0], glVertex.color[1], glVertex.color[2]);
			//	glTexCoord2f(glVertex.uv[0], glVertex.uv[1]);
			//	glNormal3f(glVertex.normal[0], glVertex.normal[1], glVertex.normal[2]);
				glVertex3f(glVertex.position[0], glVertex.position[1], glVertex.position[2]);
			}
			glEnd();
		});
	}
#endif
#if 1
	std::vector<uf::Entity*> entities;
	std::function<void(uf::Entity*)> filter = [&]( uf::Entity* entity ) {
		if ( !entity->hasComponent<uf::Graphic>() ) return;
		ext::opengl::Graphic& graphic = entity->getComponent<uf::Graphic>();
		if ( !graphic.initialized || !graphic.process || graphic.descriptor.renderMode != this->getName() ) return;
		entities.emplace_back(entity);
	};
	size_t currentDraw = 0;
	size_t currentPass = 0;
	size_t currentSubpass = 0;
	for ( uf::Scene* scene : uf::scene::scenes ) {
		if ( !scene ) continue;
		scene->process(filter);
		
		auto& controller = scene->getController();
		auto& camera = controller.getComponent<uf::Camera>();
		auto& controllerTransform = controller.getComponent<pod::Transform<>>();
		auto& sceneMetadata = scene->getComponent<uf::Serializer>();
	/*
		if ( !ext::json::isNull( sceneMetadata["system"]["renderer"]["clear values"][0] ) ) {
			auto& v = sceneMetadata["system"]["renderer"]["clear values"][0];
			commands.record([&](){
				glClearColor(v[0].as<float>(), v[1].as<float>(), v[2].as<float>(), v[3].as<float>());
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			});
		}
	*/

		auto& view = camera.getView();
		auto& projection = camera.getProjection();
		auto rotation = uf::matrix::inverse( uf::quaternion::matrix( controllerTransform.orientation ) );
	/*	
		commands.record([&](){
			glMatrixMode(GL_PROJECTION);
			glLoadMatrixf( &projection[0] );
		});
	*/
		for ( auto* pointer : entities ) {
			uf::Object& entity = pointer->as<uf::Object>();
			auto& graphic = entity.getComponent<uf::Graphic>();
		/*
			auto& transform = entity.getComponent<pod::Transform<>>();
			auto model = view * uf::transform::model( transform );
			commands.record([&](){
				glMatrixMode(GL_MODELVIEW);
				glLoadMatrixf( &model[0] );
			});
		*/
		/*
			auto transform = uf::transform::flatten( entity.getComponent<pod::Transform<>>() );
			commands.record([&](){
				glMatrixMode(GL_MODELVIEW);
				glLoadIdentity();
				glTranslatef( transform.position.x, transform.position.y, transform.position.z );				
			});
		*/
		/*
			commands.record([&](){
				glMatrixMode(GL_MODELVIEW);
				glLoadIdentity();
			//	glTranslatef(-1.5f,0.0f,-6.0f);
			});
		*/
			commands.record([&](){
				glMatrixMode(GL_MODELVIEW);
				glLoadMatrixf( &rotation[0] );
				glTranslatef(-1.5f,0.0f,-6.0f);
			});
			auto descriptor = bindGraphicDescriptor(graphic.descriptor, currentSubpass);
			graphic.record( commands, descriptor, currentPass, currentDraw++ );
		}
		entities.clear();
	}
#endif
//	std::cout << "Recording | " << this->getName() << ": " << this->getType() << " | Commands: " << commands.size() << std::endl;
//	bindPipelines( graphics );
//	createCommandBuffers( graphics );
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
	auto& commands = getCommands(this->mostRecentCommandPoolId);
	commands.submit();
#if UF_ENV_DREAMCAST
	glKosSwapBuffers();
#else
	if (device && device->context && device->context->setActive(true)) {
		device->context->display();
	}
#endif
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