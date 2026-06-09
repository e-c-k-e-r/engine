#include <uf/utils/image/image.h>
#include <uf/utils/string/ext.h>
#include <uf/utils/string/hash.h>
#include <uf/utils/io/iostream.h>
#include <uf/utils/io/vfs.h>
#include <fstream> 					// std::fstream
#include <iostream> 				// std::fstream
#include <png/png.h> 				// libpng

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <stb/stb_image_write.h>
#include <uf/utils/renderer/renderer.h>
#include <uf/utils/string/ext.h>

namespace {
	void stbi_buffer_write_func(void *context, void *data, int size) {
		auto* buffer = static_cast<uf::stl::vector<uint8_t>*>(context);
		auto* bytes = static_cast<uint8_t*>(data);
		buffer->insert(buffer->end(), bytes, bytes + size);
	}
}

namespace impl {
	pod::Image scaleNearest( const pod::Image& image, const pod::Vector2ui& size ) {
		pod::Image out = image;
		out.size = size;
		out.pixels.assign(size.x * size.y * image.channels, 0);

		const float xRatio = (float)(image.size.x) / size.x;
		const float yRatio = (float)(image.size.y) / size.y;
		const size_t channels = image.channels;
		const size_t srcWidth = image.size.x;
		const size_t srcHeight = image.size.y;
		
		for ( auto j = 0; j < size.y; ++j ) {
			const size_t srcY = std::min((size_t)((j + 0.5f) * yRatio), srcHeight - 1);
			for ( auto i = 0; i < size.x; ++i ) {
				const size_t srcX = std::min((size_t)((i + 0.5f) * xRatio), srcWidth - 1);
				const size_t srcIdx = (srcY * srcWidth + srcX) * channels;
				const size_t dstIdx = (j * size.x + i) * channels;

				for ( size_t c = 0; c < channels; ++c )
					out.pixels[dstIdx + c] = image.pixels[srcIdx + c];
			}
		}

		return out;
	}

	pod::Image scaleBilinear( const pod::Image& image, const pod::Vector2ui& size ) {
		pod::Image out = image;
		out.size = size;
		out.pixels.assign(size.x * size.y * image.channels, 0);

		const float xRatio = (float)(image.size.x) / size.x;
		const float yRatio = (float)(image.size.y) / size.y;
		const size_t channels = image.channels;
		const size_t srcWidth = image.size.x;
		const size_t srcHeight = image.size.y;

		for ( auto j = 0; j < size.y; ++j ) {
			const float gy = (j + 0.5f) * yRatio - 0.5f;
			const int y0 = (int)(std::floor(gy));

			const size_t sy0 = std::clamp<int>(y0, 0, srcHeight - 1);
			const size_t sy1 = std::clamp<int>(y0 + 1, 0, srcHeight - 1);
			const float v = gy - y0;

			for ( auto i = 0; i < size.x; ++i ) {
				const float gx = (i + 0.5f) * xRatio - 0.5f;
				const int x0 = (int)(std::floor(gx));

				const size_t sx0 = std::clamp<int>(x0, 0, srcWidth - 1);
				const size_t sx1 = std::clamp<int>(x0 + 1, 0, srcWidth - 1);
				const float u = gx - x0;

				const size_t dstIdx = (j * size.x + i) * channels;

				for ( auto c = 0; c < channels; ++c ) {
					float p00 = image.pixels[(sy0 * srcWidth + sx0) * channels + c];
					float p10 = image.pixels[(sy0 * srcWidth + sx1) * channels + c];
					float p01 = image.pixels[(sy1 * srcWidth + sx0) * channels + c];
					float p11 = image.pixels[(sy1 * srcWidth + sx1) * channels + c];

					float val = (1.0f - u) * (1.0f - v) * p00 + u * (1.0f - v) * p10 +
								(1.0f - u) * v * p01 + u * v * p11;

					out.pixels[dstIdx + c] = (uint8_t)(std::clamp(val + 0.5f, 0.0f, 255.0f));
				}
			}
		}

		return out;
	}
}


