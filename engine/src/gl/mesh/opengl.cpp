#include <uf/gl/mesh/mesh.h>
#if defined(UF_USE_OPENGL)

spec::ogl::Vao::Vao() :
	m_index(0)
{

}
spec::ogl::Vao::~Vao() {
	this->clear();
	this->destroy();
}

void spec::ogl::Vao::generate() {
// 	OpenGL >= 3.0
	this->destroy();
	glGenVertexArrays(1, &this->m_index);
}
void spec::ogl::Vao::bind() {
// 	OpenGL >= 3.0
	glBindVertexArray(this->m_index);
}
#include <algorithm>
void spec::ogl::Vao::bindAttribute( GLuint i ) {
	if ( std::find(this->m_indexes.begin(), this->m_indexes.end(), i) == this->m_indexes.end() )
		this->m_indexes.emplace_back(i);
}
void spec::ogl::Vao::bindAttribute( GLuint i, bool enable ) {
// 	OpenGL >= 2.0
	this->bindAttribute(i);
	enable ? glEnableVertexAttribArray(i) : glDisableVertexAttribArray(i);
}
void spec::ogl::Vao::bindAttributes( bool enable ) {
	for ( auto i : this->m_indexes )
		this->bindAttribute(i, enable);
}
void spec::ogl::Vao::render( uf::Vertices<>::base_t* vbo ) {
	this->bind();
	this->bindAttributes(true);
	vbo->render();
	this->bindAttributes(false);
}
void spec::ogl::Vao::clear() {
// 	OpenGL >= 3.0
	this->m_indexes.clear();
}
void spec::ogl::Vao::destroy() {
// 	OpenGL >= 3.0
	if ( this->m_index ) glDeleteVertexArrays(1, &this->m_index);
}

GLuint& spec::ogl::Vao::getIndex() {
	return this->m_index;
}
GLuint spec::ogl::Vao::getIndex() const {
	return this->m_index;
}
bool spec::ogl::Vao::generated() const {
	return this->m_index;
}

#endif