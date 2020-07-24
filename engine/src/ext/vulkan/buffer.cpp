#define VMA_IMPLEMENTATION

#include <uf/ext/vulkan/buffer.h>
#include <uf/ext/vulkan/vulkan.h>

VkResult ext::vulkan::Buffer::map( VkDeviceSize size, VkDeviceSize offset ) {
//	return vkMapMemory( device, memory, offset, size, 0, &mapped );
	return vmaMapMemory( allocator, allocation, &mapped );
}

/**
* Unmap a mapped memory range
*
* @note Does not return a result as vkUnmapMemory can't fail
*/
void ext::vulkan::Buffer::unmap() {
	if ( !mapped ) return;
//	vkUnmapMemory( device, memory );
	vmaUnmapMemory( allocator, allocation );
	mapped = nullptr;
}

/** 
* Attach the allocated memory block to the buffer
* 
* @param offset (Optional) Byte offset (from the beginning) for the memory region to bind
* 
* @return VkResult of the bindBufferMemory call
*/
VkResult ext::vulkan::Buffer::bind( VkDeviceSize offset ) {
//	return vkBindBufferMemory( device, buffer, memory, offset );
//	return vmaBindBufferMemory( allocator, allocation, buffer );
	return VK_SUCCESS;
}

/**
* Setup the default descriptor for this buffer
*
* @param size (Optional) Size of the memory range of the descriptor
* @param offset (Optional) Byte offset from beginning
*
*/
void ext::vulkan::Buffer::setupDescriptor( VkDeviceSize size, VkDeviceSize offset ) {
	descriptor.offset = offset;
	descriptor.buffer = buffer;
	descriptor.range = size;
}

/**
* Copies the specified data to the mapped buffer
* 
* @param data Pointer to the data to copy
* @param size Size of the data to copy in machine units
*
*/
void ext::vulkan::Buffer::copyTo( void* data, VkDeviceSize size ) {
	assert(mapped);
	memcpy(mapped, data, size);
}

/** 
* Flush a memory range of the buffer to make it visible to the device
*
* @note Only required for non-coherent memory
*
* @param size (Optional) Size of the memory range to flush. Pass VK_WHOLE_SIZE to flush the complete buffer range.
* @param offset (Optional) Byte offset from beginning
*
* @return VkResult of the flush call
*/
VkResult ext::vulkan::Buffer::flush( VkDeviceSize size, VkDeviceSize offset ) {
/*
	VkMappedMemoryRange mappedRange = {};
	mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	mappedRange.memory = memory;
	mappedRange.offset = offset;
	mappedRange.size = size;
	return vkFlushMappedMemoryRanges(device, 1, &mappedRange);
*/
	return VK_SUCCESS;
}

/**
* Invalidate a memory range of the buffer to make it visible to the host
*
* @note Only required for non-coherent memory
*
* @param size (Optional) Size of the memory range to invalidate. Pass VK_WHOLE_SIZE to invalidate the complete buffer range.
* @param offset (Optional) Byte offset from beginning
*
* @return VkResult of the invalidate call
*/
VkResult ext::vulkan::Buffer::invalidate( VkDeviceSize size, VkDeviceSize offset ) {
/*
	VkMappedMemoryRange mappedRange = {};
	mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	mappedRange.memory = memory;
	mappedRange.offset = offset;
	mappedRange.size = size;
	return vkInvalidateMappedMemoryRanges(device, 1, &mappedRange);
*/
	return VK_SUCCESS;
}

void ext::vulkan::Buffer::allocate( VkBufferCreateInfo bufferCreateInfo ) {
//	VK_CHECK_RESULT(vkCreateBuffer( device, &bufferCreateInfo, nullptr, &buffer));
	VmaAllocationCreateInfo allocCreateInfo = {};
	
	allocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
	allocCreateInfo.preferredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
	if ( bufferCreateInfo.usage & (VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT) ) {
		allocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	}

	vmaCreateBuffer( allocator, &bufferCreateInfo, &allocCreateInfo, &buffer, &allocation, &allocationInfo );
//	VkMemoryPropertyFlags memFlags;
//	vmaGetMemoryTypeProperties(allocator, allocationInfo.memoryType, &memFlags);
//	memory = allocationInfo.deviceMemory;
	size = allocationInfo.size;
}

// RAII
ext::vulkan::Buffer::~Buffer() {
//	this->destroy();
}
void ext::vulkan::Buffer::initialize( VkDevice device ) {
	this->device = device;
}
void ext::vulkan::Buffer::destroy() {
	if ( buffer ) {
		vmaDestroyBuffer( allocator, buffer, allocation );
//		vkDestroyBuffer(device, buffer, nullptr);
	}

	if ( memory ) {
//		vkFreeMemory(device, memory, nullptr);
	}

	buffer = nullptr;
	memory = nullptr;
}