bool uf::image::open( pod::Image& image, const uf::stl::string& filename, bool flip ) {
	uf::stl::vector<uint8_t> buffer;
	if ( !uf::io::readAsBuffer( buffer, filename ) ) {
		return false;
	}
#if UF_USE_OPENGL_GLDC
	 auto extension = uf::io::extension( filename );
	if ( extension != "dtex" ) UF_MSG_WARNING("non-dtex loading is highly discouraged on this platform: {}", filename);
#endif
	
	image.filename = filename;
	image.pixels.clear();
	int width = 0, height = 0, channelsDud = 0, bpp = 8, channels = 4;
#if UF_USE_OPENGL_GLDC
	if ( extension == "dtex" ) {
		struct {
			char		id[4]; // 'DTEX'
			uint16_t 	width;
			uint16_t 	height;
			uint32_t 	type;
			uint32_t 	size;
		} header;

		uf::stl::vector<uint8_t> buffer;
		if ( !uf::io::readAsBuffer( buffer, filename ) ) UF_EXCEPTION("IO error: could not read DTEX: {}", filename);
		if ( buffer.size() < sizeof(header) ) UF_EXCEPTION("IO error: DTEX file is too small to contain a header: {}", filename);
		memcpy(&header, buffer.data(), sizeof(header));
		if ( buffer.size() < sizeof(header) + header.size ) UF_EXCEPTION("IO error: DTEX file is truncated or corrupted: {}", filename);

		image.pixels.resize(header.size);
		memcpy(image.pixels.data(), buffer.data() + sizeof(header), header.size);

		bool twiddled = (header.type & (1 << 26)) < 1;
		bool compressed = (header.type & (1 << 30)) > 0;
		bool mipmapped = (header.type & (1 << 31)) > 0;
		bool strided = (header.type & (1 << 25)) > 0;
		uint32_t format = (header.type >> 27) & 0b111;
		width = header.width;
		height = header.height;

		uint32_t expected = 2 * header.width * header.height;
		uint32_t ratio = (uint32_t) (((float) expected) / ((float) header.size));
		bpp = 4;
		image.format = format;
		if ( compressed ) {
			if ( twiddled ) {
				switch ( format ) {
					case 1: image.format = mipmapped ? GL_COMPRESSED_RGB_565_VQ_MIPMAP_TWID_KOS : GL_COMPRESSED_RGB_565_VQ_TWID_KOS; channels = 3; break;
					case 0: image.format = mipmapped ? GL_COMPRESSED_ARGB_1555_VQ_MIPMAP_TWID_KOS : GL_COMPRESSED_ARGB_1555_VQ_TWID_KOS; break;
					case 2: image.format = mipmapped ? GL_COMPRESSED_ARGB_4444_VQ_MIPMAP_TWID_KOS : GL_COMPRESSED_ARGB_4444_VQ_TWID_KOS; break;
					default: UF_EXCEPTION("Image error: invalid texture format: {}", filename); return false;
				}
			} else {
				switch ( format ) {
					case 1: image.format = mipmapped ? GL_COMPRESSED_RGB_565_VQ_MIPMAP_KOS : GL_COMPRESSED_RGB_565_VQ_KOS; channels = 3; break;
					case 0: image.format = mipmapped ? GL_COMPRESSED_ARGB_1555_VQ_MIPMAP_KOS : GL_COMPRESSED_ARGB_1555_VQ_KOS; break;
					case 2: image.format = mipmapped ? GL_COMPRESSED_ARGB_4444_VQ_MIPMAP_KOS : GL_COMPRESSED_ARGB_4444_VQ_KOS; break;
					default: UF_EXCEPTION("Image error: invalid texture format: {}", filename); return false;
				}
			}
		} else { UF_EXCEPTION("Image error: not a compressed texture: {}", filename); return false; }
	} else 
#endif
	{
		stbi_set_flip_vertically_on_load(flip);
		uint8_t* stbi_pixels = stbi_load_from_memory( buffer.data(), buffer.size(), &width, &height, &channelsDud, STBI_rgb_alpha );
		size_t len = width * height * channels;
		image.pixels.resize( len );
		memcpy( &image.pixels[0], stbi_pixels, len );
	}

	image.size.x = width;
	image.size.y = height;
	image.bpp = bpp * channels;
	image.channels = channels;
	return true;
}
void uf::image::clear( pod::Image& image ) {
	image.pixels.clear();
#if UF_ENV_DREAMCAST
	image.pixels.shrink_to_fit();
#endif
}

