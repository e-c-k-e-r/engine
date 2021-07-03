#pragma once

#include <uf/utils/memory/vector.h>
#include <uf/ext/opengl/ogl.h>
#include <uf/ext/opengl/enums.h>

#define GL_DEFAULT_STAGE_BUFFERS true

typedef void* GLbuffer;

namespace ext {
	namespace opengl {
		struct Device;

		struct UF_API Buffer {
			Device* device;
			GLuint buffer = 0;
			
			struct Descriptor {
				GLuint buffer = GL_NULL_HANDLE;
				GLsizeiptr offset = 0;
				GLsizeiptr range = GL_WHOLE_SIZE;
			} descriptor;

			struct CreateInfo {
				GLenum flags = 0;
				GLsizeiptr size = 0;
				GLenum usage = 0;
			};

			GLsizeiptr size = 0;
			GLsizeiptr alignment = 0;
			enums::Buffer::type_t usage = 0;

			GLhandle(VmaAllocation) allocation;
			struct AllocationInfo {
				uint32_t memoryType = 0;
				GLhandle(VkDeviceMemory) deviceMemory = GL_NULL_HANDLE;
				GLsizeiptr offset = 0;
				GLsizeiptr size = 0;
				void* pMappedData = NULL;
				void* pUserData = NULL;
			} allocationInfo;

			void* map( GLsizeiptr size = GL_WHOLE_SIZE, GLsizeiptr offset = 0 );
			void unmap();

			void* map( GLsizeiptr size = GL_WHOLE_SIZE, GLsizeiptr offset = 0 ) const;
			void unmap() const;

			bool bind( GLsizeiptr offset = 0 );

			void setupDescriptor( GLsizeiptr size = GL_WHOLE_SIZE, GLsizeiptr offset = 0 );
			void copyTo( const void* data, GLsizeiptr size ) const;
			bool flush( GLsizeiptr size = GL_WHOLE_SIZE, GLsizeiptr offset = 0 );
			bool invalidate( GLsizeiptr size = GL_WHOLE_SIZE, GLsizeiptr offset = 0 );
			void allocate( const CreateInfo& );

			// RAII
			~Buffer();
			void initialize( Device& device );
			void destroy();
		};
		struct UF_API Buffers {
			uf::stl::vector<Buffer> buffers;
			Device* device;

		//	~Buffers();
			//
			void initialize( Device& device );
			void destroy();
			//
			size_t initializeBuffer( const void* data, GLsizeiptr length, GLenum usage, bool stage = GL_DEFAULT_STAGE_BUFFERS );
			inline void updateBuffer( const void* data, GLsizeiptr length, const Buffer& buffer, bool stage = GL_DEFAULT_STAGE_BUFFERS ) const { return buffer.copyTo( data, length ); }

			inline size_t initializeBuffer( void* data, GLsizeiptr length, GLenum usage, bool stage = GL_DEFAULT_STAGE_BUFFERS ) { return initializeBuffer( (const void*) data, length, usage, stage ); }

			template<typename T> inline size_t initializeBuffer( const T& data, GLsizeiptr length, GLenum usage, bool stage = GL_DEFAULT_STAGE_BUFFERS ) { return initializeBuffer( (const void*) &data, length, usage, stage ); }
			template<typename T> inline size_t initializeBuffer( const T& data, GLenum usage, bool stage = GL_DEFAULT_STAGE_BUFFERS ) { return initializeBuffer( (const void*) &data, static_cast<GLsizeiptr>(sizeof(T)), usage, stage ); }

			inline void updateBuffer( const void* data, GLsizeiptr length, size_t index = 0, bool stage = GL_DEFAULT_STAGE_BUFFERS ) const { return updateBuffer( data, length, buffers.at(index), stage ); }
			inline void updateBuffer( void* data, GLsizeiptr length, size_t index = 0, bool stage = GL_DEFAULT_STAGE_BUFFERS ) const { return updateBuffer( (const void*) data, length, index, stage ); }
			inline void updateBuffer( void* data, GLsizeiptr length, const Buffer& buffer, bool stage = GL_DEFAULT_STAGE_BUFFERS ) const { return updateBuffer( (const void*) data, length, buffer, stage ); }

			template<typename T> inline void updateBuffer( const T& data, size_t index = 0, bool stage = GL_DEFAULT_STAGE_BUFFERS ) const { return updateBuffer( (const void*) &data, static_cast<GLsizeiptr>(sizeof(T)), index, stage ); }
			template<typename T> inline void updateBuffer( const T& data, GLsizeiptr length, size_t index = 0, bool stage = GL_DEFAULT_STAGE_BUFFERS ) const { return updateBuffer( (const void*) &data, length, index, stage ); }
			
			template<typename T> inline void updateBuffer( const T& data, const Buffer& buffer, bool stage = GL_DEFAULT_STAGE_BUFFERS ) const { return updateBuffer( (const void*) &data, static_cast<GLsizeiptr>(sizeof(T)), buffer, stage ); }
			template<typename T> inline void updateBuffer( const T& data, GLsizeiptr length, const Buffer& buffer, bool stage = GL_DEFAULT_STAGE_BUFFERS ) const { return updateBuffer( (const void*) &data, length, buffer, stage ); }
		
		};
	}
}