#include <uf/ext/ext.h>

#if UF_ENV_DREAMCAST

#include <uf/ext/opengl/opengl.h>
#include <uf/ext/opengl/commands.h>

int main(int argc, char **argv) {
	ext::opengl::settings::width = 640;
	ext::opengl::settings::height = 480;

	ext::opengl::initialize();

	{
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClearDepth(1.0);
		glDepthFunc(GL_LESS);
		glEnable(GL_DEPTH_TEST);
		glShadeModel(GL_SMOOTH);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(90.0f, (GLfloat) ext::opengl::settings::width / (GLfloat) ext::opengl::settings::height, 0.1f, 100.0f);	// Calculate The Aspect Ratio Of The Window
		glMatrixMode(GL_MODELVIEW);
	}
#if 0
	while ( true ) {
		ext::opengl::tick();
		ext::opengl::render();
	}
#endif
#if 0
	ext::opengl::CommandBuffer commands;

	commands.record([]{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0, 0, ext::opengl::settings::width, ext::opengl::settings::height);	

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(90.0f,(GLfloat) ext::opengl::settings::width / (GLfloat) ext::opengl::settings::height, 0.1f, 100.0f);

		glMatrixMode(GL_MODELVIEW);
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
	while ( true ) {
		commands.submit();
		glKosSwapBuffers();
	}
#endif
#if 1
	while ( true ) {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0, 0, ext::opengl::settings::width, ext::opengl::settings::height);	

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(90.0f,(GLfloat) ext::opengl::settings::width / (GLfloat) ext::opengl::settings::height, 0.1f, 100.0f);

		glMatrixMode(GL_MODELVIEW);
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
		glKosSwapBuffers();
	}
#endif

	ext::opengl::destroy();
	return 0;
}
#endif