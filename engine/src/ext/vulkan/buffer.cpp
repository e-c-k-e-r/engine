#if UF_USE_VULKAN

#define VMA_VULKAN_VERSION 1002000
#define VMA_IMPLEMENTATION

#include <uf/ext/vulkan/buffer.h>
#include <uf/ext/vulkan/vulkan.h>
#include <uf/ext/vulkan/device.h>

void ext::vulkan::Buffer::swap( ext::vulkan::Buffer& buffer ) {
	std::swap(this->aliased, buffer.aliased);
	std::swap(this->device, buffer.device);
	std::swap(this->buffer, buffer.buffer);
	std::swap(this->memory, buffer.memory);
	std::swap(this->descriptor, buffer.descriptor);
	std::swap(this->alignment, buffer.alignment);
	std::swap(this->address, buffer.address);
	std::swap(this->mapped, buffer.mapped);
	std::swap(this->usage, buffer.usage);
	std::swap(this->memoryProperties, buffer.memoryProperties);
	std::swap(this->allocation, buffer.allocation);
	std::swap(this->allocationInfo, buffer.allocationInfo);
}
ext::vulkan::Buffer ext::vulkan::Buffer::alias() const {
	ext::vulkan::Buffer buffer;
	buffer.aliasBuffer(*this);
	return buffer;
}
void ext::vulkan::Buffer::aliasBuffer( const ext::vulkan::Buffer& buffer ) {
	this->aliased = true;
	this->device = buffer.device;
	this->buffer = buffer.buffer;
	this->memory = buffer.memory;
	this->descriptor = buffer.descriptor;
	this->alignment = buffer.alignment;
	this->address = buffer.address;
	this->mapped = buffer.mapped;
	this->usage = buffer.usage;
	this->memoryProperties = buffer.memoryProperties;
	this->allocation = buffer.allocation;
	this->allocationInfo = buffer.allocationInfo;
}

void* ext::vulkan::Buffer::map( VkDeviceSize size, VkDeviceSize offset ) {
	if ( !mapped ) VK_CHECK_RESULT(vmaMapMemory( allocator, allocation, &mapped ));
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

	vmaCreateBufferWithAlignment( allocator, &bufferCreateInfo, &allocCreateInfo, alignment, &buffer, &allocation, &allocationInfo );
}

size_t ext::vulkan::Buffer::getAddress() {
	VkBufferDeviceAddressInfoKHR info{};
	info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
	info.buffer = buffer;
	return (this->address = vkGetBufferDeviceAddressKHR(this->device ? *this->device : ext::vulkan::device, &info));
}
size_t ext::vulkan::Buffer::getAddress() const {
	VkBufferDeviceAddressInfoKHR info{};
	info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
	info.buffer = buffer;
	return vkGetBufferDeviceAddressKHR(this->device ? *this->device : ext::vulkan::device, &info);
}

// RAII
ext::vulkan::Buffer::~Buffer() {
//	this->destroy();
}
void ext::vulkan::Buffer::initialize( ext::vulkan::Device& device, size_t alignment ) {
	this->device = &device;
	this->alignment = alignment;
}
void ext::vulkan::Buffer::destroy() {
	if ( !device || aliased ) return;

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
		nullptr,
		length,
		usage,
		memoryProperties,
		*this
	));
	if ( data && length ) update( data, length, stage );
}
bool ext::vulkan::Buffer::update( const void* data, VkDeviceSize length, bool stage ) const {
//	if ( !data || !length ) return false;
	if ( !length ) return false;
	if ( !buffer ) return false;
	if ( length > allocationInfo.size ) {
		UF_MSG_WARNING("Buffer update of {} exceeds buffer size of {}", length, allocationInfo.size );

		Buffer& b = *const_cast<Buffer*>(this);
		b.destroy();
		b.initialize( data, length, usage, memoryProperties, stage );
		return true;
	}
	if ( !data ) return false;
	if ( !stage ) {
		void* map = this->map();
		memcpy(map, data, length);
		if ((this->memoryProperties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0) this->flush();
		this->unmap();
		return false;
	}

	ext::vulkan::Device* device = this->device ? this->device : &ext::vulkan::device;
#if 1
	Buffer staging = device->fetchTransientBuffer(
		data,
		length,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);
	VkCommandBuffer copyCommand = device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, QueueEnum::TRANSFER);
		VkBufferCopy region = {};
		region.size = length;
		vkCmdCopyBuffer(copyCommand, staging.buffer, buffer, 1, &region);
	device->flushCommandBuffer(copyCommand, QueueEnum::TRANSFER);
#else
	Buffer staging;
	device->createBuffer(
		data,
		length,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		staging
	);

	// Copy to staging buffer
	VkCommandBuffer copyCommand = device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, QueueEnum::TRANSFER);
		VkBufferCopy region = {};
		region.size = length;
		vkCmdCopyBuffer(copyCommand, staging.buffer, buffer, 1, &region);
	device->flushCommandBuffer(copyCommand, QueueEnum::TRANSFER);
	staging.destroy();
#endif
	return false;
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
	buffer.initialize( *device, requestedAlignment );
	buffer.initialize( data, length, usage, memoryProperties, stage );
	return index;
}
bool ext::vulkan::Buffers::updateBuffer( const void* data, VkDeviceSize length, const Buffer& buffer, bool stage ) const {
	return buffer.update( data, length, stage );
}
#endif