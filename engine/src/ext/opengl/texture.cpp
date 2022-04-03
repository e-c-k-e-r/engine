#if UF_USE_OPENGL

#include <uf/ext/opengl/texture.h>
#include <uf/ext/opengl/initializers.h>
#include <uf/utils/image/image.h>
#include <uf/ext/opengl/opengl.h>

#include <cmath>

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
	if ( width > 1 && height > 1 && depth > 1 ) {
		this->type = enums::Image::TYPE_3D;
	} else if ( (width == 1 && height > 1 && depth > 1) || (width > 1 && height == 1 && depth > 1) || (width > 1 && height > 1 && depth == 1) ) {
		this->type = enums::Image::TYPE_2D;
	} else if ( (width > 1 && height == 1 && depth == 1) || (width == 1 && height > 1 && depth == 1) || (width == 1 && height == 1 && depth > 1) ) {
		this->type = enums::Image::TYPE_1D;
	}
	if ( layers > 1 ) {
		if ( viewType == enums::Image::VIEW_TYPE_1D ) viewType = enums::Image::VIEW_TYPE_1D_ARRAY;
		if ( viewType == enums::Image::VIEW_TYPE_2D ) viewType = enums::Image::VIEW_TYPE_2D_ARRAY;
		if ( viewType == enums::Image::VIEW_TYPE_CUBE ) viewType = enums::Image::VIEW_TYPE_CUBE_ARRAY;
	}
}
void ext::opengl::Texture::initialize( Device& device, enums::Image::viewType_t viewType, size_t width, size_t height, size_t depth, size_t layers ) {
	this->initialize( device, width, height, depth, layers );
	this->viewType = viewType;
	switch ( viewType ) {
		case enums::Image::VIEW_TYPE_1D:
		case enums::Image::VIEW_TYPE_1D_ARRAY:
			type = enums::Image::TYPE_1D;
		break;
		case enums::Image::VIEW_TYPE_2D:
		case enums::Image::VIEW_TYPE_CUBE:
		case enums::Image::VIEW_TYPE_2D_ARRAY:
		case enums::Image::VIEW_TYPE_CUBE_ARRAY: 
			type = enums::Image::TYPE_2D;
		break;
		case enums::Image::VIEW_TYPE_3D:
			type = enums::Image::TYPE_3D;
		break;
	}
}
void ext::opengl::Texture::updateDescriptors() {
	descriptor.image = image;
	descriptor.viewType = viewType;
	descriptor.format = format;
	descriptor.width = width;
	descriptor.height = height;
	descriptor.depth = depth;
	descriptor.layers = layers;
}
bool ext::opengl::Texture::generated() const {
#if UF_ENV_DREAMCAST
	return image;
#else
	return glIsTexture(image);
#endif
}
void ext::opengl::Texture::destroy() {
	if ( !device ) return;
	if ( generated() ) {
		GL_MUTEX_LOCK();
		GL_ERROR_CHECK(glDeleteTextures(1, &image));
		GL_MUTEX_UNLOCK();
	}
	image = GL_NULL_HANDLE;
}
void ext::opengl::Texture::loadFromFile(
	const uf::stl::string& filename, 
	enums::Format::type_t format
) {
	return loadFromFile( filename, ext::opengl::device, format );
}
void ext::opengl::Texture::loadFromImage(
	const uf::Image& image, 
	enums::Format::type_t format
) {
	return loadFromImage( image, ext::opengl::device, format );
}
void ext::opengl::Texture::loadFromFile(
	const uf::stl::string& filename, 
	Device& device,
	enums::Format::type_t format
) {
	uf::Image image;
	image.open(filename);
	return loadFromImage( image, device, format );
}
void ext::opengl::Texture::loadFromImage(
	const uf::Image& image, 
	Device& device,
	enums::Format::type_t format
) {
	switch ( format ) {
		case enums::Format::R8_SRGB:
		case enums::Format::R8G8_SRGB:
		case enums::Format::R8G8B8_SRGB:
		case enums::Format::R8G8B8A8_SRGB:
			srgb = true;
		break;
	}

	if ( image.getFormat() != 0 ) {
		internalFormat = image.getFormat();
	} else {
		switch ( image.getChannels() ) {
			// R
			case 1:
				switch ( image.getBpp() ) {
					case 8:
						format = srgb ? enums::Format::R8_SRGB : enums::Format::R8_UNORM;
					break;
					default:
					UF_EXCEPTION("OpenGL error: unsupported BPP of " << image.getBpp() );
					break;
				}
			break;
			// RB
			case 2:
				switch ( image.getBpp() ) {
					case 16:
						format = srgb ? enums::Format::R8G8_SRGB : enums::Format::R8G8_UNORM;
					break;
					default:
					UF_EXCEPTION("OpenGL error: unsupported BPP of " << image.getBpp() );
					break;
				}
			break;
			// RGB
			case 3:
				switch ( image.getBpp() ) {
					case 24:
						format = srgb ? enums::Format::R8G8B8_SRGB : enums::Format::R8G8B8_UNORM;
					break;
					default:
					UF_EXCEPTION("OpenGL error: unsupported BPP of " << image.getBpp() );
					break;
				}
			break;
			// RGBA
			case 4:
				switch ( image.getBpp() ) {
					case 16:
						format = enums::Format::R4G4B4A4_UNORM_PACK16;
					break;
					case 32:
						format = srgb ? enums::Format::R8G8B8A8_SRGB : enums::Format::R8G8B8A8_UNORM;
					break;
					default:
					UF_EXCEPTION("OpenGL error: unsupported BPP of " << image.getBpp() );
					break;
				}
			break;
			default:
			UF_EXCEPTION("OpenGL error: unsupported channels of " << image.getChannels() );
			break;
		}
	}
	// convert to power of two
	//image.padToPowerOfTwo();

	this->fromBuffers( 
		(void*) image.getPixelsPtr(),
		image.getPixels().size(),
		format,
		image.getDimensions()[0],
		image.getDimensions()[1],
		1,
		1,
		device
	);
}

