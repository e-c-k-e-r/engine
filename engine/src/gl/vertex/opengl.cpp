#include <uf/gl/vertex/vertex.h>
#if defined(UF_USE_OPENGL)

spec::ogl::Vbo::Vbo( GLuint type, std::size_t dimensions ) :
	m_index(0),
	m_type(type),
	m_attribute(0),
	m_dimensions(dimensions)
{
	
}
spec::ogl::Vbo::Vbo( Vbo&& move ) : Vbo() {
	this->m_index = std::move(move.m_index);
	this->m_type = std::move(move.m_type);
	this->m_attribute = std::move(move.m_attribute);
	this->m_dimensions = std::move(move.m_dimensions);
}
spec::ogl::Vbo::Vbo( const Vbo& copy ) {
	this->m_index = copy.m_index;
	this->m_type = copy.m_type;
	this->m_attribute = copy.m_attribute;
	this->m_dimensions = copy.m_dimensions;
}
spec::ogl::Vbo::~Vbo() {
	this->clear();
	this->destroy();
}

void spec::ogl::Vbo::bind() const {
// 	OpenGL >= 1.5
	glBindBuffer( this->m_type, this->m_index );
}
void spec::ogl::Vbo::setType( GLuint type ) {
	this->m_type = type;
}
void spec::ogl::Vbo::bindAttribute( bool enable ) {
// 	OpenGL >= 2.0
	enable ? glEnableVertexAttribArray(this->m_attribute) : glDisableVertexAttribArray(this->m_attribute);
}
void spec::ogl::Vbo::bindAttribute( GLuint attribute, GLuint dimensions, GLboolean normalize, GLuint stride, void* offset ) {
// 	OpenGL >= 2.0
	GLuint type = this->deduceType();
	this->m_attribute = attribute;
	if ( dimensions == 0 ) dimensions = this->m_dimensions;

	this->bind();
	this->bindAttribute(true);
	if ( type == GL_UNSIGNED_INT || type == GL_INT ) {
		glVertexAttribIPointer(attribute, dimensions, type, stride, offset);
	} else glVertexAttribPointer(attribute, dimensions, type, normalize, stride, offset);
	this->bindAttribute(false);
}
void spec::ogl::Vbo::clear() {
//	if ( this->generated() ) glDeleteBuffers( 1, &this->m_index );
}
void spec::ogl::Vbo::destroy() {
	if ( this->generated() ) glDeleteBuffers( 1, &this->m_index );
}

GLuint& spec::ogl::Vbo::getIndex() {
	return this->m_index;
}
GLuint spec::ogl::Vbo::getIndex() const {
	return this->m_index;
}
GLuint spec::ogl::Vbo::getType() const {
	return this->m_type;
}
bool spec::ogl::Vbo::generated() const {
	return this->m_index;
}
void spec::ogl::Vbo::render( GLuint mode, std::size_t start, std::size_t end ) const {
	glDrawArrays( mode, start, end );
}

spec::ogl::Vbo& spec::ogl::Vbo::operator=( Vbo&& move ) {
	this->m_index = std::move(move.m_index);
	this->m_type = std::move(move.m_type);
	this->m_attribute = std::move(move.m_attribute);
	this->m_dimensions = std::move(move.m_dimensions);
	return *this;
}
spec::ogl::Vbo& spec::ogl::Vbo::operator=( const Vbo& copy ) {
	this->m_index = copy.m_index;
	this->m_type = copy.m_type;
	this->m_attribute = copy.m_attribute;
	this->m_dimensions = copy.m_dimensions;
	return *this;
}

#endif