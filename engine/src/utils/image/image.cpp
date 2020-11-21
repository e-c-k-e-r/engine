#include <uf/utils/image/image.h>
#include <uf/utils/string/ext.h>
#include <uf/utils/string/hash.h>
#include <uf/utils/io/iostream.h>
#include <fstream> 					// std::fstream
#include <iostream> 				// std::fstream
#include <png/png.h> 				// libpng

#define STB_IMAGE_IMPLEMENTATION
#include <gltf/stb_image.h>

// 	C-tor
// Default
uf::Image::Image() :
	m_bpp(8),
	m_channels(4) {

}
// Just Size
uf::Image::Image( const Image::vec2_t& size ) :
	m_dimensions(size),
	m_bpp(8),
	m_channels(4)
{
	this->m_pixels.reserve(size.x*size.y*this->m_channels);
}
// Move pixels
uf::Image::Image( Image&& move ) : 
	m_pixels(std::move(move.m_pixels)),
	m_dimensions(std::move(move.m_dimensions)),
	m_bpp(move.m_bpp),
	m_channels(move.m_channels),
	m_filename(move.m_filename)
{

}
// Copy pixels
uf::Image::Image( const Image& copy ) :
	m_pixels(copy.m_pixels),
	m_dimensions(copy.m_dimensions),
	m_bpp(copy.m_bpp),
	m_channels(copy.m_channels),
	m_filename(copy.m_filename)
{

}
// Move from vector of pixels
uf::Image::Image( Image::container_t&& move, const Image::vec2_t& size ) :
	m_pixels(std::move(move)),
	m_dimensions(size),
	m_bpp(8),
	m_channels(4)
{

}
// Copy from vector of pixels
uf::Image::Image( const Image::container_t& copy, const Image::vec2_t& size ) :
	m_pixels(copy),
	m_dimensions(size),
	m_bpp(8),
	m_channels(4)
{

}

std::string uf::Image::getFilename() const {
	return this->m_filename;
}

