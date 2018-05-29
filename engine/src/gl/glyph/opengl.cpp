#include <uf/gl/glyph/glyph.h>

// 	OpenGL ops
void spec::ogl::GlyphTexture::generate( const std::string& font, unsigned long c, uint size ) {
	ext::freetype::Glyph glyph = ext::freetype::initialize(font);
	this->generate( glyph, c );
}
void spec::ogl::GlyphTexture::generate( ext::freetype::Glyph& glyph, unsigned long c, uint size ) {
	ext::freetype::setPixelSizes( glyph, size );
	if ( !ext::freetype::load( glyph, c ) ) return;

	if ( this->m_index ) this->destroy();

	this->setSize( { glyph.face->glyph->bitmap.width, glyph.face->glyph->bitmap.rows } );
	this->setBearing( { glyph.face->glyph->bitmap_left, glyph.face->glyph->bitmap_top } );
	this->setAdvance( {glyph.face->glyph->advance.x, glyph.face->glyph->advance.y} );

	GLuint type, format = GL_RED, internalFormat = GL_RED;
	type = GL_UNSIGNED_BYTE;
	
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glGenTextures( 1, &this->m_index );
	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, this->m_index );
	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		internalFormat,
		this->m_size.x,
		this->m_size.y,
		0,
		format,
		type,
		glyph.face->glyph->bitmap.buffer
	);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}
void spec::ogl::GlyphTexture::generate( const std::string& font, const uf::String& c, uint size ) {
	ext::freetype::Glyph glyph = ext::freetype::initialize(font);
	this->generate( glyph, c );
}
void spec::ogl::GlyphTexture::generate( ext::freetype::Glyph& glyph, const uf::String& c, uint size ) {
	ext::freetype::setPixelSizes( glyph, size );
	if ( !ext::freetype::load( glyph, c ) ) return;

	if ( this->m_index ) this->destroy();

	this->setSize( { glyph.face->glyph->bitmap.width, glyph.face->glyph->bitmap.rows } );
	this->setBearing( { glyph.face->glyph->bitmap_left, glyph.face->glyph->bitmap_top } );
	this->setAdvance( {glyph.face->glyph->advance.x, glyph.face->glyph->advance.y} );

	GLuint type, format = GL_RED, internalFormat = GL_RED;
	type = GL_UNSIGNED_BYTE;
	
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glGenTextures( 1, &this->m_index );
	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, this->m_index );
	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		internalFormat,
		this->m_size.x,
		this->m_size.y,
		0,
		format,
		type,
		glyph.face->glyph->bitmap.buffer
	);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}
// 	Get
const pod::Vector2ui& spec::ogl::GlyphTexture::getSize() const {
	return this->m_size;
}
const pod::Vector2ui& spec::ogl::GlyphTexture::getBearing() const {
	return this->m_bearing;
}
const pod::Vector2i& spec::ogl::GlyphTexture::getAdvance() const {
	return this->m_advance;
}
//	Set
void spec::ogl::GlyphTexture::setSize( const pod::Vector2ui& size ) {
	this->m_size = size;
}
void spec::ogl::GlyphTexture::setBearing( const pod::Vector2ui& bearing ) {
	this->m_bearing = bearing;
}
void spec::ogl::GlyphTexture::setAdvance( const pod::Vector2i& advance ) {
	this->m_advance = advance;
}

// OpenGL ops
void spec::ogl::GlyphMesh::generate() {
	if ( this->generated() ) {
		this->m_vbo.subBuffer();
		return;
	}
	this->m_vao.generate();
	this->m_vao.bind();
	if ( !this->m_vbo.generated() ) this->m_vbo.generate(GL_DYNAMIC_DRAW, false);
	this->bindAttributes();
}
void spec::ogl::GlyphMesh::bindAttributes() {
	GLuint i = 0;
	if ( this->m_vbo.generated() ) {
		this->m_vbo.bindAttribute(i);
		this->m_vao.bindAttribute(i);
	}
}
void spec::ogl::GlyphMesh::render() {
	this->m_vao.render( this->m_vbo );
}
// OpenGL Getters
bool spec::ogl::GlyphMesh::loaded() const {
	return this->m_vbo.loaded();
}
bool spec::ogl::GlyphMesh::generated() const {
	return this->m_vao.generated();
}
// Move Setters
void spec::ogl::GlyphMesh::setPoints(uf::Vertices4f&& points ) {
	this->m_vbo = std::move(points);
}
// Copy Setters
void spec::ogl::GlyphMesh::setPoints( const uf::Vertices4f& points ) {
	this->m_vbo = points;
}
// Non-const Getters
uf::Vertices4f& spec::ogl::GlyphMesh::getPoints() {
	return this->m_vbo;
}
// Const Getters
const uf::Vertices4f& spec::ogl::GlyphMesh::getPoints() const {
	return this->m_vbo;
}