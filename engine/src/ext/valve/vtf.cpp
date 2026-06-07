#include <uf/ext/valve/bsp.h>
#include <uf/ext/valve/mdl.h>
#include <uf/ext/valve/vtf.h>
#include <uf/ext/valve/vpk.h>
#include <uf/ext/valve/common.h>

namespace impl {
	constexpr uint32_t IMAGE_FORMAT_RGBA8888 = 0;
	constexpr uint32_t IMAGE_FORMAT_BGR888 = 3;
	constexpr uint32_t IMAGE_FORMAT_BGRA8888 = 12;
	constexpr uint32_t IMAGE_FORMAT_DXT1 = 13;
	constexpr uint32_t IMAGE_FORMAT_DXT5 = 15;

#pragma pack(push, 1)
	struct VTFHeader {
		char signature[4]; // "VTF\0"
		uint32_t version[2];
		uint32_t headerSize;
		uint16_t width, height;
		uint32_t flags;
		uint16_t frames;
		uint16_t firstFrame;
		uint8_t padding0[4];
		float reflectivity[3];
		uint8_t padding1[4];
		float bumpmapScale;
		uint32_t highResImageFormat;
		uint8_t mipmapCount;
		uint32_t lowResImageFormat;
		uint8_t lowResImageWidth;
		uint8_t lowResImageHeight;
	};
#pragma pack(pop)

	// to-do: cram this inside the image functions
	inline void decodeRGB565( uint16_t color, uint8_t& r, uint8_t& g, uint8_t& b ) {
		r = (uint8_t)(((color >> 11) & 0x1F) * 255 / 31);
		g = (uint8_t)(((color >> 5) & 0x3F) * 255 / 63);
		b = (uint8_t)((color & 0x1F) * 255 / 31);
	}

	void decompressDXT1Block( const uint8_t* block, uint8_t* out, int x, int y, int width, int height ) {
		uint16_t color0 = *(const uint16_t*)(block + 0);
		uint16_t color1 = *(const uint16_t*)(block + 2);
		uint32_t indices = *(const uint32_t*)(block + 4);

		uint8_t r[4], g[4], b[4];
		decodeRGB565(color0, r[0], g[0], b[0]);
		decodeRGB565(color1, r[1], g[1], b[1]);

		if (color0 > color1) {
			r[2] = (2 * r[0] + r[1]) / 3; g[2] = (2 * g[0] + g[1]) / 3; b[2] = (2 * b[0] + b[1]) / 3;
			r[3] = (r[0] + 2 * r[1]) / 3; g[3] = (g[0] + 2 * g[1]) / 3; b[3] = (b[0] + 2 * b[1]) / 3;
		} else {
			r[2] = (r[0] + r[1]) / 2; g[2] = (g[0] + g[1]) / 2; b[2] = (b[0] + b[1]) / 2;
			r[3] = 0; g[3] = 0; b[3] = 0;
		}

		for ( int py = 0; py < 4; ++py ) {
			for ( int px = 0; px < 4; ++px ) {
				if (x + px >= width || y + py >= height) continue;
				uint8_t idx = (indices >> ((py * 4 + px) * 2)) & 0x03;

				int offset = ((y + py) * width + (x + px)) * 4;
				out[offset + 0] = r[idx];
				out[offset + 1] = g[idx];
				out[offset + 2] = b[idx];
				out[offset + 3] = (color0 <= color1 && idx == 3) ? 0 : 255;
			}
		}
	}

	void decompressDXT5Block(const uint8_t* block, uint8_t* out, int x, int y, int width, int height) {
		uint8_t a0 = block[0];
		uint8_t a1 = block[1];
		uint64_t alphaIndices = (*(const uint64_t*)block) >> 16;

		uint8_t alphas[8];
		alphas[0] = a0;
		alphas[1] = a1;
		if (a0 > a1) {
			for (int i = 0; i < 6; ++i) alphas[2 + i] = ((6 - i) * a0 + (i + 1) * a1) / 7;
		} else {
			for (int i = 0; i < 4; ++i) alphas[2 + i] = ((4 - i) * a0 + (i + 1) * a1) / 5;
			alphas[6] = 0;
			alphas[7] = 255;
		}

		uint16_t color0 = *(const uint16_t*)(block + 8);
		uint16_t color1 = *(const uint16_t*)(block + 10);
		uint32_t colorIndices = *(const uint32_t*)(block + 12);

		uint8_t r[4], g[4], b[4];
		decodeRGB565(color0, r[0], g[0], b[0]);
		decodeRGB565(color1, r[1], g[1], b[1]);

		r[2] = (2 * r[0] + r[1]) / 3; g[2] = (2 * g[0] + g[1]) / 3; b[2] = (2 * b[0] + b[1]) / 3;
		r[3] = (r[0] + 2 * r[1]) / 3; g[3] = (g[0] + 2 * g[1]) / 3; b[3] = (b[0] + 2 * b[1]) / 3;

		for ( int py = 0; py < 4; ++py ) {
			for ( int px = 0; px < 4; ++px ) {
				if (x + px >= width || y + py >= height) continue;

				int bitOffset = (py * 4 + px) * 3;
				uint8_t alphaIdx = (alphaIndices >> bitOffset) & 0x07;
				uint8_t colorIdx = (colorIndices >> ((py * 4 + px) * 2)) & 0x03;

				int offset = ((y + py) * width + (x + px)) * 4;
				out[offset + 0] = r[colorIdx];
				out[offset + 1] = g[colorIdx];
				out[offset + 2] = b[colorIdx];
				out[offset + 3] = alphas[alphaIdx];
			}
		}
	}
}

