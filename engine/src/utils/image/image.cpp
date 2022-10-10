#include <uf/utils/image/image.h>
#include <uf/utils/string/ext.h>
#include <uf/utils/string/hash.h>
#include <uf/utils/io/iostream.h>
#include <fstream> 					// std::fstream
#include <iostream> 				// std::fstream
#include <png/png.h> 				// libpng

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <gltf/stb_image.h>
#include <gltf/stb_image_write.h>
#include <uf/utils/renderer/renderer.h>

// 	C-tor
// Default
/*
uf::Image::Image() :
	m_bpp(8),
	m_channels(4),
	m_format(0) {

}
// Just Size
uf::Image::Image( const Image::vec2_t& size ) :
	m_dimensions(size),
	m_bpp(8),
	m_channels(4),
	m_format(0)
{
	this->m_pixels.reserve(size.x*size.y*this->m_channels);
}
// Move pixels
uf::Image::Image( Image&& move ) : 
	m_pixels(std::move(move.m_pixels)),
	m_dimensions(std::move(move.m_dimensions)),
	m_bpp(move.m_bpp),
	m_channels(move.m_channels),
	m_filename(move.m_filename),
	m_format(move.m_format)
{

}
// Copy pixels
uf::Image::Image( const Image& copy ) :
	m_pixels(copy.m_pixels),
	m_dimensions(copy.m_dimensions),
	m_bpp(copy.m_bpp),
	m_channels(copy.m_channels),
	m_filename(copy.m_filename),
	m_format(copy.m_format)
{

}
// Move from vector of pixels
uf::Image::Image( Image::container_t&& move, const Image::vec2_t& size ) :
	m_pixels(std::move(move)),
	m_dimensions(size),
	m_bpp(8),
	m_channels(4),
	m_format(0)
{

}
// Copy from vector of pixels
uf::Image::Image( const Image::container_t& copy, const Image::vec2_t& size ) :
	m_pixels(copy),
	m_dimensions(size),
	m_bpp(8),
	m_channels(4),
	m_format(0)
{

}
*/
uf::stl::string uf::Image::getFilename() const {
	return this->m_filename;
}

#define _PACK4(v) ((v * 0xF) / 0xFF)
#define PACK_ARGB4444(a,r,g,b) (_PACK4(a) << 12) | (_PACK4(r) << 8) | (_PACK4(g) << 4) | (_PACK4(b))
#define PACK_ARGB8888(a,r,g,b) ( ((a & 0xFF) << 24) | ((r & 0xFF) << 16) | ((g & 0xFF) << 8) | (b & 0xFF) )
#define PACK_ARGB1555(a,r,g,b) \
	(((uint16_t)(a > 0) << 15) | (((uint16_t) r >> 3) << 10) | (((uint16_t)g >> 3) << 5) | ((uint16_t)b >> 3))

#define PACK_RGB565(r,g,b) \
	((((uint16_t)r & 0xf8) << 8) | (((uint16_t) g & 0xfc) << 3) | ((uint16_t) b >> 3))

