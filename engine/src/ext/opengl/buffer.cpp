#if UF_USE_OPENGL

#define VMA_IMPLEMENTATION

#include <uf/ext/opengl/buffer.h>
#include <uf/ext/opengl/opengl.h>
#include <uf/ext/opengl/device.h>

bool ext::opengl::Buffer::map( GLsizeiptr size, GLsizeiptr offset ) {
	return 0;
}
void ext::opengl::Buffer::unmap() {
}

bool ext::opengl::Buffer::bind( GLsizeiptr offset ) {
	return 0;
}

void ext::opengl::Buffer::setupDescriptor( GLsizeiptr size, GLsizeiptr offset ) {
	descriptor.buffer = buffer;
	descriptor.offset = offset;
	descriptor.range = size;
}

void ext::opengl::Buffer::copyTo( void* data, GLsizeiptr len ) {
	if ( !buffer || !data ) return;
	if ( len >= size ) len = size;
	memcpy( buffer, data, len );
}

bool ext::opengl::Buffer::flush( GLsizeiptr size, GLsizeiptr offset ) {
	return 0;
}
bool ext::opengl::Buffer::invalidate( GLsizeiptr size, GLsizeiptr offset ) {
	return 0;
}

void ext::opengl::Buffer::allocate( const CreateInfo& bufferCreateInfo ) {
	this->destroy();
	
	this->buffer = malloc( bufferCreateInfo.size );
	this->usageFlags = bufferCreateInfo.usage;

	this->size = bufferCreateInfo.size;
	this->allocationInfo.size = bufferCreateInfo.size;
	this->allocationInfo.offset = 0;
}

// RAII
ext::opengl::Buffer::~Buffer() {
//	this->destroy();
}
void ext::opengl::Buffer::initialize( Device& device ) {
	this->device = &device;
}
void ext::opengl::Buffer::destroy() {
	if ( device && buffer ) free(buffer);
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

size_t ext::opengl::Buffers::initializeBuffer( void* data, GLsizeiptr length, GLenum usageFlags, GLhandle(VkMemoryPropertyFlags) memoryPropertyFlags, bool stage ) {
	size_t index = buffers.size();
	auto& buffer = buffers.emplace_back();
	
	buffer.allocate( Buffer::CreateInfo{ 
		.flags = 0,
		.size = length,
		.usage = usageFlags,
	} );
	buffer.copyTo( data, length );

	return index;
}
void ext::opengl::Buffers::updateBuffer( void* data, GLsizeiptr length, size_t index, bool stage ) {
	Buffer& buffer = buffers.at(index);
	return updateBuffer( data, length, buffer, stage );
}
void ext::opengl::Buffers::updateBuffer( void* data, GLsizeiptr length, Buffer& buffer, bool stage ) {
	buffer.copyTo( data, length );
}

#endif