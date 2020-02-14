#if UF_USE_OPENGL

#define VMA_IMPLEMENTATION

#include <uf/ext/opengl/buffer.h>
#include <uf/ext/opengl/opengl.h>
#include <uf/ext/opengl/device.h>

GLhandle(VkResult) ext::opengl::Buffer::map( GLsizeiptr size, GLsizeiptr offset ) {
	return 0;
}

void ext::opengl::Buffer::unmap() {
}

GLhandle(VkResult) ext::opengl::Buffer::bind( GLsizeiptr offset ) {
	return 0;
}

void ext::opengl::Buffer::setupDescriptor( GLsizeiptr size, GLsizeiptr offset ) {
}

void ext::opengl::Buffer::copyTo( void* data, GLsizeiptr size ) {

}

GLhandle(VkResult) ext::opengl::Buffer::flush( GLsizeiptr size, GLsizeiptr offset ) {
	return 0;
}


GLhandle(VkResult) ext::opengl::Buffer::invalidate( GLsizeiptr size, GLsizeiptr offset ) {
	return 0;
}

void ext::opengl::Buffer::allocate( GLhandle(VkBufferCreateInfo) bufferCreateInfo ) {

}

// RAII
ext::opengl::Buffer::~Buffer() {
//	this->destroy();
}
void ext::opengl::Buffer::initialize( GLhandle(VkDevice) device ) {
	this->device = device;
}
void ext::opengl::Buffer::destroy() {

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

size_t ext::opengl::Buffers::initializeBuffer( void* data, GLsizeiptr length, GLhandle(VkBufferUsageFlags) usageFlags, GLhandle(VkMemoryPropertyFlags) memoryPropertyFlags, bool stage ) {
	return 0;
}
void ext::opengl::Buffers::updateBuffer( void* data, GLsizeiptr length, size_t index, bool stage ) {
	Buffer& buffer = buffers.at(index);
	return updateBuffer( data, length, buffer, stage );
}
void ext::opengl::Buffers::updateBuffer( void* data, GLsizeiptr length, Buffer& buffer, bool stage ) {
}

#endif