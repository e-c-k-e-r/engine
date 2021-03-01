#pragma once

#include <uf/ext/vulkan/vk.h>
#include <vector>

namespace ext {
	namespace vulkan {
		struct Device;

		struct UF_API Buffer {
			VkDevice device;
			VkBuffer buffer = VK_NULL_HANDLE;
			VkDeviceMemory memory = VK_NULL_HANDLE;
			VkDescriptorBufferInfo descriptor = {
				VK_NULL_HANDLE,
				0,
				0
			};
			VkDeviceSize size = 0;
			VkDeviceSize alignment = 0;
			void* mapped = nullptr;

			VkBufferUsageFlags usage;
			VkMemoryPropertyFlags memoryProperties;
			
			VkMemoryAllocateInfo memAlloc;
			VkMemoryRequirements memReqs;

			VmaAllocation allocation;
			VmaAllocationInfo allocationInfo;

			VkResult map( VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0 );

			void unmap();

			VkResult bind( VkDeviceSize offset = 0 );

			void setupDescriptor( VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0 );
			void copyTo( void* data, VkDeviceSize size );
			VkResult flush( VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0 );
			VkResult invalidate( VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0 );
		/*
			template<typename T>
			std::size_t getAlignment( const T& t ) {
				size_t min = device->properties.limits.minUniformBufferOffsetAlignment;
				size_t dynamicAlignment = sizeof(T);
				if (min > 0) {
					dynamicAlignment = (dynamicAlignment + min - 1) & ~(min - 1);
				}
				return dynamicAlignment;
			};
		*/
			void allocate( VkBufferCreateInfo );

			// RAII
			~Buffer();
			void initialize( VkDevice device );
			void destroy();
		};
		struct UF_API Buffers {
			std::vector<Buffer> buffers;
			Device* device;

		//	~Buffers();
			//
			void initialize( Device& device );
			void destroy();
			//
			size_t initializeBuffer( void* data, VkDeviceSize length, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, bool stage = VK_DEFAULT_STAGE_BUFFERS );
			template<typename T> inline size_t initializeBuffer( T data, VkDeviceSize length, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, bool stage = VK_DEFAULT_STAGE_BUFFERS ) { return initializeBuffer( (void*) &data, length, usage, memoryProperties, stage ); }
			template<typename T> inline size_t initializeBuffer( T data, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, bool stage = VK_DEFAULT_STAGE_BUFFERS ) { return initializeBuffer( (void*) &data, static_cast<VkDeviceSize>(sizeof(T)), usage, memoryProperties, stage ); }

			void updateBuffer( void* data, VkDeviceSize length, size_t index = 0, bool stage = VK_DEFAULT_STAGE_BUFFERS );
			void updateBuffer( void* data, VkDeviceSize length, Buffer& buffer, bool stage = VK_DEFAULT_STAGE_BUFFERS );
			template<typename T> inline void updateBuffer( T data, VkDeviceSize length, size_t index = 0, bool stage = VK_DEFAULT_STAGE_BUFFERS ) { return updateBuffer( (void*) &data, length, index, stage ); }
			template<typename T> inline void updateBuffer( T data, size_t index = 0, bool stage = VK_DEFAULT_STAGE_BUFFERS ) { return updateBuffer( (void*) &data, static_cast<VkDeviceSize>(sizeof(T)), index, stage ); }
			template<typename T> inline void updateBuffer( T data, VkDeviceSize length, Buffer& buffer, bool stage = VK_DEFAULT_STAGE_BUFFERS ) { return updateBuffer( (void*) &data, length, buffer, stage ); }
		};
	}
}