bool ext::valve::loadVmt( uf::Serializer& dict, const uf::stl::string& filename ) {
	uf::stl::string content;
	if ( !uf::io::readAsString(content, filename) ) return false;

	uf::stl::string line;
	std::istringstream file(content);
	while ( std::getline(file, line) ) {
		uf::stl::string comment = "";
		size_t commentPos = line.find("//"); // strip comments
		if ( commentPos != uf::stl::string::npos ) {
			comment = line.substr(commentPos);
			line = line.substr(0, commentPos);
		}

		uf::stl::string key, value;
		if ( impl::parseKeyValue(line, key, value) ) {
			std::transform(key.begin(), key.end(), key.begin(), ::tolower);
			std::transform(value.begin(), value.end(), value.begin(), ::tolower);
			std::replace(value.begin(), value.end(), '\\', '/');

			if ( key == "include" ) {
                ext::valve::loadVmt( dict, value );
            } else {
				dict[key] = impl::processValue( value );
            }
		}
	}
	return true;
}

bool ext::valve::loadVtf( pod::Image& image, const uf::stl::string& filename ) {
	uf::stl::vector<uint8_t> buffer;
	if ( !uf::io::readAsBuffer(buffer, filename) ) return false;

	const impl::VTFHeader* header = (const impl::VTFHeader*)(buffer.data());
	if ( strncmp(header->signature, "VTF", 3) != 0 ) return false;

	size_t offset = header->headerSize;
	if ( header->lowResImageFormat != 0xFFFFFFFF ) {
		offset += std::max<size_t>(1, (header->lowResImageWidth * header->lowResImageHeight) / 2);
	}

	for ( int mip = header->mipmapCount - 1; mip > 0; --mip ) {
		int mipWidth = std::max(1, header->width >> mip);
		int mipHeight = std::max(1, header->height >> mip);

		if ( header->highResImageFormat == impl::IMAGE_FORMAT_DXT1 ) {
			offset += std::max(8, (mipWidth / 4) * (mipHeight / 4) * 8);
		} else if ( header->highResImageFormat == impl::IMAGE_FORMAT_DXT5 ) {
			offset += std::max(16, (mipWidth / 4) * (mipHeight / 4) * 16);
		} else if ( header->highResImageFormat == impl::IMAGE_FORMAT_BGRA8888 ) {
			offset += mipWidth * mipHeight * 4;
		}
	}

	const uint8_t* data = buffer.data() + offset;
	image.size = { header->width, header->height };
	image.channels = 4;
	image.bpp = 8 * 4;
	image.pixels.resize( header->width * header->height * 4 );

	int dataSize = 0;
	if ( header->highResImageFormat == impl::IMAGE_FORMAT_DXT1 ) {
		int blockCountX = (header->width + 3) / 4;
		int blockCountY = (header->height + 3) / 4;
		for ( int by = 0; by < blockCountY; ++by ) {
			for ( int bx = 0; bx < blockCountX; ++bx ) {
				impl::decompressDXT1Block(data + (by * blockCountX + bx) * 8, image.pixels.data(), bx * 4, by * 4, header->width, header->height);
			}
		}
	} else if ( header->highResImageFormat == impl::IMAGE_FORMAT_DXT5 ) {
		int blockCountX = (header->width + 3) / 4;
		int blockCountY = (header->height + 3) / 4;
		for ( int by = 0; by < blockCountY; ++by) {
			for ( int bx = 0; bx < blockCountX; ++bx) {
				impl::decompressDXT5Block(data + (by * blockCountX + bx) * 16, image.pixels.data(), bx * 4, by * 4, header->width, header->height);
			}
		}
	} else if ( header->highResImageFormat == impl::IMAGE_FORMAT_BGRA8888 ) {
		for ( auto i = 0; i < header->width * header->height; ++i ) {
			image.pixels[i * 4 + 0] = data[i * 4 + 2];
			image.pixels[i * 4 + 1] = data[i * 4 + 1];
			image.pixels[i * 4 + 2] = data[i * 4 + 0];
			image.pixels[i * 4 + 3] = data[i * 4 + 3];
		}
	} else {
		UF_MSG_WARNING("Unsupported VTF format: {}", header->highResImageFormat);
	}

	return true;
}