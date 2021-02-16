#pragma once

#include <uf/ext/opengl/ogl.h>
#include <uf/utils/thread/thread.h>
#include <uf/utils/thread/perthread.h>

namespace ext {
	namespace opengl {
		struct UF_API CommandBuffer {
			typedef std::function<void()> function_t;
			
			std::vector<CommandBuffer::function_t> commands;
			
			void record( const CommandBuffer::function_t& );
			void record( const CommandBuffer& );
			void submit();
			void flush();
			size_t size() const;
		};
	}
}