// from file
bool uf::Image::open( const uf::stl::string& filename, bool flip ) {
#if UF_USE_OPENGL_GLDC
	uf::stl::string extension = uf::io::extension(filename);
	if ( extension != "dtex" ) {
		uf::stl::string dtex = uf::string::replace( filename, ".png", ".dtex" );
		if ( uf::io::exists(dtex) ) return this->open(dtex, flip);

		UF_MSG_WARNING("non-dtex loading is highly discouraged on this platform: {}", filename);
	//	return false;
	}
#endif
	if ( !uf::io::exists(filename) ) UF_EXCEPTION("IO error: file does not exist: {}", filename);
	
	this->m_filename = filename;
	this->m_pixels.clear();
	int width = 0, height = 0, channelsDud = 0, bit_depth = 8, channels = 4;
#if UF_USE_OPENGL_GLDC
	if ( extension == "dtex" ) {
		struct {
			char		id[4]; // 'DTEX'
			uint16_t 	width;
			uint16_t 	height;
			uint32_t 	type;
			uint32_t 	size;
		} header;

		FILE* file = NULL;
		file = fopen(filename.c_str(), "rb");
		fread(&header, sizeof(header), 1, file);
		this->m_pixels.resize(header.size);
		fread(this->m_pixels.data(), header.size, 1, file);
		fclose(file);

		bool twiddled = (header.type & (1 << 26)) < 1;
		bool compressed = (header.type & (1 << 30)) > 0;
		bool mipmapped = (header.type & (1 << 31)) > 0;
		bool strided = (header.type & (1 << 25)) > 0;
		uint32_t format = (header.type >> 27) & 0b111;
		width = header.width;
		height = header.height;

		uint32_t expected = 2 * header.width * header.height;
		uint32_t ratio = (uint32_t) (((float) expected) / ((float) header.size));
		bit_depth = 4;
		this->m_format = format;
		if ( compressed ) {
			if ( twiddled ) {
				switch ( format ) {
					case 1: this->m_format = mipmapped ? GL_COMPRESSED_RGB_565_VQ_MIPMAP_TWID_KOS : GL_COMPRESSED_RGB_565_VQ_TWID_KOS; channels = 3; break;
					case 0: this->m_format = mipmapped ? GL_COMPRESSED_ARGB_1555_VQ_MIPMAP_TWID_KOS : GL_COMPRESSED_ARGB_1555_VQ_TWID_KOS; break;
					case 2: this->m_format = mipmapped ? GL_COMPRESSED_ARGB_4444_VQ_MIPMAP_TWID_KOS : GL_COMPRESSED_ARGB_4444_VQ_TWID_KOS; break;
					default: UF_EXCEPTION("Image error: invalid texture format: {}", filename); return false;
				}
			} else {
				switch ( format ) {
					case 1: this->m_format = mipmapped ? GL_COMPRESSED_RGB_565_VQ_MIPMAP_KOS : GL_COMPRESSED_RGB_565_VQ_KOS; channels = 3; break;
					case 0: this->m_format = mipmapped ? GL_COMPRESSED_ARGB_1555_VQ_MIPMAP_KOS : GL_COMPRESSED_ARGB_1555_VQ_KOS; break;
					case 2: this->m_format = mipmapped ? GL_COMPRESSED_ARGB_4444_VQ_MIPMAP_KOS : GL_COMPRESSED_ARGB_4444_VQ_KOS; break;
					default: UF_EXCEPTION("Image error: invalid texture format: {}", filename); return false;
				}
			}
		} else { UF_EXCEPTION("Image error: not a compressed texture: {}", filename); return false; }
	} else 
#endif
	{
		stbi_set_flip_vertically_on_load(flip);
		uint8_t* buffer = stbi_load( filename.c_str(), &width, &height, &channelsDud, STBI_rgb_alpha );
		size_t len = width * height * channels;
		this->m_pixels.resize( len );
		memcpy( &this->m_pixels[0], buffer, len );
	//	this->m_pixels.insert( this->m_pixels.end(), (uint8_t*) buffer, buffer + len );
		stbi_image_free(buffer);
	}

	this->m_dimensions.x = width;
	this->m_dimensions.y = height;
	this->m_bpp = bit_depth * channels;
	this->m_channels = channels;
	return true;
}
void uf::Image::loadFromBuffer( const Image::pixel_t::type_t* pointer, const pod::Vector2ui& size, std::size_t bit_depth, std::size_t channels, bool flip ) {
	this->m_dimensions = size;
	this->m_bpp = bit_depth * channels;
	this->m_channels = channels;

	size_t len = size.x * size.y * channels;
	this->m_pixels.clear();
	this->m_pixels.resize( len );

	if ( pointer ) memcpy( &this->m_pixels[0], pointer, len );
	else memset( &this->m_pixels[0], 0, len );

	if ( flip ) this->flip();
}
void uf::Image::loadFromBuffer( const Image::container_t& container, const pod::Vector2ui& size, std::size_t bit_depth, std::size_t channels, bool flip ) {
	this->m_dimensions = size;
	this->m_bpp = bit_depth * channels;
	this->m_channels = channels;
	this->m_pixels = container;

	if ( flip ) this->flip();
}
void uf::Image::flip() {
	auto w = this->m_dimensions.x;
	auto h = this->m_dimensions.y;
	uint8_t* pixels = &this->m_pixels[0];
	for (uint j = 0; j * 2 < h; ++j) {
		uint x = j * w * this->m_bpp/8;
		uint y = (h - 1 - j) * w * this->m_bpp/8;
		for (uint i = w * this->m_bpp/8; i > 0; --i) {
			std::swap( pixels[x], pixels[y] );
			++x, ++y;
		}
	}
}
void uf::Image::padToPowerOfTwo(  ) {
	pod::Vector2ui next = {
		this->m_dimensions.x,
		this->m_dimensions.y
	}; {
		next.x--;
		next.x |= next.x >> 1;
		next.x |= next.x >> 2;
		next.x |= next.x >> 4;
		next.x |= next.x >> 8;
		next.x |= next.x >> 16;
		next.x++;
		next.y--;
		next.y |= next.y >> 1;
		next.y |= next.y >> 2;
		next.y |= next.y >> 4;
		next.y |= next.y >> 8;
		next.y |= next.y >> 16;
		next.y++;
	}
	// no point in repadding
	if ( this->m_dimensions.x == next.x && this->m_dimensions.y == next.y ) {
		return;
	}

	uint len = next.x * next.y * this->m_bpp / 8;
	uint8_t* buffer = new uint8_t[len];
	for ( size_t i = 0; i < len; ++i ) buffer[i] = 0;

	for ( size_t y = 0; y < this->m_dimensions.y; ++y ) {
	for ( size_t x = 0; x < this->m_dimensions.x; ++x ) {
		size_t src = x * this->m_channels + this->m_dimensions.x * this->m_channels * y;
		size_t dst = x * this->m_channels + next.x * this->m_channels * y;
		for ( size_t i = 0; i < this->m_channels; ++i ) {
			buffer[dst+i] = this->m_pixels[src+i];
		}
	}
	}

	this->m_dimensions.x = next.x;
	this->m_dimensions.y = next.y;

	this->m_pixels.clear();
	this->m_pixels.insert( this->m_pixels.end(), (uint8_t*) buffer, buffer + len );
	delete[] buffer;
}
// from stream
void uf::Image::open( const std::istream& stream ) {
	// ?
}
// move from vector of pixels
void uf::Image::move( Image::container_t&& move, const Image::vec2_t& size ) {
	this->m_pixels = std::move(move);
	this->m_dimensions = size;
}
// copy from vector of pixels
void uf::Image::copy( const Image::container_t& copy, const Image::vec2_t& size ) {
	this->m_pixels = copy;
	this->m_dimensions = size;
}
void uf::Image::copy( const uf::Image& copy ) {
	this->m_pixels = copy.m_pixels;
	this->m_dimensions = copy.m_dimensions;
	this->m_bpp = copy.m_bpp;
	this->m_channels = copy.m_channels;
	this->m_filename = copy.m_filename;
	this->m_format = copy.m_format;
}
// 	D-tor
// empties pixel container
void uf::Image::clear() {
	this->m_pixels.clear();
#if UF_ENV_DREAMCAST
	this->m_pixels.shrink_to_fit();
#endif
}
uf::Image::container_t& uf::Image::getPixels() {
	return this->m_pixels;	
}
const uf::Image::container_t& uf::Image::getPixels() const {
	return this->m_pixels;	
}
uf::Image::pixel_t::type_t* uf::Image::getPixelsPtr() {
//	return (this->m_pixels.empty() ? NULL : &this->m_pixels[0]);
	return ( this->m_pixels.empty() ) ? NULL : &this->m_pixels[0];
}
const uf::Image::pixel_t::type_t* uf::Image::getPixelsPtr() const {
//	return (this->m_pixels.empty() ? NULL : &this->m_pixels[0]);
	return ( this->m_pixels.empty() ) ? NULL : &this->m_pixels[0];
}
uf::Image::vec2_t& uf::Image::getDimensions() {
	return this->m_dimensions;
}
const uf::Image::vec2_t& uf::Image::getDimensions() const {
	return this->m_dimensions;
}
std::size_t& uf::Image::getBpp() {
	return this->m_bpp;
}
std::size_t uf::Image::getBpp() const {
	return this->m_bpp;
}
std::size_t& uf::Image::getChannels() {
	return this->m_channels;
}
std::size_t uf::Image::getChannels() const {
	return this->m_channels;
}
std::size_t uf::Image::getFormat() const {
	return this->m_format;
}
uf::stl::string uf::Image::getHash() const {
	return uf::string::sha256( this->m_pixels );
}
uf::Image::pixel_t uf::Image::at( const uf::Image::vec2_t& at ) {
	std::size_t i = at.x * this->m_channels + this->m_dimensions.x*this->m_channels*at.y;
	return {
		this->m_pixels[i++],
		this->m_pixels[i++],
		this->m_pixels[i++],
		this->m_pixels[i++],
	};
}
// 	Modifiers
// to file
bool uf::Image::save( const uf::stl::string& filename, bool flip ) const {
	if ( this->m_pixels.empty() ) return false;
	
	uint w = this->m_dimensions.x;
	uint h = this->m_dimensions.y;
	auto* pixels = &this->m_pixels[0];
	uf::stl::string extension = uf::io::extension(filename);
	stbi_flip_vertically_on_write(flip);
	if ( extension == "png" ) {
		stbi_write_png(filename.c_str(), w, h, this->m_channels, &pixels[0], w * this->m_channels);
	} else if ( extension == "jpg" || extension == "jpeg" ) {
		stbi_write_jpg(filename.c_str(), w, h, this->m_channels, &pixels[0], w * this->m_channels);
	}
	return true;
}
// to stream
void uf::Image::save( std::ostream& stream ) const {

}
#include <uf/utils/string/ext.h>
void uf::Image::convert( const uf::stl::string& from, const uf::stl::string& to ) {
	uf::Image::container_t pixels = std::move(this->m_pixels);
	if ( uf::string::lowercase(to) != "rgba" ) {
	} else {
		this->m_pixels.reserve(this->m_dimensions.x * this->m_dimensions.y * 4);
		for ( size_t i = 0; i < this->m_dimensions.x * this->m_dimensions.y * this->m_channels; i += this->m_channels ) {
			if ( uf::string::lowercase(from) == "r" ) {
				this->m_pixels.emplace_back( pixels[i] );
				this->m_pixels.emplace_back( pixels[i] );
				this->m_pixels.emplace_back( pixels[i] );
				this->m_pixels.emplace_back( 0xFF );
			} else if ( uf::string::lowercase(from) == "ra" ) {
				this->m_pixels.emplace_back( pixels[i+0] );
				this->m_pixels.emplace_back( pixels[i+0] );
				this->m_pixels.emplace_back( pixels[i+0] );
				this->m_pixels.emplace_back( pixels[i+1] );
			} else if ( uf::string::lowercase(from) == "rgba" ) {
				this->m_pixels.emplace_back( pixels[i+0] );
				this->m_pixels.emplace_back( pixels[i+1] );
				this->m_pixels.emplace_back( pixels[i+2] );
				this->m_pixels.emplace_back( 0xFF );
			}
		}
	}
	if ( !this->m_pixels.empty() ) {
		this->m_channels = 4;
		this->m_bpp = 8 * this->m_channels;
	} else {
		this->m_pixels = std::move(pixels);
	}
}
// Merges one image on top of another
uf::Image uf::Image::overlay(const Image& top, const Image::vec2_t& corner) const {
	return *this;
}
// Changes all pixel from one color (from), to another (to)
uf::Image uf::Image::replace(const Image::pixel_t& from, const Image::pixel_t& to ) const {
	return *this;
}
// Crops an image
uf::Image uf::Image::subImage( const Image::vec2_t& start, const Image::vec2_t& end) const {
	return *this;
/*
	pod::Vector2ui size = parameter;
	if ( mode == uf::Image::CropMode::START_SPAN_SIZE ) size = parameter;
	if ( mode == uf::Image::CropMode::START_TO_END ) size = parameter - start;
	if ( size > this->size ) return uf::Image(*this);


	uint len = size.product() * this->bpp / 8;
	uint8_t* src = (uint8_t*) this->raw;
	uint8_t* dst = new uint8_t[len];

	uint minX = start.x();
	uint minY = start.y();
	uint dstWidth = size.x();
	uint dstHeight = size.x();
	uint srcWidth = this->size.x();
	uint srcHeight = this->size.y();

	for(uint x = minX; x < dstWidth + minX; x++) {
		for(uint y = minY; y < dstHeight + minY; y++) {
			uint srcOffset = y * srcWidth + x;
			uint dstOffset = (y - minY) * dstWidth + (x - minX);
			srcOffset *= this->bpp / 8;
			dstOffset *= this->bpp / 8;

			for ( uint i = 0; i < this->bpp/4; i++ ) dst[dstOffset+i] = src[srcOffset+i];
		}
	}

	uf::Image image;
	image.raw = (uint8_t*) dst;
	image.len = len;
	image.bpp = this->bpp;
	image.size = size;
	image.format = this->format;
	return image;
*/
}

uf::Image& uf::Image::operator=( const uf::Image& copy ) {
	this->m_pixels = copy.m_pixels;
	this->m_dimensions = copy.m_dimensions;
	this->m_bpp = copy.m_bpp;
	this->m_channels = copy.m_channels;
	this->m_filename = copy.m_filename;
	this->m_format = copy.m_format;
	return *this;
}