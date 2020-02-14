#pragma once

#include <vector>
#include <uf/ext/opengl/ogl.h>

#define GL_DEFAULT_STAGE_BUFFERS true

namespace ext {
	namespace opengl {
		struct Device;

		struct UF_API Buffer {
			GLhandle(VkDevice) device;
			GLhandle(VkBuffer) buffer = GL_NULL_HANDLE;
			GLhandle(VkDeviceMemory) memory = GL_NULL_HANDLE;
			GLhandle(VkDescriptorBufferInfo) descriptor; // = {GL_NULL_HANDLE,0,0};
			GLsizeiptr size = 0;
			GLsizeiptr alignment = 0;
			void* mapped = nullptr;

			GLhandle(VkBufferUsageFlags) usageFlags;
			GLhandle(VkMemoryPropertyFlags) memoryPropertyFlags;
			
			GLhandle(VkMemoryAllocateInfo) memAlloc;
			GLhandle(VkMemoryRequirements) memReqs;

			GLhandle(VmaAllocation) allocation;
			GLhandle(VmaAllocationInfo) allocationInfo;

			GLhandle(VkResult) map( GLsizeiptr size = GL_WHOLE_SIZE, GLsizeiptr offset = 0 );

			void unmap();

			GLhandle(VkResult) bind( GLsizeiptr offset = 0 );

			void setupDescriptor( GLsizeiptr size = GL_WHOLE_SIZE, GLsizeiptr offset = 0 );
			void copyTo( void* data, GLsizeiptr size );
			GLhandle(VkResult) flush( GLsizeiptr size = GL_WHOLE_SIZE, GLsizeiptr offset = 0 );
			GLhandle(VkResult) invalidate( GLsizeiptr size = GL_WHOLE_SIZE, GLsizeiptr offset = 0 );
			void allocate( GLhandle(VkBufferCreateInfo) );

			// RAII
			~Buffer();
			void initialize( GLhandle(VkDevice) device );
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
			size_t initializeBuffer( void* data, GLsizeiptr length, GLhandle(VkBufferUsageFlags) usageFlags, GLhandle(VkMemoryPropertyFlags) memoryPropertyFlags = GLenumerator(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT), bool stage = GL_DEFAULT_STAGE_BUFFERS );
			template<typename T> inline size_t initializeBuffer( T data, GLsizeiptr length, GLhandle(VkBufferUsageFlags) usageFlags, GLhandle(VkMemoryPropertyFlags) memoryPropertyFlags = GLenumerator(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT), bool stage = GL_DEFAULT_STAGE_BUFFERS ) { return initializeBuffer( (void*) &data, length, usageFlags, memoryPropertyFlags, stage ); }
			template<typename T> inline size_t initializeBuffer( T data, GLhandle(VkBufferUsageFlags) usageFlags, GLhandle(VkMemoryPropertyFlags) memoryPropertyFlags = GLenumerator(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT), bool stage = GL_DEFAULT_STAGE_BUFFERS ) { return initializeBuffer( (void*) &data, static_cast<GLsizeiptr>(sizeof(T)), usageFlags, memoryPropertyFlags, stage ); }

			void updateBuffer( void* data, GLsizeiptr length, size_t index = 0, bool stage = GL_DEFAULT_STAGE_BUFFERS );
			void updateBuffer( void* data, GLsizeiptr length, Buffer& buffer, bool stage = GL_DEFAULT_STAGE_BUFFERS );
			template<typename T> inline void updateBuffer( T data, GLsizeiptr length, size_t index = 0, bool stage = GL_DEFAULT_STAGE_BUFFERS ) { return updateBuffer( (void*) &data, length, index, stage ); }
			template<typename T> inline void updateBuffer( T data, size_t index = 0, bool stage = GL_DEFAULT_STAGE_BUFFERS ) { return updateBuffer( (void*) &data, static_cast<GLsizeiptr>(sizeof(T)), index, stage ); }
			template<typename T> inline void updateBuffer( T data, GLsizeiptr length, Buffer& buffer, bool stage = GL_DEFAULT_STAGE_BUFFERS ) { return updateBuffer( (void*) &data, length, buffer, stage ); }
		};
	}
}