void uf::image::load( pod::Image& image, const pod::Image::pixel_t::type_t* pointer, const pod::Vector2ui& size, size_t bpp, size_t channels, bool flip ) {
	image.size = size;
	image.bpp = bpp * channels;
	image.channels = channels;

	size_t len = size.x * size.y * channels;
	UF_ASSERT( len > 0 );
	image.pixels.clear();
	image.pixels.resize( len );

	if ( pointer ) memcpy( &image.pixels[0], pointer, len );
	else memset( &image.pixels[0], 0, len );

	if ( flip ) uf::image::flip( image );
}
void uf::image::load( pod::Image& image, const pod::Image::container_t& container, const pod::Vector2ui& size, size_t bpp, size_t channels, bool flip ) {
	image.size = size;
	image.bpp = bpp * channels;
	image.channels = channels;
	image.pixels = container;

	if ( flip ) uf::image::flip( image );
}
bool uf::image::save( const pod::Image& image, const uf::stl::string& filename, bool flip ) {
	if ( image.pixels.empty() ) return false;
	
	uint w = image.size.x;
	uint h = image.size.y;
	auto* pixels = &image.pixels[0];
	uf::stl::string extension = uf::io::extension(filename);
	stbi_flip_vertically_on_write(flip);
	
	uf::stl::vector<uint8_t> buffer;

	if ( extension == "png" ) {
		stbi_write_png_to_func(stbi_buffer_write_func, &buffer, w, h, image.channels, pixels, w * image.channels);
	} else if ( extension == "jpg" || extension == "jpeg" ) {
		stbi_write_jpg_to_func(stbi_buffer_write_func, &buffer, w, h, image.channels, pixels, 90); // 90 is quality
	} else {
		UF_MSG_ERROR("Unsupported image save format: {}", extension);
		return false;
	}

	if ( buffer.empty() ) return false;
	return uf::vfs::write( filename, buffer.data(), buffer.size() ) > 0;
}
void uf::image::save( const pod::Image& image, std::ostream& stream ) {

}

pod::Image::pixel_t uf::image::at( pod::Image& image, const pod::Vector2ui& at ) {
	size_t i = at.x * image.channels + image.size.x * image.channels * at.y;
	return {
		image.pixels[i++],
		image.pixels[i++],
		image.pixels[i++],
		image.pixels[i++],
	};
}

uf::stl::string uf::image::hash( const pod::Image& image ) {
	return uf::string::sha256( image.pixels );
}

