#if 0
#include <uf/ext/ext.h>

#include <uf/ext/opengl/opengl.h>
#include <uf/ext/opengl/commands.h>
int main(int argc, char **argv) {
	ext::opengl::settings::width = 640;
	ext::opengl::settings::height = 480;

	ext::opengl::initialize();

	while ( true ) {
		ext::opengl::tick();
		ext::opengl::render();
	}

	ext::opengl::destroy();
	return 0;
}
#endif