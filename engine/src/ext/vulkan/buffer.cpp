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
//	size = allocationInfo.size;
}

// RAII
ext::vulkan::Buffer::~Buffer() {
//	this->destroy();
}
void ext::vulkan::Buffer::initialize( VkDevice device ) {
	this->device = device;
}
void ext::vulkan::Buffer::destroy() {
	if ( !device ) return;

	if ( buffer ) {
		vmaDestroyBuffer( allocator, buffer, allocation );
//		vkDestroyBuffer(device, buffer, nullptr);
	}
//	if ( memory ) vkFreeMemory(device, memory, nullptr);

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
	for ( auto& buffer : buffers ) if ( buffer.device ) buffer.destroy();
	buffers.clear();
}

size_t ext::vulkan::Buffers::initializeBuffer( const void* data, VkDeviceSize length, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties, bool stage ) {
	size_t index = buffers.size();
	Buffer& buffer = buffers.emplace_back();
	
	// Stage
	if ( !stage ) {
		VK_CHECK_RESULT(device->createBuffer(
			usage,
			memoryProperties,
			buffer,
			length
		));
		this->updateBuffer( data, length, buffer, stage );
		return index;
	}

	// implicitly set properties
	usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

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
		usage,
		memoryProperties,
		buffer,
		storageBufferSize
	);

	// Copy to staging buffer
	VkCommandBuffer copyCmd = device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
	VkBufferCopy copyRegion = {};
	copyRegion.size = storageBufferSize;
	vkCmdCopyBuffer(copyCmd, staging.buffer, buffer.buffer, 1, &copyRegion);
	device->flushCommandBuffer(copyCmd, true);
	staging.destroy();

	return index;
}
void ext::vulkan::Buffers::updateBuffer( const void* data, VkDeviceSize length, const Buffer& buffer, bool stage ) const {
//	assert(buffer.allocationInfo.size == length);

	if ( length > buffer.allocationInfo.size ) {
		if ( !true ) {
			VK_VALIDATION_MESSAGE("Mismatch buffer update: Requesting " << buffer.allocationInfo.size << ", got " << length << "; Userdata might've been corrupted, please try validating with shader.validate() before updating buffer");
		} else {
			VK_VALIDATION_MESSAGE("Mismatch buffer update: Requesting " << buffer.allocationInfo.size << ", got " << length << ", resetting for safety");
			length = buffer.allocationInfo.size;
		}
//		assert(buffer.allocationInfo.size > length);
	}
	if ( !stage ) {
	#if UF_TESTING
		void* map = buffer.map();
		memcpy(map, data, length);
		if ((buffer.memoryProperties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0) buffer.flush();
		buffer.unmap();
	#else
		void* map = buffer.map();
		memcpy(map, data, length);
		if ((buffer.memoryProperties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0) buffer.flush();
		buffer.unmap();
	#endif
		return;
	}
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

	device->flushCommandBuffer(copyCmd, true);
	staging.destroy();
}

#endif