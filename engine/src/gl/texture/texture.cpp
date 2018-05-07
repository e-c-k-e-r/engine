#include <uf/gl/texture/texture.h>
#include <uf/utils/io/iostream.h>
// 	C-tors
spec::ogl::Texture::Texture() :
	m_index(0)
{

}
// Move texture
spec::ogl::Texture::Texture( Texture&& move ) :
	m_index(std::move(move.m_index)),
	m_image(std::move(move.m_image))
{

} 				
// Copy texture
spec::ogl::Texture::Texture( const Texture& copy ) :
	m_index(copy.m_index),
	m_image(copy.m_image)
{

} 			
// Move image
spec::ogl::Texture::Texture( Texture::image_t&& move ) :
	m_index(0),
	m_image(std::move(move))
{

} 		
// Copy image
spec::ogl::Texture::Texture( const Texture::image_t& copy ) :
	m_index(0),
	m_image(copy)
{

} 	

// Open image from filename
void spec::ogl::Texture::open( const std::string& filename ) {
	this->m_image.open(filename);
} 	
// 	D-tor
spec::ogl::Texture::~Texture() {
	this->clear();
	this->destroy();
}
void spec::ogl::Texture::clear() {
	this->m_image.clear();
}
void spec::ogl::Texture::destroy() {
	if ( this->m_index ) glDeleteTextures( 1, &this->m_index );
}
void spec::ogl::Texture::bind( uint slot ) const {
	glActiveTexture( GL_TEXTURE0 + slot );
	glBindTexture( GL_TEXTURE_2D, this->m_index );
}
void spec::ogl::Texture::generate() {
	if ( !this->loaded() ) return;
	if ( this->m_index ) this->destroy();
	GLuint type, format = GL_RGBA, internalFormat = GL_RGBA;
	if ( this->m_image.getChannels() == 3 ) format = GL_RGB;
	if ( this->m_image.getChannels() == 4 ) format = GL_RGBA;
	internalFormat = format;
	type = GL_UNSIGNED_BYTE;
	
	glGenTextures( 1, &this->m_index );
	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, this->m_index );
	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		internalFormat,
		this->m_image.getDimensions().x,
		this->m_image.getDimensions().y,
		0,
		format,
		type,
		((uint8_t*)this->m_image.getPixelsPtr())
	);
	
	int mipmap_levels = 8;
	float max_aniso = 0.0f;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_aniso);

	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, max_aniso);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );

	// glTexStorage2D(GL_TEXTURE_2D, mipmap_levels, format == GL_RGBA ? GL_RGBA8 : GL_RGB8, this->m_image.getDimensions().x, this->m_image.getDimensions().y);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, this->m_image.getDimensions().x, this->m_image.getDimensions().y, format, GL_UNSIGNED_BYTE, ((uint8_t*)this->m_image.getPixelsPtr()));
	glGenerateMipmap(GL_TEXTURE_2D);
}
// 	OpenGL Getters
GLuint& spec::ogl::Texture::getIndex() {
	return this->m_index;
}
GLuint spec::ogl::Texture::getIndex() const {
	return this->m_index;
}
GLuint spec::ogl::Texture::getType() const {
	return this->m_index;
}
bool spec::ogl::Texture::loaded() const {
	return this->m_image.getPixelsPtr();
}
bool spec::ogl::Texture::generated() const {
	return this->m_index;
}
spec::ogl::Texture::image_t& spec::ogl::Texture::getImage() {
	return this->m_image;
}
const spec::ogl::Texture::image_t& spec::ogl::Texture::getImage() const {
	return this->m_image;
}