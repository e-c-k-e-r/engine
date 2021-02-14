#if UF_USE_OPENGL

#include <uf/ext/opengl/opengl.h>
#include <uf/ext/opengl/rendermodes/base.h>
#include <uf/ext/opengl/initializers.h>
#include <uf/utils/window/window.h>
#include <uf/ext/opengl/graphic.h>
#include <uf/ext/opengl/rendertarget.h>
#include <uf/utils/graphic/graphic.h>

const std::string ext::opengl::BaseRenderMode::getType() const {
	return "Swapchain";
}
void ext::opengl::BaseRenderMode::createCommandBuffers( const std::vector<ext::opengl::Graphic*>& graphics ) {
}

void ext::opengl::BaseRenderMode::initialize( Device& device ) {
	this->metadata["name"] = "Swapchain";
	auto windowSize = device.window->getSize();
	this->width = windowSize.x;
	this->height = windowSize.y;

	ext::opengl::RenderMode::initialize( device );

#if 1
	{
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClearDepth(1.0);
		glDepthFunc(GL_LESS);
		glEnable(GL_DEPTH_TEST);
		glShadeModel(GL_SMOOTH);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(90.0f,(GLfloat) ext::opengl::settings::width / (GLfloat) ext::opengl::settings::height, 0.1f, 100.0f);	// Calculate The Aspect Ratio Of The Window
		glMatrixMode(GL_MODELVIEW);
	}
#endif
}

void ext::opengl::BaseRenderMode::tick() {
	ext::opengl::RenderMode::tick();
#if 0
	if ( ext::opengl::states::resized ) {
		if ( ext::opengl::settings::height <= 0 ) ext::opengl::settings::height = 1;

		glViewport(0, 0, ext::opengl::settings::width, ext::opengl::settings::height);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(45.0f,(GLfloat) ext::opengl::settings::width / (GLfloat) ext::opengl::settings::height, 0.1f, 100.0f);	// Calculate The Aspect Ratio Of The Window
		glMatrixMode(GL_MODELVIEW);
	}
#endif
}
void ext::opengl::BaseRenderMode::render() {
#if 0
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

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

	glColor3f(0.5f,0.5f,1.0f);
	glBegin(GL_QUADS);
		glVertex3f(-1.0f, 1.0f, 0.0f);
		glVertex3f( 1.0f, 1.0f, 0.0f);
		glVertex3f( 1.0f,-1.0f, 0.0f);
		glVertex3f(-1.0f,-1.0f, 0.0f);
	glEnd();
#endif
	if ( ext::opengl::renderModes.size() > 1 ) return;
	ext::opengl::RenderMode::render();
}

void ext::opengl::BaseRenderMode::destroy() {
	ext::opengl::RenderMode::destroy();
}

#endif