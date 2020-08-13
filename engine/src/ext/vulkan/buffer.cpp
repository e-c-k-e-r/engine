#define VMA_IMPLEMENTATION

#include <uf/ext/vulkan/buffer.h>
#include <uf/ext/vulkan/vulkan.h>
#include <uf/ext/vulkan/device.h>

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
	for ( auto& buffer : buffers ) buffer.destroy();
	buffers.clear();
}

size_t ext::vulkan::Buffers::initializeBuffer( void* data, VkDeviceSize length, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, bool stage ) {
	Buffer buffer;
	size_t index = buffers.size();
//	Buffer& buffer = buffers.emplace_back();
	
	// Stage
	if ( !stage ) {
		VK_CHECK_RESULT(device->createBuffer(
			usageFlags,
			memoryPropertyFlags,
			buffer,
			length
		));
		size_t index = buffers.size();
		buffers.push_back( std::move(buffer) );
		this->updateBuffer( data, length, index, stage );
		return index;
	}

	VkQueue queue = device->graphicsQueue;
	Buffer staging;
	VkDeviceSize storageBufferSize = length;
	device->createBuffer(
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		staging,
		storageBufferSize,
		data
	);

	device->createBuffer(
		usageFlags,
		memoryPropertyFlags,
		buffer,
		storageBufferSize
	);

	// Copy to staging buffer
	VkCommandBuffer copyCmd = device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
	VkBufferCopy copyRegion = {};
	copyRegion.size = storageBufferSize;
	vkCmdCopyBuffer(copyCmd, staging.buffer, buffer.buffer, 1, &copyRegion);
	device->flushCommandBuffer(copyCmd, queue, true);
	staging.destroy();

	buffers.push_back( std::move(buffer) );
	return index;
}
void ext::vulkan::Buffers::updateBuffer( void* data, VkDeviceSize length, size_t index, bool stage ) {
	Buffer& buffer = buffers.at(index);
	return updateBuffer( data, length, buffer, stage );
}
void ext::vulkan::Buffers::updateBuffer( void* data, VkDeviceSize length, Buffer& buffer, bool stage ) {
	if ( !stage ) {
		VK_CHECK_RESULT(buffer.map());
		memcpy(buffer.mapped, data, length);
		buffer.unmap();
		return;
	}
	VkQueue queue = device->graphicsQueue;
	Buffer staging;
	device->createBuffer(
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		staging,
		length,
		data
	);

	// Copy to staging buffer
	VkCommandBuffer copyCmd = device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
	VkBufferCopy copyRegion = {};
	copyRegion.size = length;
	vkCmdCopyBuffer(copyCmd, staging.buffer, buffer.buffer, 1, &copyRegion);
	device->flushCommandBuffer(copyCmd, queue, true);
	staging.destroy();
}