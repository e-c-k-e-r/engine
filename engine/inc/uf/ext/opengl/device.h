#pragma once

#include <uf/ext/opengl/ogl.h>
#include <uf/ext/opengl/buffer.h>
#include <uf/ext/opengl/commands.h>
#include <uf/spec/context/context.h>
#include <uf/utils/window/window.h>
#include <uf/utils/thread/thread.h>
#include <uf/utils/thread/perthread.h>

#include <deque>

namespace ext {
	namespace opengl {
		struct UF_API Device {
		public:
			uf::Window* window;
			spec::Context::Settings contextSettings;
			uf::ThreadUnique<spec::Context*> contexts;
			
			CommandBuffer commandBuffer;
			struct HandlePool {
				static size_t pageSize;
				typedef GLuint handle_t;

				enums::Command::type_t type;
				std::deque<handle_t> available;
				std::vector<handle_t> pool;

				void allocate();
				void allocate(size_t);
				void clear();

				bool used(handle_t);

				size_t alloc( handle_t& );
				void free( handle_t& );

			} textures, buffers;

			void initialize();
			void destroy();

			GLuint createBuffer( enums::Buffer::type_t usage, GLsizeiptr size, void* data = NULL );
			void* getBuffer( GLuint );
			void destroyBuffer( GLuint& );

			size_t createTexture( GLuint& );
			void destroyTexture( GLuint& );

			spec::Context& activateContext();
			spec::Context& activateContext(std::thread::id);
		};
	}
}