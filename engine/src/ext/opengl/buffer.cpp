#if UF_USE_OPENGL

#define VMA_IMPLEMENTATION

#include <uf/ext/opengl/buffer.h>
#include <uf/ext/opengl/opengl.h>
#include <uf/ext/opengl/device.h>

void* ext::opengl::Buffer::map( GLsizeiptr size, GLsizeiptr offset ) {
	return NULL;
}
void ext::opengl::Buffer::unmap() {
}
void* ext::opengl::Buffer::map( GLsizeiptr size, GLsizeiptr offset ) const {
	return NULL;
}
void ext::opengl::Buffer::unmap() const {
}

bool ext::opengl::Buffer::bind( GLsizeiptr offset ) {
	return false;
}

void ext::opengl::Buffer::setupDescriptor( GLsizeiptr size, GLsizeiptr offset ) {
	descriptor.buffer = buffer;
	descriptor.offset = offset;
	descriptor.range = size;
}

void ext::opengl::Buffer::copyTo( const void* data, GLsizeiptr len ) const {
	if ( !buffer || !data ) return;
	if ( len >= size ) len = size;
#if !UF_USE_OPENGL_FIXED_FUNCTION
// GPU-bound buffer
	GLenum usage;
	if ( usage & enums::Buffer::STREAM ) {
		if ( usage & enums::Buffer::DRAW ) usage = GL_STREAM_DRAW;
		if ( usage & enums::Buffer::READ ) usage = GL_STREAM_READ;
		if ( usage & enums::Buffer::COPY ) usage = GL_STREAM_COPY;
	}
	if ( usage & enums::Buffer::STATIC ) {
		if ( usage & enums::Buffer::DRAW ) usage = GL_STATIC_DRAW;
		if ( usage & enums::Buffer::READ ) usage = GL_STATIC_READ;
		if ( usage & enums::Buffer::COPY ) usage = GL_STATIC_COPY;
	}
	if ( usage & enums::Buffer::DYNAMIC ) {
		if ( usage & enums::Buffer::DRAW ) usage = GL_DYNAMIC_DRAW;
		if ( usage & enums::Buffer::READ ) usage = GL_DYNAMIC_READ;
		if ( usage & enums::Buffer::COPY ) usage = GL_DYNAMIC_COPY;
	}
	GL_ERROR_CHECK(glBindBuffer(GL_COPY_WRITE_BUFFER, buffer));
	GL_ERROR_CHECK(glBufferData(GL_COPY_WRITE_BUFFER, len, data, usage));
	GL_ERROR_CHECK(glBindBuffer(GL_COPY_WRITE_BUFFER, 0));
#else
// CPU-bound buffer
	void* pointer = device->getBuffer( buffer );
	if ( pointer ) memcpy( pointer, data, len );
#endif
}

bool ext::opengl::Buffer::flush( GLsizeiptr size, GLsizeiptr offset ) {
	return 0;
}
bool ext::opengl::Buffer::invalidate( GLsizeiptr size, GLsizeiptr offset ) {
	return 0;
}

void ext::opengl::Buffer::allocate( const CreateInfo& bufferCreateInfo ) {
	this->destroy();
	if ( !device ) device = &ext::opengl::device;

	this->buffer = device->createBuffer( bufferCreateInfo.usage, bufferCreateInfo.size );
	this->usage = bufferCreateInfo.usage;

	this->size = bufferCreateInfo.size;
	this->allocationInfo.size = bufferCreateInfo.size;
	this->allocationInfo.offset = 0;

	setupDescriptor( bufferCreateInfo.size, 0 );
}

// RAII
ext::opengl::Buffer::~Buffer() {
//	this->destroy();
}
void ext::opengl::Buffer::initialize( Device& device ) {
	this->device = &device;
}
void ext::opengl::Buffer::destroy() {
	if ( device && buffer ) device->destroyBuffer( buffer );
	device = NULL;
	buffer = NULL;
}

//
// Buffers
void ext::opengl::Buffers::initialize( Device& device ) {
	this->device = &device;
}

void ext::opengl::Buffers::destroy() {
	for ( auto& buffer : buffers ) buffer.destroy();
	buffers.clear();
}

size_t ext::opengl::Buffers::initializeBuffer( const void* data, GLsizeiptr length, GLenum usage, bool stage ) {
	size_t index = buffers.size();
	auto& buffer = buffers.emplace_back();
	
	buffer.initialize( *device );
	buffer.allocate( Buffer::CreateInfo{ 
		.flags = 0,
		.size = length,
		.usage = usage,
	} );
	buffer.copyTo( data, length );

	return index;
}
#endif