void ext::opengl::Texture::fromBuffers(
	void* buffer,
	size_t bufferSize,
	enums::Format::type_t format,
	uint32_t texWidth,
	uint32_t texHeight,
	uint32_t texDepth,
	uint32_t layers
) {
	return this->fromBuffers( buffer, bufferSize, format, texWidth, texHeight, texDepth, layers, ext::opengl::device );
}


void ext::opengl::Texture::fromBuffers(
	void* data,
	size_t bufferSize,
	enums::Format::type_t format,
	uint32_t texWidth,
	uint32_t texHeight,
	uint32_t texDepth,
	uint32_t layers,
	Device& device
) {
	this->initialize(device, viewType, texWidth, texHeight, texDepth, layers);
	this->format = format;

#if !UF_ENV_DREAMCAST
	if ( this->mips == 0 ) {
		this->mips = 1;
	} else if ( this->depth == 1 ) {
		this->mips = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
	}
#endif
	// Create sampler
	sampler.descriptor.mip.min = 0;
	sampler.descriptor.mip.max = static_cast<float>(this->mips);
	sampler.initialize( device );
	
	device.activateContext();
#if 0
	device.createTexture(image);
	this->updateDescriptors();
	if ( data ) this->update( data, bufferSize, 0 );
#else
	GL_MUTEX_LOCK();
	GL_ERROR_CHECK(glGenTextures(1, &image));
	GL_ERROR_CHECK(glBindTexture(viewType, image));
	GL_ERROR_CHECK(glTexParameteri(viewType, GL_TEXTURE_MIN_FILTER, sampler.descriptor.filter.min));
	GL_ERROR_CHECK(glTexParameteri(viewType, GL_TEXTURE_MAG_FILTER, sampler.descriptor.filter.mag));
	GL_ERROR_CHECK(glBindTexture(viewType, 0));
	GL_MUTEX_UNLOCK();
	if ( data ) this->update( data, bufferSize, 0 );
	this->updateDescriptors();
#endif
}

