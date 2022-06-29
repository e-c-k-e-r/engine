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
			size_t address = {};
			void* mapped = nullptr;

			VkBufferUsageFlags usage = 0;
			VkMemoryPropertyFlags memoryProperties = 0;

			VmaAllocation allocation = {};
			VmaAllocationInfo allocationInfo = {};

			void* map( VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0 );
			void unmap();
			void* map( VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0 ) const;
			void unmap() const;

			VkResult bind( VkDeviceSize offset = 0 );

			void setupDescriptor( VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0 );
			void copyTo( void* data, VkDeviceSize size );
			VkResult flush( VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0 ) const;
			VkResult invalidate( VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0 );
			void allocate( VkBufferCreateInfo );

			uint64_t getAddress();
			uint64_t getAddress() const;

			// RAII
			~Buffer();
			void initialize( ext::vulkan::Device& device, size_t = {} );
			void initialize( const void*, VkDeviceSize, VkBufferUsageFlags, VkMemoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, bool = VK_DEFAULT_STAGE_BUFFERS );
			bool update( const void*, VkDeviceSize, bool = VK_DEFAULT_STAGE_BUFFERS ) const;
			void destroy();

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
			void destroy();
			//

			size_t initializeBuffer( const void*, VkDeviceSize, VkBufferUsageFlags, VkMemoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, bool = VK_DEFAULT_STAGE_BUFFERS );
			bool updateBuffer( const void*, VkDeviceSize, const Buffer&, bool = VK_DEFAULT_STAGE_BUFFERS ) const;

		//	template<typename T> inline size_t initializeBuffer( const T* data, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, bool stage = VK_DEFAULT_STAGE_BUFFERS ) { return initializeBuffer( (const void*) data, static_cast<VkDeviceSize>(sizeof(T)), usage, memoryProperties, stage ); }
		//	template<typename T> inline size_t initializeBuffer( const T& data, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, bool stage = VK_DEFAULT_STAGE_BUFFERS ) { return initializeBuffer( (const void*) &data, static_cast<VkDeviceSize>(sizeof(T)), usage, memoryProperties, stage ); }

			inline bool updateBuffer( const void* data, VkDeviceSize length, size_t index = 0, bool stage = VK_DEFAULT_STAGE_BUFFERS ) const { return updateBuffer( data, length, buffers.at(index), stage ); }
		//	template<typename T> inline bool updateBuffer( const T* data, const Buffer& buffer, bool stage = VK_DEFAULT_STAGE_BUFFERS ) const { return updateBuffer( (const void*) data, static_cast<VkDeviceSize>(sizeof(T)), buffer, stage ); }
	//		template<typename T> inline bool updateBuffer( const T& data, const Buffer& buffer, bool stage = VK_DEFAULT_STAGE_BUFFERS ) const { return updateBuffer( (const void*) &data, static_cast<VkDeviceSize>(sizeof(T)), buffer, stage ); }
		};

		struct AccelerationStructure {
			VkAccelerationStructureKHR handle{VK_NULL_HANDLE};
			size_t deviceAddress{};
			Buffer buffer{};

			size_t instanceID{};
		};
	}
}