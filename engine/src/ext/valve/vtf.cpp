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

	constexpr uint32_t TEXTUREFLAGS_ENVMAP = 0x00002000;
	constexpr uint32_t TEXTUREFLAGS_NORMAL = 0x00000080;

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

	bool isCubemap = (header->flags & impl::TEXTUREFLAGS_ENVMAP) != 0;
	int numFrames = std::max<int>(1, header->frames);

	size_t singleFaceSize = 0;
	for ( int mip = header->mipmapCount - 1; mip >= 0; --mip ) {
		int mipWidth = std::max(1, header->width >> mip);
		int mipHeight = std::max(1, header->height >> mip);
		int blocksX = (mipWidth + 3) / 4;
		int blocksY = (mipHeight + 3) / 4;

		if ( header->highResImageFormat == impl::IMAGE_FORMAT_DXT1 ) singleFaceSize += blocksX * blocksY * 8;
		else if ( header->highResImageFormat == impl::IMAGE_FORMAT_DXT5 ) singleFaceSize += blocksX * blocksY * 16;
		else if ( header->highResImageFormat == impl::IMAGE_FORMAT_BGRA8888 ) singleFaceSize += mipWidth * mipHeight * 4;
	}

	size_t offset = header->headerSize;
	if ( header->lowResImageFormat != 0xFFFFFFFF ) {
		offset += std::max<size_t>(1, (header->lowResImageWidth * header->lowResImageHeight) / 2);
	}

	size_t remainingBytes = buffer.size() - offset;
	size_t bytesPerFace = singleFaceSize * numFrames;
	int actualFaces = bytesPerFace > 0 ? (remainingBytes / bytesPerFace) : 1;

	if ( actualFaces == 6 || actualFaces == 7 ) {
		isCubemap = true;
	}

	int numFaces = 1;
	if ( isCubemap ) {
		if ( actualFaces >= 6 ) {
			numFaces = actualFaces;
		} else {
			isCubemap = false;
			numFaces = 1;
		}
	}

	for ( int mip = header->mipmapCount - 1; mip > 0; --mip ) {
		int mipWidth = std::max(1, header->width >> mip);
		int mipHeight = std::max(1, header->height >> mip);
		int blocksX = (mipWidth + 3) / 4;
		int blocksY = (mipHeight + 3) / 4;

		size_t mipSize = 0;
		if ( header->highResImageFormat == impl::IMAGE_FORMAT_DXT1 ) mipSize = blocksX * blocksY * 8;
		else if ( header->highResImageFormat == impl::IMAGE_FORMAT_DXT5 ) mipSize = blocksX * blocksY * 16;
		else if ( header->highResImageFormat == impl::IMAGE_FORMAT_BGRA8888 ) mipSize = mipWidth * mipHeight * 4;

		offset += mipSize * numFrames * numFaces;
	}

	int outputFaces = isCubemap ? 6 : 1;

	image.size = { header->width, header->height * outputFaces };
	image.channels = 4;
	image.bpp = 8 * 4;
	image.pixels.resize( header->width * header->height * 4 * outputFaces );

	int faceSizePixels = header->width * header->height;
	int blocksX = (header->width + 3) / 4;
	int blocksY = (header->height + 3) / 4;

	const int faceMap[6] = { 4, 5, 0, 1, 2, 3 };
	const int faceModes[6] = { 2, 0, 6, 1, 1, 3  };
	for ( int face = 0; face < outputFaces; ++face ) {
		const uint8_t* data = buffer.data() + offset;
		int mappedFace = (isCubemap && face < 6) ? faceMap[face] : face;
		uint8_t* outPixels = image.pixels.data() + (mappedFace * faceSizePixels * 4);

		if ( header->highResImageFormat == impl::IMAGE_FORMAT_DXT1 ) {
			for ( int by = 0; by < blocksY; ++by ) {
				for ( int bx = 0; bx < blocksX; ++bx ) {
					impl::decompressDXT1Block(data + (by * blocksX + bx) * 8, outPixels, bx * 4, by * 4, header->width, header->height);
				}
			}
			offset += blocksX * blocksY * 8;
		} else if ( header->highResImageFormat == impl::IMAGE_FORMAT_DXT5 ) {
			for ( int by = 0; by < blocksY; ++by) {
				for ( int bx = 0; bx < blocksX; ++bx) {
					impl::decompressDXT5Block(data + (by * blocksX + bx) * 16, outPixels, bx * 4, by * 4, header->width, header->height);
				}
			}
			offset += blocksX * blocksY * 16;
		} else if ( header->highResImageFormat == impl::IMAGE_FORMAT_BGRA8888 ) {
			for ( auto i = 0; i < faceSizePixels; ++i ) {
				outPixels[i * 4 + 0] = data[i * 4 + 2];
				outPixels[i * 4 + 1] = data[i * 4 + 1];
				outPixels[i * 4 + 2] = data[i * 4 + 0];
				outPixels[i * 4 + 3] = data[i * 4 + 3];
			}
			offset += faceSizePixels * 4;
		}

		if ( isCubemap ) {
			int mode = faceModes[mappedFace];
			if ( mode == 0 ) continue;
			uf::stl::vector<uint8_t> temp(faceSizePixels * 4);
			memcpy(temp.data(), outPixels, faceSizePixels * 4);

			int size = header->width;
			int max = size - 1;

			for ( int y = 0; y < size; ++y ) {
				for ( int x = 0; x < size; ++x ) {
					int srcX = x;
					int srcY = y;

					switch ( mode ) {
						case 1: srcX = y; srcY = max - x; break; // 90 CW
						case 2: srcX = max - x; srcY = max - y; break; // 180
						case 3: srcX = max - y; srcY = x; break; // 90 CCW
						case 4: srcX = max - x; srcY = y; break; // Flip Horizontal
						case 5: srcX = x; srcY = max - y; break; // Flip Vertical
						case 6: srcX = y; srcY = x; break; // Transpose (Diagonal Flip)
						case 7: srcX = max - y; srcY = max - x; break; // Anti-Transpose
					}

					int srcIdx = (srcY * size + srcX) * 4;
					int dstIdx = (y * size + x) * 4;

					outPixels[dstIdx + 0] = temp[srcIdx + 0];
					outPixels[dstIdx + 1] = temp[srcIdx + 1];
					outPixels[dstIdx + 2] = temp[srcIdx + 2];
					outPixels[dstIdx + 3] = temp[srcIdx + 3];
				}
			}
		}
	}

	if ( (header->flags & impl::TEXTUREFLAGS_NORMAL) != 0 && header->highResImageFormat == impl::IMAGE_FORMAT_DXT5 ) {
		size_t pixelCount = image.pixels.size() / 4;
		for ( size_t i = 0; i < pixelCount; ++i ) {
			uint8_t& r = image.pixels[i * 4 + 0];
			uint8_t& g = image.pixels[i * 4 + 1];
			uint8_t& b = image.pixels[i * 4 + 2];
			uint8_t& a = image.pixels[i * 4 + 3];

			float x = (a / 255.0f) * 2.0f - 1.0f;
			float y = (g / 255.0f) * 2.0f - 1.0f;

			y = -y;

			float z = std::sqrt(std::max(1.0f - (x * x + y * y), 0.0f));

			r = (uint8_t)((x * 0.5f + 0.5f) * 255.0f);
			g = (uint8_t)((y * 0.5f + 0.5f) * 255.0f);
			b = (uint8_t)((z * 0.5f + 0.5f) * 255.0f);

			a = 255;
		}
	}

	return true;
}