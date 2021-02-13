#if UF_USE_OPENGL

#include <uf/ext/opengl/texture.h>
#include <uf/ext/opengl/initializers.h>
#include <uf/utils/image/image.h>
#include <uf/ext/opengl/opengl.h>

ext::opengl::Texture2D ext::opengl::Texture2D::empty;

void ext::opengl::Sampler::initialize( Device& device ) {
}
void ext::opengl::Sampler::destroy() {
}

void ext::opengl::Texture::initialize( Device& device, size_t width, size_t height, size_t depth, size_t layers ) {
	this->device = &device;
	this->width = width;
	this->height = height;
	this->depth = depth;
	this->layers = layers;
	// implicitly set type
}
void ext::opengl::Texture::initialize( Device& device, GLhandle(VkImageViewType) viewType, size_t width, size_t height, size_t depth, size_t layers ) {
}
void ext::opengl::Texture::updateDescriptors() {
}
bool ext::opengl::Texture::generated() const {
	return false;
}
void ext::opengl::Texture::destroy() {
}
void ext::opengl::Texture::setImageLayout(
	GLhandle(VkCommandBuffer) cmdbuffer,
	GLhandle(VkImage) image,
	GLhandle(VkImageLayout) oldImageLayout,
	GLhandle(VkImageLayout) newImageLayout,
	GLhandle(VkImageSubresourceRange) subresourceRange
) {

}
void ext::opengl::Texture::setImageLayout(
	GLhandle(VkCommandBuffer) cmdbuffer,
	GLhandle(VkImage) image,
	GLhandle(VkImageAspectFlags) aspectMask,
	GLhandle(VkImageLayout) oldImageLayout,
	GLhandle(VkImageLayout) newImageLayout,
	uint32_t mipLevels
) {

}
void ext::opengl::Texture::loadFromFile(
	std::string filename, 
	GLhandle(VkFormat) format,
	GLhandle(VkImageUsageFlags) imageUsageFlags,
	GLhandle(VkImageLayout) imageLayout
) {
	return loadFromFile( filename, ext::opengl::device, format, imageUsageFlags, imageLayout );
}
void ext::opengl::Texture::loadFromFile(
	std::string filename, 
	Device& device,
	GLhandle(VkFormat) format,
	GLhandle(VkImageUsageFlags) imageUsageFlags,
	GLhandle(VkImageLayout) imageLayout 
) {

}
void ext::opengl::Texture::loadFromImage(
	uf::Image& image,
	GLhandle(VkFormat) format,
	GLhandle(VkImageUsageFlags) imageUsageFlags,
	GLhandle(VkImageLayout) imageLayout
) {
	return loadFromImage( image, ext::opengl::device, format, imageUsageFlags, imageLayout );
}
void ext::opengl::Texture::loadFromImage(
	uf::Image& image, 
	Device& device,
	GLhandle(VkFormat) format,
	GLhandle(VkImageUsageFlags) imageUsageFlags,
	GLhandle(VkImageLayout) imageLayout
) {

}

void ext::opengl::Texture::fromBuffers(
	void* buffer,
	GLhandle(VkDeviceSize) bufferSize,
	GLhandle(VkFormat) format,
	uint32_t texWidth,
	uint32_t texHeight,
	uint32_t texDepth,
	uint32_t layers,
	GLhandle(VkImageUsageFlags) imageUsageFlags,
	GLhandle(VkImageLayout) imageLayout
) {
	return this->fromBuffers( buffer, bufferSize, format, texWidth, texHeight, texDepth, layers, ext::opengl::device, imageUsageFlags, imageLayout );
}


void ext::opengl::Texture::fromBuffers(
	void* data,
	GLhandle(VkDeviceSize) bufferSize,
	GLhandle(VkFormat) format,
	uint32_t texWidth,
	uint32_t texHeight,
	uint32_t texDepth,
	uint32_t layers,
	Device& device,
	GLhandle(VkImageUsageFlags) imageUsageFlags,
	GLhandle(VkImageLayout) imageLayout
) {
}

void ext::opengl::Texture::asRenderTarget( Device& device, uint32_t width, uint32_t height, GLhandle(VkFormat) format ) {
}
void ext::opengl::Texture::aliasTexture( const Texture& texture ) {
}
void ext::opengl::Texture::aliasAttachment( const RenderTarget::Attachment& attachment, bool createSampler ) {
}

void ext::opengl::Texture::update( uf::Image& image, GLhandle(VkImageLayout) targetImageLayout, uint32_t layer ) {
	return this->update( (void*) image.getPixelsPtr(), image.getPixels().size(), layer );
}
void ext::opengl::Texture::update( void* data, GLhandle(VkDeviceSize) bufferSize, GLhandle(VkImageLayout) targetImageLayout, uint32_t layer ) {
}

void ext::opengl::Texture::generateMipmaps( GLhandle(VkCommandBuffer) commandBuffer, uint32_t layer ) {
}

ext::opengl::Texture2D::Texture2D() {
}
ext::opengl::Texture3D::Texture3D() {
}
ext::opengl::TextureCube::TextureCube() {
}

#endif