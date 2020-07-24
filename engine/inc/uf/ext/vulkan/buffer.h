#pragma once

#include <uf/ext/vulkan.h>

namespace ext {
	namespace vulkan {
		struct UF_API Buffer {
			VkDevice device;
			VkBuffer buffer = VK_NULL_HANDLE;
			VkDeviceMemory memory = VK_NULL_HANDLE;
			VkDescriptorBufferInfo descriptor;
			VkDeviceSize size = 0;
			VkDeviceSize alignment = 0;
			void* mapped = nullptr;

			VkBufferUsageFlags usageFlags;
			VkMemoryPropertyFlags memoryPropertyFlags;
			
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
	}
}