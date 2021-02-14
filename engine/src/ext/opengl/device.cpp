#if UF_USE_OPENGL

#include <uf/ext/opengl/opengl.h>
#include <uf/ext/opengl/device.h>
#include <uf/ext/opengl/initializers.h>
#include <uf/utils/window/window.h>
#include <uf/utils/string/ext.h>
#include <uf/ext/openvr/openvr.h>

#include <set>
#include <map>

#include <uf/utils/serialize/serializer.h>

void ext::opengl::Device::initialize() {
	spec::Context::globalInit();
	this->context = (spec::Context*) spec::uni::Context::create( contextSettings, *this->window->getHandle() );

#if UF_ENV_DREAMCAST
	//glKosInit();
	GLdcConfig config;
	glKosInitConfig(&config);
	config.fsaa_enabled = GL_FALSE;
	glKosInitEx(&config);
#else
	glewExperimental = GL_TRUE;
	if ( glewInit() != GLEW_OK ) {
		std::cout << "ERROR: " << errorString() << std::endl;
		std::exit(EXIT_FAILURE);
		return;
	}
#endif
}
void ext::opengl::Device::destroy() {
	if ( this->context ) {
		this->context->terminate();
		delete this->context;
		this->context = NULL;
	}
	spec::Context::globalCleanup();
}

#endif