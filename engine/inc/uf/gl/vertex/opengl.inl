template<typename T, std::size_t N>
spec::ogl::Vertices<T,N>::Vertices( GLuint type ) : Vbo(type, N) {
}
template<typename T, std::size_t N>
spec::ogl::Vertices<T,N>::Vertices( const typename Vertices<T,N>::vector_t::container_t array, std::size_t len, GLuint type ) : Vertices( type ) {
	this->add( array, len );
}
template<typename T, std::size_t N>
spec::ogl::Vertices<T,N>::Vertices( typename Vertices<T,N>::vectors_t&& vectors, GLuint type ) : 
	m_vertices(std::move(vectors)),
	Vbo(type, N)
{
//	this->m_vertices = std::move(vectors);
}
template<typename T, std::size_t N>
spec::ogl::Vertices<T,N>::Vertices( const typename Vertices<T,N>::vectors_t& vectors, GLuint type ) : Vertices( type ) {
	this->add( vectors );
}
template<typename T, std::size_t N>
spec::ogl::Vertices<T,N>::Vertices( Vertices<T,N>&& move ) : Vbo( (Vbo&&) move ) {
	this->m_vertices = std::move(move.m_vertices);
}
template<typename T, std::size_t N>
spec::ogl::Vertices<T,N>::Vertices( const Vertices<T,N>& copy ) : Vbo( (const Vbo&) copy ) {
	this->m_vertices = copy.m_vertices;
}
//	~Vertices();

template<typename T, std::size_t N>
void spec::ogl::Vertices<T,N>::add( const typename Vertices<T,N>::vector_t::container_t array, std::size_t len ) {
	this->m_vertices.insert( this->m_vertices.end(), array, array + len );
}
template<typename T, std::size_t N>
void spec::ogl::Vertices<T,N>::add( const typename Vertices<T,N>::vectors_t& vectors ) {
	this->m_vertices.insert( this->m_vertices.end(), vectors.cbegin(), vectors.cend() );
}
template<typename T, std::size_t N>
void spec::ogl::Vertices<T,N>::add( const typename Vertices<T,N>::vector_t& vector ) {
//	this->add( vector.get(), vector.size );
	this->m_vertices.insert( this->m_vertices.end(), vector.get(), vector.get() + vector.size );
}
template<typename T, std::size_t N>
void spec::ogl::Vertices<T,N>::set( typename Vertices<T,N>::vectors_t&& vectors ) {
	this->m_vertices = std::move(vectors);
}
template<typename T, std::size_t N>
void spec::ogl::Vertices<T,N>::clear() {
	this->m_vertices.clear();
}

template<typename T, std::size_t N>
typename spec::ogl::Vertices<T,N>::container_t& spec::ogl::Vertices<T,N>::getVertices() {
	return this->m_vertices;
}
template<typename T, std::size_t N>
const typename spec::ogl::Vertices<T,N>::container_t& spec::ogl::Vertices<T,N>::getVertices() const {
	return this->m_vertices;
}
template<typename T, std::size_t N>
std::size_t spec::ogl::Vertices<T,N>::verticesCount() const {
	return this->m_vertices.size() / N;
}

template<typename T, std::size_t N>
bool spec::ogl::Vertices<T,N>::loaded() const {
	return !this->m_vertices.empty();
}
#include <uf/utils/io/iostream.h>
template<typename T, std::size_t N>
void spec::ogl::Vertices<T,N>::generate() {
//	OpenGL >= 1.5
	if ( !this->loaded() ) return;
	if ( this->generated() ) glDeleteBuffers( 1, &this->m_index );

	glGenBuffers( 1, &this->m_index );
	glBindBuffer( this->m_type, this->m_index );
	glBufferData( this->m_type, this->m_vertices.size() * sizeof(T), &this->m_vertices[0], GL_STATIC_DRAW );
}
template<typename T, std::size_t N>
void spec::ogl::Vertices<T,N>::render( GLuint mode, std::size_t start, std::size_t end ) const {
	Vbo::render( mode, start, (!end ? this->verticesCount() : end) );
}

#include <typeinfo>
template<typename T, std::size_t N>
GLuint spec::ogl::Vertices<T,N>::deduceType() const {
	static GLuint type = 0;
	if ( type == 0 ) {	
		static const std::string type_t = typeid(T).name();
		static const std::string byte_t = typeid(char).name();
		static const std::string ubyte_t = typeid(unsigned char).name();
		static const std::string short_t = typeid(short).name();
		static const std::string ushort_t = typeid(unsigned short).name();
		static const std::string int_t = typeid(int).name();
		static const std::string fix_t = typeid(int).name();
		static const std::string uint_t = typeid(unsigned int).name();
		static const std::string float_t = typeid(float).name();

		if ( type_t == byte_t ) 		type = GL_BYTE;
		else if ( type_t == ubyte_t ) 	type = GL_UNSIGNED_BYTE;
		else if ( type_t == short_t ) 	type = GL_SHORT;
		else if ( type_t == ushort_t ) 	type = GL_UNSIGNED_SHORT;
		else if ( type_t == int_t ) 	type = GL_INT;
		else if ( type_t == fix_t ) 	type = GL_FIXED;
		else if ( type_t == uint_t ) 	type = GL_UNSIGNED_INT;
		else if ( type_t == float_t ) 	type = GL_FLOAT;
		else 							type = GL_FLOAT;
	}
	return type;
}

template<typename T, std::size_t N>
spec::ogl::Vertices<T,N>& spec::ogl::Vertices<T,N>::operator=( Vertices<T,N>&& move ) {
	Vbo::operator=(move);
	this->m_vertices = std::move(move.m_vertices);
	return *this;
}
template<typename T, std::size_t N>
spec::ogl::Vertices<T,N>& spec::ogl::Vertices<T,N>::operator=( const Vertices<T,N>& copy ) {
	Vbo::operator=(copy);
	this->m_vertices = copy.m_vertices;
	return *this;
}