void uf::image::flip( pod::Image& image ) {
	auto w = image.size.x;
	auto h = image.size.y;
	uint8_t* pixels = &image.pixels[0];
	for ( uint j = 0; j * 2 < h; ++j ) {
		uint x = j * w * image.bpp/8;
		uint y = (h - 1 - j) * w * image.bpp/8;
		for ( uint i = w * image.bpp/8; i > 0; --i ) {
			std::swap( pixels[x], pixels[y] );
			++x, ++y;
		}
	}
}
void uf::image::padToPowerOfTwo( pod::Image& image ) {
	pod::Vector2ui next = {
		image.size.x,
		image.size.y
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
	if ( image.size.x == next.x && image.size.y == next.y ) {
		return;
	}

	uint len = next.x * next.y * image.bpp / 8;
	uint8_t* buffer = new uint8_t[len];
	for ( size_t i = 0; i < len; ++i ) buffer[i] = 0;

	for ( size_t y = 0; y < image.size.y; ++y ) {
	for ( size_t x = 0; x < image.size.x; ++x ) {
		size_t src = x * image.channels + image.size.x * image.channels * y;
		size_t dst = x * image.channels + next.x * image.channels * y;
		for ( size_t i = 0; i < image.channels; ++i ) {
			buffer[dst+i] = image.pixels[src+i];
		}
	}
	}

	image.size.x = next.x;
	image.size.y = next.y;

	image.pixels.clear();
	image.pixels.insert( image.pixels.end(), (uint8_t*) buffer, buffer + len );
	delete[] buffer;
}
void uf::image::convert( pod::Image& image, const uf::stl::string& from, const uf::stl::string& to ) {
/*
	pod::Image::container_t pixels = std::move(image.pixels);
	if ( uf::string::lowercase(to) != "rgba" ) {
	} else {
		image.pixels.reserve(image.size.x * image.size.y * 4);
		for ( size_t i = 0; i < image.size.x * image.size.y * image.channels; i += image.channels ) {
			if ( uf::string::lowercase(from) == "r" ) {
				image.pixels.emplace_back( pixels[i] );
				image.pixels.emplace_back( pixels[i] );
				image.pixels.emplace_back( pixels[i] );
				image.pixels.emplace_back( 0xFF );
			} else if ( uf::string::lowercase(from) == "ra" ) {
				image.pixels.emplace_back( pixels[i+0] );
				image.pixels.emplace_back( pixels[i+0] );
				image.pixels.emplace_back( pixels[i+0] );
				image.pixels.emplace_back( pixels[i+1] );
			} else if ( uf::string::lowercase(from) == "rgba" ) {
				image.pixels.emplace_back( pixels[i+0] );
				image.pixels.emplace_back( pixels[i+1] );
				image.pixels.emplace_back( pixels[i+2] );
				image.pixels.emplace_back( 0xFF );
			}
		}
	}
	if ( !image.pixels.empty() ) {
		image.channels = 4;
		image.bpp = 8 * image.channels;
	} else {
		image.pixels = std::move(pixels);
	}
*/
}
pod::Image uf::image::overlay( const pod::Image& image, const pod::Image& top, const pod::Vector2ui& corner ) {
	Image out = image;
	for (size_t y = 0; y < top.size.y; ++y) {
		for (size_t x = 0; x < top.size.x; ++x) {
			size_t dstX = corner.x + x;
			size_t dstY = corner.y + y;
			if (dstX >= image.size.x || dstY >= image.size.y) continue;
			size_t dstIdx = (dstY*image.size.x + dstX) * image.channels;
			size_t srcIdx = (y*top.size.x + x) * top.channels;

			float alpha = top.pixels[srcIdx+3] / 255.0f;
			for (size_t c = 0; c < 3; ++c) {
				out.pixels[dstIdx+c] =
					static_cast<uint8_t>( (1-alpha)*out.pixels[dstIdx+c] +
										   alpha*top.pixels[srcIdx+c] );
			}
			out.pixels[dstIdx+3] = 255;
		}
	}
	return out;
}
pod::Image uf::image::replace( const pod::Image& image, const pod::Image::pixel_t& from, const pod::Image::pixel_t& to ) {
	Image out = image;
	for ( auto i = 0; i < out.pixels.size(); i += out.channels ) {
		if (out.pixels[i]   == from[0] &&
			out.pixels[i+1] == from[1] &&
			out.pixels[i+2] == from[2] &&
			out.pixels[i+3] == from[3]) {
			out.pixels[i]   = to[0];
			out.pixels[i+1] = to[1];
			out.pixels[i+2] = to[2];
			out.pixels[i+3] = to[3];
		}
	}
	return out;
}
pod::Image uf::image::subImage( const pod::Image& image, const pod::Vector2ui& start, const pod::Vector2ui& end ) {
	pod::Image out = image;
	out.size = { end.x - start.x, end.y - start.y };
	out.pixels.assign(out.size.x * out.size.y * image.channels, 0);
	for (size_t y = 0; y < out.size.y; ++y) {
		for (size_t x = 0; x < out.size.x; ++x) {
			size_t dstIdx = (y*out.size.x + x) * image.channels;
			size_t srcIdx = ((start.y+y)*out.size.x + (start.x+x)) * image.channels;
			for (size_t c = 0; c < image.channels; ++c)
				out.pixels[dstIdx+c] = image.pixels[srcIdx+c];
		}
	}
	return out;
}
pod::Image uf::image::scale( const pod::Image& image, const pod::Vector2ui& size, const uf::stl::string& _filter ) {
	auto filter = uf::string::lowercase( _filter );
	if ( filter == "nearest" ) return impl::scaleNearest( image, size );
	if ( filter == "linear" || filter == "bilinear" ) return impl::scaleBilinear( image, size );\
	UF_EXCEPTION("unrecognized scale filter: {}", filter );
}
/*
uf::Image::Image() {
	size = {0,0};
	bpp = 8;
	channels = 4;
	format = 0;
}

uf::Image::Image(const pod::Vector2ui& s) {
	size = s;
	bpp = 8;
	channels = 4;
	format = 0;
	pixels.resize(size.x * size.y * channels);
}

uf::Image::Image( pod::Image::container_t&& move, const pod::Vector2ui& s ) {
	pixels = std::move( move );
	size = s;
	bpp = 8;
	channels = 4;
	format = 0;
}

uf::Image::Image( const pod::Image::container_t& copy, const pod::Vector2ui& s ) {
	pixels = copy;
	size = s;
	bpp = 8;
	channels = 4;
	format = 0;
}

uf::Image::Image( const uf::Image& copy ) {
	this->copy( copy );
}

uf::Image::Image( uf::Image&& move ) noexcept {
	//this->move( move );
	pixels = std::move( move.pixels );
	size = move.size;
	bpp = move.bpp;
	channels = move.channels;
	format = move.format;
}

uf::Image& uf::Image::operator=( const uf::Image& copy ) {
	this->copy( copy );
	return *this;
}

uf::Image& uf::Image::operator=( uf::Image&& move ) noexcept {
	//this->move( move );
	pixels = std::move( move.pixels );
	size = move.size;
	bpp = move.bpp;
	channels = move.channels;
	format = move.format;
	return *this;
}
*/
uf::stl::string uf::Image::getFilename() const {
	return this->filename;
}
void uf::Image::setFilename( const uf::stl::string& filename ) {
	this->filename = filename;
}

// from file
bool uf::Image::open( const uf::stl::string& filename, bool flip ) {
	return uf::image::open( *this, filename, flip );
}
void uf::Image::loadFromBuffer( const pod::Image::pixel_t::type_t* pointer, const pod::Vector2ui& size, size_t bpp, size_t channels, bool flip ) {
	return uf::image::load( *this, pointer, size, bpp, channels, flip );
}
void uf::Image::loadFromBuffer( const pod::Image::container_t& container, const pod::Vector2ui& size, size_t bpp, size_t channels, bool flip ) {
	return uf::image::load( *this, container, size, bpp, channels, flip );
}
void uf::Image::flip() {
	return uf::image::flip( *this );
}
void uf::Image::padToPowerOfTwo(  ) {
	return uf::image::padToPowerOfTwo( *this );
}
// from stream
void uf::Image::open( const std::istream& stream ) {
	//return uf::image::open( *this, stream );
}
// move from vector of pixels
void uf::Image::move( pod::Image::container_t&& move, const pod::Vector2ui& size ) {
	this->pixels = std::move(move);
	this->size = size;
}
void uf::Image::move( uf::Image&& move ) {
	this->pixels = std::move(move.pixels);
	this->size = move.size;
	this->bpp = move.bpp;
	this->channels = move.channels;
	this->filename = move.filename;
	this->format = move.format;
}
// copy from vector of pixels
void uf::Image::copy( const pod::Image::container_t& copy, const pod::Vector2ui& size ) {
	this->pixels = copy;
	this->size = size;
}
void uf::Image::copy( const uf::Image& copy ) {
	this->pixels = copy.pixels;
	this->size = copy.size;
	this->bpp = copy.bpp;
	this->channels = copy.channels;
	this->filename = copy.filename;
	this->format = copy.format;
}
// 	D-tor
// empties pixel container
void uf::Image::clear() {
	uf::image::clear( *this );
}
pod::Image::container_t& uf::Image::getPixels() {
	return this->pixels;	
}
const pod::Image::container_t& uf::Image::getPixels() const {
	return this->pixels;	
}
pod::Image::pixel_t::type_t* uf::Image::getPixelsPtr() {
//	return (this->pixels.empty() ? NULL : &this->pixels[0]);
	return ( this->pixels.empty() ) ? NULL : &this->pixels[0];
}
const pod::Image::pixel_t::type_t* uf::Image::getPixelsPtr() const {
//	return (this->pixels.empty() ? NULL : &this->pixels[0]);
	return ( this->pixels.empty() ) ? NULL : &this->pixels[0];
}
pod::Vector2ui& uf::Image::getDimensions() {
	return this->size;
}
const pod::Vector2ui& uf::Image::getDimensions() const {
	return this->size;
}
size_t& uf::Image::getBpp() {
	return this->bpp;
}
size_t uf::Image::getBpp() const {
	return this->bpp;
}
size_t& uf::Image::getChannels() {
	return this->channels;
}
size_t uf::Image::getChannels() const {
	return this->channels;
}
size_t uf::Image::getFormat() const {
	return this->format;
}
uf::stl::string uf::Image::getHash() const {
	return uf::image::hash( *this );
}
pod::Image::pixel_t uf::Image::at( const pod::Vector2ui& at ) {
	return uf::image::at( *this, at );
}

// 	Modifiers
// to file
bool uf::Image::save( const uf::stl::string& filename, bool flip ) const {
	return uf::image::save( *this, filename, flip );
}
// to stream
void uf::Image::save( std::ostream& stream ) const {
	return uf::image::save( *this, stream );
}
void uf::Image::convert( const uf::stl::string& from, const uf::stl::string& to ) {
	return uf::image::convert( *this, from, to );
}
// Merges one image on top of another
uf::Image uf::Image::overlay(const Image& top, const pod::Vector2ui& corner) const {
	return uf::image::overlay( *this, top, corner );
}
// Changes all pixel from one color (from), to another (to)
uf::Image uf::Image::replace(const pod::Image::pixel_t& from, const pod::Image::pixel_t& to ) const {
	return uf::image::replace( *this, from, to );
}
// Crops an image
uf::Image uf::Image::subImage( const pod::Vector2ui& start, const pod::Vector2ui& end) const {
	return uf::image::subImage( *this, start, end );
}
// Scales an image, nearest = true does nearest neighbor, nearest = false does bilinear interpolation
uf::Image uf::Image::scale( const pod::Vector2ui& newSize, const uf::stl::string& filter ) const {
	return uf::image::scale( *this, newSize, filter );
}