#pragma once

#include <uf/ext/vulkan/vk.h>
#include <uf/utils/memory/vector.h>

namespace ext {
	namespace vulkan {
		struct Device;

		struct UF_API Buffer {
			bool aliased = false;
			ext::vulkan::Device* device = NULL;
			VkBuffer buffer = VK_NULL_HANDLE;
			VkDeviceMemory memory = VK_NULL_HANDLE;
			VkDescriptorBufferInfo descriptor = {
				VK_NULL_HANDLE,
				0,
				0
			};
			VkDeviceSize alignment = 0;
			mutable size_t address = {};
			void* mapped = nullptr;
			int32_t count = 1;

			VkBufferUsageFlags usage = 0;
			VkMemoryPropertyFlags memoryProperties = 0;

			VmaAllocation allocation = {};
			VmaAllocationInfo allocationInfo = {};

			void* map( VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0 );
			void unmap();

			void updateDescriptor( VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0 );
			void allocate( VkBufferCreateInfo );

			uint64_t getAddress() const;
			
			VkDeviceSize getLength() const; // returns the aligned length for the entire buffer
			VkDeviceSize getOffset( size_t = 0 ) const; // returns the offset / stride / length of one object within the buffer

			~Buffer();
			void initialize( ext::vulkan::Device& device, size_t = {} );
			void initialize( const void*, VkDeviceSize, VkBufferUsageFlags, VkMemoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, bool = VK_DEFAULT_STAGE_BUFFERS );
			bool update( const void*, VkDeviceSize, bool = VK_DEFAULT_STAGE_BUFFERS ) const; // returns true if a reallocation occurred (to signal rebuilding command buffers)
			void destroy(bool = VK_DEFAULT_DEFER_BUFFER_DESTROY);

			void swap( Buffer& );
			Buffer alias() const;
			void aliasBuffer( const Buffer& );
		};
		struct UF_API Buffers {
			size_t requestedAlignment{};
			uf::stl::vector<Buffer> buffers;
			Device* device = NULL;

		//	~Buffers();
			//
			void initialize( Device& device );
			void destroy(bool = VK_DEFAULT_DEFER_BUFFER_DESTROY);
			//

			size_t initializeBuffer( const void*, VkDeviceSize, VkBufferUsageFlags, VkMemoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, bool = VK_DEFAULT_STAGE_BUFFERS );
			bool updateBuffer( const void*, VkDeviceSize, const Buffer&, bool = VK_DEFAULT_STAGE_BUFFERS ) const;
			inline bool updateBuffer( const void* data, VkDeviceSize length, size_t index = 0, bool stage = VK_DEFAULT_STAGE_BUFFERS ) const { return updateBuffer( data, length, buffers.at(index), stage ); }
		};

		struct AccelerationStructure {
			VkAccelerationStructureKHR handle{VK_NULL_HANDLE};
			size_t deviceAddress{};
			Buffer buffer{};

			size_t instanceID{};
			bool aliased = false;
		};
	}
}