#include <uf/gl/mesh/mesh.h>

// uint uf::Mesh::MAX_BONES = 64;

// C-tor
uf::Mesh::Mesh() :
	m_indexed(false)
{

}
uf::Mesh::Mesh( uf::Mesh&& mesh ) { this->move(mesh); }
uf::Mesh::Mesh( const uf::Mesh& mesh ) { this->copy(mesh); }

uf::Mesh& uf::Mesh::operator=( uf::Mesh&& mesh ) { this->move(mesh); return *this; }
uf::Mesh& uf::Mesh::operator=( const uf::Mesh& mesh ) { this->clear(); this->destroy(); this->copy(mesh); return *this; }
// D-tor
uf::Mesh::~Mesh() {
	this->clear();
	this->destroy();
}
void uf::Mesh::clear() {
	this->m_vao.clear();
	this->m_ibo.clear();
	this->m_position.clear();
	this->m_normal.clear();
	this->m_color.clear();
	this->m_uv.clear();
}
void uf::Mesh::destroy() {
	this->m_vao.destroy();
	this->m_ibo.destroy();
	this->m_position.destroy();
	this->m_normal.destroy();
	this->m_color.destroy();
	this->m_uv.destroy();
}

// OpenGL ops
void uf::Mesh::generate() {
	this->m_vao.generate();
	this->m_vao.bind();

	if ( this->m_position.loaded() && !this->m_position.generated() ) this->m_position.generate();
	if ( this->m_uv.loaded() && !this->m_uv.generated() ) this->m_uv.generate();
	if ( this->m_normal.loaded() && !this->m_normal.generated() ) this->m_normal.generate();
	if ( this->m_color.loaded() && !this->m_color.generated() ) this->m_color.generate();
	this->bindAttributes();
	
	if ( this->m_indexed ) this->m_ibo.generate();
}
void uf::Mesh::bindAttributes() {
	GLuint i = 0;
	if ( this->m_position.generated() ) {
		this->m_position.bindAttribute(i);
		this->m_vao.bindAttribute(i);
		++i;
	}
	if ( this->m_uv.generated() ) {
		this->m_uv.bindAttribute(i);
		this->m_vao.bindAttribute(i);
		++i;
	}
	if ( this->m_normal.generated() ) {
		this->m_normal.bindAttribute(i);
		this->m_vao.bindAttribute(i);
		++i;
	}
	if ( this->m_color.generated() ) {
		this->m_color.bindAttribute(i);
		this->m_vao.bindAttribute(i);
		++i;
	}
}
void uf::Mesh::render() {
	this->m_indexed ? this->m_vao.render(this->m_ibo) : this->m_vao.render( this->m_position );
}
// OpenGL uf::Mesh::Getters
bool uf::Mesh::loaded() const {
	return 	this->m_position.loaded() ||
			this->m_normal.loaded() ||
			this->m_color.loaded() ||
			this->m_uv.loaded();
}
bool uf::Mesh::generated() const {
	return this->m_vao.generated();
}

// Indexed ops
void uf::Mesh::index() {
	if ( this->m_indexed ) return;
	this->m_indexed = true;
	this->m_ibo.index( this->m_position, this->m_normal, this->m_color, this->m_uv );
}
// Pseudo-Move/Copy
void uf::Mesh::move( uf::Mesh& mesh ) { // moves GL object
	this->destroy();

	this->m_vao = std::move( mesh.m_vao );
	this->m_ibo = std::move( mesh.m_ibo );
	this->m_indexed = std::move( mesh.m_indexed );
}
void uf::Mesh::copy( const uf::Mesh& mesh ) { // copies vertices
	this->setPositions( mesh.getPositions() );
	this->setNormals( mesh.getNormals() );
	this->setColors( mesh.getColors() );
	this->setUvs( mesh.getUvs() );
}
void uf::Mesh::insert( const uf::Mesh& mesh ) { // inserts vertices
	this->m_position.getVertices().insert( this->m_position.getVertices().end(), mesh.getPositions().getVertices().cbegin(), mesh.getPositions().getVertices().cend() );
	this->m_normal.getVertices().insert( this->m_normal.getVertices().end(), mesh.getNormals().getVertices().cbegin(), mesh.getNormals().getVertices().cend() );
	this->m_color.getVertices().insert( this->m_color.getVertices().end(), mesh.getColors().getVertices().cbegin(), mesh.getColors().getVertices().cend() );
	this->m_uv.getVertices().insert( this->m_uv.getVertices().end(), mesh.getUvs().getVertices().cbegin(), mesh.getUvs().getVertices().cend() );
}
// Move Setters
void uf::Mesh::setPositions( uf::Vertices3f&& position ) {
	this->m_position = std::move(position);
}
void uf::Mesh::setNormals( uf::Vertices3f&& normal ) {
	this->m_normal = std::move(normal);
}
void uf::Mesh::setColors (uf::Vertices4f&& color ) {
	this->m_color = std::move(color);
}
void uf::Mesh::setUvs( uf::Vertices2f&& uv ) {
	this->m_uv = std::move(uv);
}
// Copy Setters
void uf::Mesh::setPositions( const uf::Vertices3f& position ) {
	this->m_position = position;
}
void uf::Mesh::setNormals( const uf::Vertices3f& normal ) {
	this->m_normal = normal;
}
void uf::Mesh::setColors (const uf::Vertices4f& color ) {
	this->m_color = color;
}
void uf::Mesh::setUvs( const uf::Vertices2f& uv ) {
	this->m_uv = uv;
}

// Non-const Getters
uf::Vertices3f& uf::Mesh::getPositions() {
	return this->m_position;
}
uf::Vertices3f& uf::Mesh::getNormals() {
	return this->m_normal;
}
uf::Vertices4f& uf::Mesh::getColors() {
	return this->m_color;
}
uf::Vertices2f& uf::Mesh::getUvs() {
	return this->m_uv;
}
// Const \Getters
const uf::Vertices3f& uf::Mesh::getPositions() const {
	return this->m_position;
}
const uf::Vertices3f& uf::Mesh::getNormals() const {
	return this->m_normal;
}
const uf::Vertices4f& uf::Mesh::getColors() const {
	return this->m_color;
}
const uf::Vertices2f& uf::Mesh::getUvs() const {
	return this->m_uv;
}