void ext::opengl::Texture::asRenderTarget( Device& device, uint32_t width, uint32_t height, enums::Format::type_t format ) {
}
void ext::opengl::Texture::aliasTexture( const Texture& texture ) {
	image = texture.image;
	type = texture.type;
	viewType = texture.viewType;
	format = texture.format;
	sampler = texture.sampler;
	width = texture.width;
	height = texture.height;
	depth = texture.depth;
	layers = texture.layers;
	
	this->updateDescriptors();
}
void ext::opengl::Texture::aliasAttachment( const RenderTarget::Attachment& attachment, bool createSampler ) {
}

void ext::opengl::Texture::update( uf::Image& image, uint32_t layer ) {
	return this->update( (void*) image.getPixelsPtr(), image.getPixels().size(), layer );
}
void ext::opengl::Texture::update( void* data, size_t bufferSize, uint32_t layer ) {
#if UF_ENV_DREAMCAST || UF_USE_OPENGL_GLDC
	if ( internalFormat > 0 ) {
	GL_MUTEX_LOCK();
		GL_ERROR_CHECK(glBindTexture(viewType, image));
		GL_ERROR_CHECK(glCompressedTexImage2DARB( viewType, 0, internalFormat, width, height, 0, bufferSize, data));
		GL_ERROR_CHECK(glBindTexture(viewType, 0));
	GL_MUTEX_UNLOCK();
		return;
	}
#endif
	GLenum format = GL_RGBA;
	GLenum type = GL_UNSIGNED_BYTE;
	switch ( this->format ) {
		case enums::Format::R8_UNORM:
		case enums::Format::R8_SRGB:
			format = GL_LUMINANCE;
		break;
		case enums::Format::R8G8_UNORM:
		case enums::Format::R8G8_SRGB:
			format = GL_LUMINANCE_ALPHA;
		break;
		case enums::Format::R8G8B8_UNORM:
		case enums::Format::R8G8B8_SRGB:
			format = GL_RGB;
		break;
		case enums::Format::R4G4B4A4_UNORM_PACK16:
		case enums::Format::R4G4B4A4_UNORM_PSRGB:
			format = GL_RGBA;
			type = GL_UNSIGNED_SHORT_4_4_4_4;
		break;
		case enums::Format::R8G8B8A8_UNORM:
		case enums::Format::R8G8B8A8_SRGB:
			format = GL_RGBA;
		break;
	}
	GL_MUTEX_LOCK();
	GL_ERROR_CHECK(glBindTexture(viewType, image));
	switch ( viewType ) {
		case enums::Image::VIEW_TYPE_2D: { GL_ERROR_CHECK(glTexImage2D(viewType, 0, format, width, height, 0, format, type, data)); } break;
	#if !UF_USE_OPENGL_FIXED_FUNCTION
		case enums::Image::VIEW_TYPE_3D: { GL_ERROR_CHECK(glTexImage3D(viewType, 0, format, width, height, depth, 0, format, type, data)); } break;
	#endif
	}
	GL_ERROR_CHECK(glBindTexture(viewType, 0));
	GL_MUTEX_UNLOCK();
}

void ext::opengl::Texture::generateMipmaps( uint32_t layer ) {
}

ext::opengl::Texture2D::Texture2D() {
	type = enums::Image::TYPE_2D;
	viewType = enums::Image::VIEW_TYPE_2D;
}
ext::opengl::Texture3D::Texture3D() {
	type = enums::Image::TYPE_3D;
	viewType = enums::Image::VIEW_TYPE_3D;
}
ext::opengl::TextureCube::TextureCube() {
	type = enums::Image::TYPE_2D;
	viewType = enums::Image::VIEW_TYPE_CUBE;
}

#endif