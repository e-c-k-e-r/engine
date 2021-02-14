#pragma once

#include <vector>
#include <uf/ext/opengl/ogl.h>

#define GL_DEFAULT_STAGE_BUFFERS true

namespace ext {
	namespace opengl {
		struct Device;

		struct UF_API Buffer {
			Device* device;
			void* buffer = GL_NULL_HANDLE;
			
			struct {
				void* buffer = NULL;
				GLsizeiptr offset = 0;
				GLsizeiptr range = GL_WHOLE_SIZE;
			} descriptor = {GL_NULL_HANDLE,0,0};

			struct CreateInfo {
				GLenum flags = 0;
				GLsizeiptr size = 0;
				GLenum usage = 0;
			};

			GLsizeiptr size = 0;
			GLsizeiptr alignment = 0;
			GLenum usageFlags = 0;

			GLhandle(VmaAllocation) allocation;
			struct AllocationInfo {
				uint32_t memoryType = 0;
				GLhandle(VkDeviceMemory) deviceMemory = GL_NULL_HANDLE;
				GLsizeiptr offset = 0;
				GLsizeiptr size = 0;
				void* pMappedData = NULL;
				void* pUserData = NULL;
			} allocationInfo;

			bool map( GLsizeiptr size = GL_WHOLE_SIZE, GLsizeiptr offset = 0 );
			void unmap();

			bool bind( GLsizeiptr offset = 0 );

			void setupDescriptor( GLsizeiptr size = GL_WHOLE_SIZE, GLsizeiptr offset = 0 );
			void copyTo( void* data, GLsizeiptr size );
			bool flush( GLsizeiptr size = GL_WHOLE_SIZE, GLsizeiptr offset = 0 );
			bool invalidate( GLsizeiptr size = GL_WHOLE_SIZE, GLsizeiptr offset = 0 );
			void allocate( const CreateInfo& );

			// RAII
			~Buffer();
			void initialize( Device& device );
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
			size_t initializeBuffer( void* data, GLsizeiptr length, GLenum usageFlags, GLhandle(VkMemoryPropertyFlags) memoryPropertyFlags = GLenumerator(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT), bool stage = GL_DEFAULT_STAGE_BUFFERS );
			template<typename T> inline size_t initializeBuffer( T data, GLsizeiptr length, GLenum usageFlags, GLhandle(VkMemoryPropertyFlags) memoryPropertyFlags = GLenumerator(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT), bool stage = GL_DEFAULT_STAGE_BUFFERS ) {
				return initializeBuffer( (void*) &data, length, usageFlags, memoryPropertyFlags, stage );
			}
			template<typename T> inline size_t initializeBuffer( T data, GLenum usageFlags, GLhandle(VkMemoryPropertyFlags) memoryPropertyFlags = GLenumerator(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT), bool stage = GL_DEFAULT_STAGE_BUFFERS ) {
				return initializeBuffer( (void*) &data, static_cast<GLsizeiptr>(sizeof(T)), usageFlags, memoryPropertyFlags, stage );
			}

			void updateBuffer( void* data, GLsizeiptr length, size_t index = 0, bool stage = GL_DEFAULT_STAGE_BUFFERS );
			void updateBuffer( void* data, GLsizeiptr length, Buffer& buffer, bool stage = GL_DEFAULT_STAGE_BUFFERS );
			template<typename T> inline void updateBuffer( T data, GLsizeiptr length, size_t index = 0, bool stage = GL_DEFAULT_STAGE_BUFFERS ) { return updateBuffer( (void*) &data, length, index, stage ); }
			template<typename T> inline void updateBuffer( T data, size_t index = 0, bool stage = GL_DEFAULT_STAGE_BUFFERS ) { return updateBuffer( (void*) &data, static_cast<GLsizeiptr>(sizeof(T)), index, stage ); }
			template<typename T> inline void updateBuffer( T data, GLsizeiptr length, Buffer& buffer, bool stage = GL_DEFAULT_STAGE_BUFFERS ) { return updateBuffer( (void*) &data, length, buffer, stage ); }
		};
	}
}