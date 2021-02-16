#if UF_USE_OPENGL

#include <uf/ext/opengl/opengl.h>
#include <uf/ext/opengl/commands.h>

void ext::opengl::CommandBuffer::record( const CommandBuffer::function_t& command ) {
	commands.emplace_back( command );
}
void ext::opengl::CommandBuffer::record( const CommandBuffer& commandBuffer ) {
	commands.insert( commands.end(), commandBuffer.commands.begin(), commandBuffer.commands.end() );
}
void ext::opengl::CommandBuffer::submit() {
	for ( auto& command : commands ) {
		command();
	}
}
void ext::opengl::CommandBuffer::flush() {
	commands.empty();
}
size_t ext::opengl::CommandBuffer::size() const {
	return commands.size();
}

#endif