#if UF_USE_OPENGL

#include <uf/ext/opengl/opengl.h>
#include <uf/ext/opengl/rendertarget.h>
#include <uf/ext/opengl/initializers.h>
#include <uf/utils/window/window.h>

void ext::opengl::RenderTarget::addPass( GLhandle(VkPipelineStageFlags) stage, GLhandle(VkAccessFlags) access, const std::vector<size_t>& colors, const std::vector<size_t>& inputs, const std::vector<size_t>& resolves, size_t depth, bool autoBuildPipeline ) {
}
size_t ext::opengl::RenderTarget::attach( const Attachment::Descriptor& descriptor, Attachment* attachment ) {
	return attachments.size()-1;
}
void ext::opengl::RenderTarget::initialize( Device& device ) {
}

void ext::opengl::RenderTarget::destroy() {
}

#endif