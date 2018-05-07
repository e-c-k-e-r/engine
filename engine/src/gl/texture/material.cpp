#include <uf/gl/texture/texture.h>

#include <uf/gl/mesh/mesh.h>

// C-tor
uf::Material::Material() {

}
// D-tor
uf::Material::~Material() {
	this->clear();
	this->destroy();
}
void uf::Material::clear() {
	this->m_diffuse.clear();
	this->m_specular.clear();
	this->m_normal.clear();
}
void uf::Material::destroy() {
	this->m_diffuse.destroy();
	this->m_specular.destroy();
	this->m_normal.destroy();
}

// OpenGL ops
void uf::Material::generate() {
	if ( this->m_diffuse.loaded() && !this->m_diffuse.generated() ) this->m_diffuse.generate();
	if ( this->m_specular.loaded() && !this->m_specular.generated() ) this->m_specular.generate();
	if ( this->m_normal.loaded() && !this->m_normal.generated() ) this->m_normal.generate();
}
void uf::Material::bind( uint diffuse, uint specular, uint normal ) {
	if ( this->m_diffuse.generated() ) this->m_diffuse.bind(diffuse); else diffuse = -1;
	if ( this->m_specular.generated() ) this->m_specular.bind(specular); else specular = -1;
	if ( this->m_normal.generated() ) this->m_normal.bind(normal); else normal = -1;
}
void uf::Material::bind( int& diffuse, int& specular, int& normal ) {
	if ( this->m_diffuse.generated() ) this->m_diffuse.bind(diffuse); else diffuse = -1;
	if ( this->m_specular.generated() ) this->m_specular.bind(specular); else specular = -1;
	if ( this->m_normal.generated() ) this->m_normal.bind(normal); else normal = -1;
}
// OpenGL uf::Material::Getters
bool uf::Material::loaded() const {
	return 	this->m_diffuse.loaded() ||
			this->m_specular.loaded() ||
			this->m_normal.loaded();
}
bool uf::Material::generated() const {
	return 	this->m_diffuse.generated() ||
			this->m_specular.generated() ||
			this->m_normal.generated();
}

/*
// Move Setters
void uf::Material::setDiffuse( uf::Texture&& diffuse ) {
	this->m_diffuse = std::move(diffuse);
}
void uf::Material::setSpecular( uf::Texture&& specular ) {
	this->m_specular = std::move(specular);
}
void uf::Material::setNormal (uf::Texture&& normal ) {
	this->m_normal = std::move(normal);
}
// Copy Setters
void uf::Material::setDiffuse( const uf::Texture& diffuse ) {
	this->m_diffuse = diffuse;
}
void uf::Material::setSpecular( const uf::Texture& specular ) {
	this->m_specular = specular;
}
void uf::Material::setNormal (const uf::Texture& normal ) {
	this->m_normal = normal;
}
*/

// Non-const Getters
uf::Texture& uf::Material::getDiffuse() {
	return this->m_diffuse;
}
uf::Texture& uf::Material::getSpecular() {
	return this->m_specular;
}
uf::Texture& uf::Material::getNormal() {
	return this->m_normal;
}
// Const \Getters
const uf::Texture& uf::Material::getDiffuse() const {
	return this->m_diffuse;
}
const uf::Texture& uf::Material::getSpecular() const {
	return this->m_specular;
}
const uf::Texture& uf::Material::getNormal() const {
	return this->m_normal;
}