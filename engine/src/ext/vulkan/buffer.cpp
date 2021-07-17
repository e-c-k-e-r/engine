#if UF_USE_VULKAN

#define VMA_IMPLEMENTATION

#include <uf/ext/vulkan/buffer.h>
#include <uf/ext/vulkan/vulkan.h>
#include <uf/ext/vulkan/device.h>

void ext::vulkan::Buffer::aliasBuffer( const ext::vulkan::Buffer& buffer ) {
	*this = {
		.device = NULL,
		.buffer = buffer.buffer,
		.memory = buffer.memory,
		.descriptor = buffer.descriptor,
		.alignment = buffer.alignment,
		.mapped = buffer.mapped,
		.usage = buffer.usage,
		.memoryProperties = buffer.memoryProperties,
		.allocation = buffer.allocation,
		.allocationInfo = buffer.allocationInfo,
	};
}

void* ext::vulkan::Buffer::map( VkDeviceSize size, VkDeviceSize offset ) {
	VK_CHECK_RESULT(vmaMapMemory( allocator, allocation, &mapped ));
	return mapped;
}
void ext::vulkan::Buffer::unmap() {
	if ( !mapped ) return;
	vmaUnmapMemory( allocator, allocation );
	mapped = nullptr;
}
void* ext::vulkan::Buffer::map( VkDeviceSize size, VkDeviceSize offset ) const {
	void* mapped{};
	VK_CHECK_RESULT(vmaMapMemory( allocator, allocation, &mapped ));
	return mapped;
}
void ext::vulkan::Buffer::unmap() const {
	vmaUnmapMemory( allocator, allocation );
}

VkResult ext::vulkan::Buffer::bind( VkDeviceSize offset ) {
	return VK_SUCCESS;
}

void ext::vulkan::Buffer::setupDescriptor( VkDeviceSize size, VkDeviceSize offset ) {
	descriptor.offset = offset;
	descriptor.buffer = buffer;
	descriptor.range = size;
}

void ext::vulkan::Buffer::copyTo( void* data, VkDeviceSize size ) {
	assert(mapped);
	memcpy(mapped, data, size);
}

VkResult ext::vulkan::Buffer::flush( VkDeviceSize size, VkDeviceSize offset ) const {
	return VK_SUCCESS;
}

VkResult ext::vulkan::Buffer::invalidate( VkDeviceSize size, VkDeviceSize offset ) {
	return VK_SUCCESS;
}

void ext::vulkan::Buffer::allocate( VkBufferCreateInfo bufferCreateInfo ) {
	VmaAllocationCreateInfo allocCreateInfo = {};
	
	allocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
	allocCreateInfo.preferredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
	if ( bufferCreateInfo.usage & (VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT) ) {
		allocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	}

	vmaCreateBuffer( allocator, &bufferCreateInfo, &allocCreateInfo, &buffer, &allocation, &allocationInfo );
}

// RAII
ext::vulkan::Buffer::~Buffer() {
//	this->destroy();
}
void ext::vulkan::Buffer::initialize( ext::vulkan::Device& device ) {
	this->device = &device;
}
void ext::vulkan::Buffer::destroy() {
	if ( !device ) return;

	if ( buffer ) {
		vmaDestroyBuffer( allocator, buffer, allocation );
	}

	buffer = nullptr;
	memory = nullptr;
}
void ext::vulkan::Buffer::initialize( const void* data, VkDeviceSize length, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties, bool stage ) {
	if ( !device ) device = &ext::vulkan::device;
	if ( stage ) usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT; // implicitly set properties
	VK_CHECK_RESULT(device->createBuffer(
		usage,
		memoryProperties,
		*this,
		length
	));
	if ( data && length ) update( data, length, stage );
}
void ext::vulkan::Buffer::update( const void* data, VkDeviceSize length, bool stage ) const {
	if ( !data || !length ) return;

	if ( length > allocationInfo.size ) {
		UF_MSG_DEBUG("LENGTH OF " << length << " EXCEEDS BUFFER SIZE " << allocationInfo.size );
		Buffer& b = *const_cast<Buffer*>(this);
		b.destroy();
		b.initialize( data, length, usage, memoryProperties, stage );
		return;
	}
	if ( !stage ) {
		void* map = this->map();
		memcpy(map, data, length);
		if ((this->memoryProperties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0) this->flush();
		this->unmap();
		return;
	}

	Buffer staging;

	ext::vulkan::Device* device = this->device ? this->device : &ext::vulkan::device;
	device->createBuffer(
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		staging,
		length,
		data
	);

	// Copy to staging buffer
	VkCommandBuffer copyCommand = device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
	VkBufferCopy region = {};
	region.size = length;
	vkCmdCopyBuffer(copyCommand, staging.buffer, buffer, 1, &region);

	device->flushCommandBuffer(copyCommand, true);
	staging.destroy();
}

//
// Buffers
void ext::vulkan::Buffers::initialize( Device& device ) {
	this->device = &device;
}
/*
ext::vulkan::Buffers::~Buffers() {
	for ( auto& buffer : buffers ) buffer.destroy();
	buffers.clear();
}
*/
void ext::vulkan::Buffers::destroy() {
	for ( auto& buffer : buffers ) if ( buffer.device ) buffer.destroy();
	buffers.clear();
}

size_t ext::vulkan::Buffers::initializeBuffer( const void* data, VkDeviceSize length, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties, bool stage ) {
	size_t index = buffers.size();
	auto& buffer = buffers.emplace_back();
	buffer.initialize( *device );
	buffer.initialize( data, length, usage, memoryProperties, stage );
	return index;
}
void ext::vulkan::Buffers::updateBuffer( const void* data, VkDeviceSize length, const Buffer& buffer, bool stage ) const {
	buffer.update( data, length, stage );
}

#endif