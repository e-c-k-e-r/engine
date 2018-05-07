#include <uf/gl/gbuffer/gbuffer.h>

#include <uf/gl/mesh/mesh.h>
#include <uf/gl/shader/shader.h>
#include <uf/gl/texture/texture.h>
#include <uf/utils/io/iostream.h>
#include <uf/utils/math/transform.h>
// 	C-tors
spec::ogl::GeometryTexture::GeometryTexture() {

}
// Move texture
spec::ogl::GeometryTexture::GeometryTexture( GeometryTexture&& move ) :
	m_size(std::move(move.m_size)),
	m_type(std::move(move.m_type))
{

} 				
// Copy texture
spec::ogl::GeometryTexture::GeometryTexture( const GeometryTexture& copy ) :
	m_size(copy.m_size),
	m_type(copy.m_type)
{

} 
void spec::ogl::GeometryTexture::bind( uint slot ) const {
	glActiveTexture( GL_TEXTURE0 + slot );
	glBindTexture( GL_TEXTURE_2D, this->m_index );
}
void spec::ogl::GeometryTexture::generate() {
	if ( this->m_index ) this->destroy();
	if ( this->m_size.x == 0 && this->m_size.y == 0 ) return;
	glGenTextures(1, &this->m_index);
	glBindTexture(GL_TEXTURE_2D, this->m_index);
	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		this->m_type.internal,
		this->m_size.x,
		this->m_size.y,
		0,
		this->m_type.format,
		this->m_type.type,
		NULL
	);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, this->m_type.attachment, GL_TEXTURE_2D, this->m_index, 0);
}
const pod::Vector2ui&  spec::ogl::GeometryTexture::getSize() const { return this->m_size; }
spec::ogl::GeometryTexture::type_t  spec::ogl::GeometryTexture::getType() const { return this->m_type; }
const std::string& spec::ogl::GeometryTexture::getName() const { return this->m_name;}
void spec::ogl::GeometryTexture::setSize( const pod::Vector2ui& size ) { this->m_size = size; }
void spec::ogl::GeometryTexture::setType( spec::ogl::GeometryTexture::type_t type ) { this->m_type = type; }
void spec::ogl::GeometryTexture::setName( const std::string& name ) { this->m_name = name; }

spec::ogl::GeometryBuffer::GeometryBuffer() :
	m_index(0),
	m_size({0,0})
{

}
spec::ogl::GeometryBuffer::GeometryBuffer( GeometryBuffer&& move ) :
	m_index(std::move(move.m_index)),
	m_size(std::move(move.m_size)),
	m_buffers(std::move(move.m_buffers))
{

}
spec::ogl::GeometryBuffer::GeometryBuffer( const GeometryBuffer& copy ) :
	m_index(copy.m_index),
	m_size(copy.m_size),
	m_buffers(copy.m_buffers)
{		

}

spec::ogl::GeometryBuffer::~GeometryBuffer() {
	this->destroy();
}

void spec::ogl::GeometryBuffer::generate( bool defaultConfig ) {
	if ( this->m_index ) this->destroy();

	glGenFramebuffers(1, &this->m_index);
	glBindFramebuffer(GL_FRAMEBUFFER, this->m_index);

	std::vector<uint> attachments;
	for ( auto& texture : this->m_buffers ) {
		attachments.push_back(texture.getType().attachment);
		texture.generate();
	}

	if ( attachments.size() - 1 > 0 )
		glDrawBuffers(attachments.size() - 1, &attachments[0]);
	else {
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
	}

//	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) std::cout << "Framebuffer not complete!" << std::endl;
	GLenum status = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	if ( status != GL_FRAMEBUFFER_COMPLETE ) {
		std::string ERR_TYPE = "???";
		switch ( status ) {
			case GL_FRAMEBUFFER_UNDEFINED: ERR_TYPE = "GL_FRAMEBUFFER_UNDEFINED"; break;
			case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT : ERR_TYPE = "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT"; break;
			case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT : ERR_TYPE = "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT"; break;
			case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER : ERR_TYPE = "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER"; break;
			case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER : ERR_TYPE = "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER"; break;
			case GL_FRAMEBUFFER_UNSUPPORTED : ERR_TYPE = "GL_FRAMEBUFFER_UNSUPPORTED"; break;
			case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE : ERR_TYPE = "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE"; break;
			case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS : ERR_TYPE = "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS"; break;
		}
		uf::iostream << "FBO error: " << ERR_TYPE << "\n";
	}

	// Render plane
	uf::Mesh& mesh = this->getComponent<uf::Mesh>();
	std::vector<float> vertices = {
		-1.0f, 1.0f, 0.0f,
		 1.0f, 1.0f, 0.0f,
		 1.0f,-1.0f, 0.0f,
		 1.0f,-1.0f, 0.0f,
		-1.0f,-1.0f, 0.0f,
		-1.0f, 1.0f, 0.0f,
	};
	std::vector<float> uvs = {
		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,

		1.0f, 0.0f,
		0.0f, 0.0f,
		0.0f, 1.0f,
	};
	
	mesh.getPositions().getVertices() = vertices;
	mesh.getUvs().getVertices() = uvs;

	mesh.index();
	mesh.generate();
}
void spec::ogl::GeometryBuffer::bind() const { 
	if ( !this->m_index ) return;
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->m_index);
}
void spec::ogl::GeometryBuffer::unbind() const { glBindFramebuffer(GL_FRAMEBUFFER, 0); }
void spec::ogl::GeometryBuffer::destroy() {
	if ( this->m_index ) {
		glDeleteFramebuffers( 1, &this->m_index );
	}
	for ( auto& texture : this->m_buffers ) {
		texture.destroy();
	}
}
void spec::ogl::GeometryBuffer::copy() const {

}
void spec::ogl::GeometryBuffer::render() {

}

spec::ogl::GeometryBuffer::type_t& spec::ogl::GeometryBuffer::getIndex() { return this->m_index; }
spec::ogl::GeometryBuffer::type_t spec::ogl::GeometryBuffer::getIndex() const { return this->m_index; }
pod::Vector2ui& spec::ogl::GeometryBuffer::getSize() { return this->m_size; }
const pod::Vector2ui& spec::ogl::GeometryBuffer::getSize() const { return this->m_size; }
void spec::ogl::GeometryBuffer::setSize( const pod::Vector2ui& size ) { this->m_size = size; }

spec::ogl::GeometryBuffer::container_t& spec::ogl::GeometryBuffer::getBuffers() { return this->m_buffers; }