#include <uf/config.h>
#if defined(UF_USE_GLEW)

#include <uf/gl/gl.h>
#include <uf/ext/ogl/ogl.h>

ext::GL ext::gl;
UF_API_CALL ext::GL::GL() : m_initialized(false) {

}

bool UF_API_CALL ext::GL::initialize() {
	glewExperimental = GL_TRUE;
	return this->m_initialized = glewInit() == GLEW_OK;
}
void UF_API_CALL ext::GL::terminate() {

}
const GLubyte* UF_API_CALL ext::GL::getError() {
	GLenum error;
	const GLubyte* string;
	if ((error = glGetError()) != GL_NO_ERROR) string = gluErrorString(error);
	return string;
}

#endif