// from file
bool uf::Image::open( const std::string& filename ) {
	if ( !uf::io::exists(filename) ) throw std::runtime_error("does not exist: " + filename);
	this->m_filename = filename;
	this->m_pixels.clear();
	std::string extension = uf::io::extension(filename);
	if ( extension == "png" ) {
		uint mode = 2;
		#if 0
			png_byte header[8];
			FILE* file = fopen(filename.c_str(), "rb");

			fread(header, 1, 8, file);
			if (png_sig_cmp(header, 0, 8)) {
				fclose(file);
			//	uf::iostream << "[Error] Not a PNG file" << "\n";
			//	return false;
				throw std::runtime_error(filename + " not a png");
			}

			png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
			if ( !png_ptr ) {
				uf::iostream << "[Error] Failed to create PNG read struct" << "\n";
				return false;
			}
			png_infop info_ptr = png_create_info_struct(png_ptr);
			if ( !info_ptr ) {
				uf::iostream << "[Error] Failed to create PNG info struct" << "\n";
				png_destroy_read_struct(&png_ptr, (png_infopp) NULL, (png_infopp) NULL);
				return false;
			}
			
			uint8_t* buffer = NULL;
			png_bytep* row_ptrs = NULL;

			if (setjmp(png_jmpbuf(png_ptr))) {
				uf::iostream << "[Error] Error" << "\n";
				png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);
					if (row_ptrs != NULL) delete [] row_ptrs;
				if (buffer != NULL) delete [] buffer;
				return false;
			}

			png_init_io(png_ptr, file);
			png_set_sig_bytes(png_ptr, 8);
			png_read_info(png_ptr, info_ptr);

			png_uint_32 bit_depth = png_get_bit_depth(png_ptr, info_ptr);
			png_uint_32 color_type = png_get_color_type(png_ptr, info_ptr);
			png_uint_32 width = png_get_image_width(png_ptr, info_ptr);
			png_uint_32 height = png_get_image_height(png_ptr, info_ptr);
			png_uint_32 channels = png_get_channels(png_ptr, info_ptr);

			switch (color_type) {
				case PNG_COLOR_TYPE_PALETTE:
					png_set_palette_to_rgb(png_ptr);
					channels = 3;
					break;
				case PNG_COLOR_TYPE_GRAY:
					if (bit_depth < 8) png_set_expand_gray_1_2_4_to_8(png_ptr);
					bit_depth = 8;
				break;
			}

			if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
				png_set_tRNS_to_alpha(png_ptr);
				channels+=1;
			}
			if (bit_depth == 16) png_set_strip_16(png_ptr);

			uint len = width * height * bit_depth * channels / 8;
			row_ptrs = new png_bytep[height];
			buffer = new uint8_t[len];
			const uint stride = height * bit_depth * channels / 8;

			for ( std::size_t i = 0; i < height; i++ )
				row_ptrs[i] = ((png_bytep) buffer) + ((height - i - 1) * stride);

			png_read_image(png_ptr, row_ptrs);

			delete[] (png_bytep) row_ptrs;	
			png_destroy_read_struct(&png_ptr, &info_ptr,(png_infopp) NULL);
			fclose(file);

			this->m_dimensions.x = width;
			this->m_dimensions.y = height;
			this->m_bpp = bit_depth * channels;
			this->m_channels = channels;
			this->m_pixels.insert( this->m_pixels.end(), buffer, buffer + len );
		/*
			this->size = pod::Vector2i(width, height);
			this->len = len;
			this->bpp = bit_depth * channels;
			this->format = uf::Image::Format::RGBA;
			this->m_pixels = (uf::Image::raw_t) buffer;
		
			if ( channels == 3 ) {
				this->format = uf::Image::Format::RGB;
		//		this->convert(uf::Image::Format::RGBA, 32);
			}
		*/
		#else
			int width, height, channels, bit_depth;
			bit_depth = 8;
			stbi_set_flip_vertically_on_load(true);
			unsigned char* buffer = stbi_load(
				filename.c_str(),
				&width,
				&height,
				&channels,
				STBI_rgb_alpha
			);
			uint len = width * height * channels;
			this->m_dimensions.x = width;
			this->m_dimensions.y = height;
			this->m_bpp = bit_depth * channels;
			this->m_channels = channels;
			this->m_pixels.insert( this->m_pixels.end(), (uint8_t*) buffer, buffer + len );
			stbi_image_free(buffer);
		#endif
		return true;
	}
	return true;
}
void uf::Image::loadFromBuffer( const Image::pixel_t::type_t* pointer, const pod::Vector2ui& size, std::size_t bit_depth, std::size_t channels, bool flip ) {
	this->m_dimensions = size;
	this->m_bpp = bit_depth * channels;
	this->m_channels = channels;

	size_t len = size.x * size.y * channels;
	this->m_pixels.clear();
	this->m_pixels.resize( len );
	//for ( size_t i = 0; i < len; ++i ) this->m_pixels[i] = pointer[i];
	if ( pointer ) {
		memcpy( &this->m_pixels[0], pointer, len );
	} else {
		memset( &this->m_pixels[0], 0, len );
	}

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
// 	D-tor
uf::Image::~Image() {
	this->clear();
}
// empties pixel container
void uf::Image::clear() {
	this->m_pixels.clear();
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
std::string uf::Image::getHash() const {
	return uf::string::sha256( this->m_pixels );
}
uf::Image::pixel_t uf::Image::at( const uf::Image::vec2_t& at ) {
	std::size_t i = at.x * this->m_channels + this->m_dimensions.x*this->m_channels*at.y;
	uf::Image::pixel_t pixel;
	pixel.r = this->m_pixels[i++];
	pixel.g = this->m_pixels[i++];
	pixel.b = this->m_pixels[i++];
	pixel.a = this->m_pixels[i++];
	return pixel;
}
// 	Modifiers
// to file
bool uf::Image::save( const std::string& filename, bool flip ) {
	uint w = this->m_dimensions.x;
	uint h = this->m_dimensions.y;
	uint8_t* pixels = &this->m_pixels[0];

	if ( flip )
		for (uint j = 0; j * 2 < h; ++j) {
			uint x = j * w * this->m_bpp/8;
			uint y = (h - 1 - j) * w * this->m_bpp/8;
			for (uint i = w * this->m_bpp/8; i > 0; --i) {
				std::swap( pixels[x], pixels[y] );
				++x, ++y;
			/*
				uint8_t tmp = pixels[x];
				pixels[x] = pixels[y];
				pixels[y] = tmp;
				++x;
				++y;
			*/
			}
		}


	png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png)
		return false;

	png_infop info = png_create_info_struct(png);
	if (!info) {
		png_destroy_write_struct(&png, &info);
		return false;
	}

	FILE *fp = fopen(filename.c_str(), "wb");
	if (!fp) {
		png_destroy_write_struct(&png, &info);
		return false;
	}

	png_init_io(png, fp);
	png_set_IHDR(png, info, w, h, 8 /* depth */, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
	if ( this->m_channels == 4 ) png_set_IHDR(png, info, w, h, 8 /* depth */, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
	if ( this->m_channels == 3 ) png_set_IHDR(png, info, w, h, 8 /* depth */, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
	png_colorp palette = (png_colorp)png_malloc(png, PNG_MAX_PALETTE_LENGTH * sizeof(png_color));
	if (!palette) {
		fclose(fp);
		png_destroy_write_struct(&png, &info);
		return false;
	}
	png_set_PLTE(png, info, palette, PNG_MAX_PALETTE_LENGTH);
	png_write_info(png, info);
	png_set_packing(png);

	png_bytepp rows = (png_bytepp)png_malloc(png, h * sizeof(png_bytep));
	for (uint i = 0; i < h; ++i)
		rows[i] = (png_bytep)(pixels + (h - i - 1) * w * this->m_bpp/8);

	png_write_image(png, rows);
	png_write_end(png, info);
	png_free(png, palette);
	png_destroy_write_struct(&png, &info);

	fclose(fp);
	delete[] rows;
	return true;
}
// to stream
void uf::Image::save( std::ostream& stream ) {

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
	uf::Vector2ui size = parameter;
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
	return *this;
}