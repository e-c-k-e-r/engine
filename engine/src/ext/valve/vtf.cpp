#if UF_USE_VALVE
#include <uf/ext/valve/bsp.h>
#include <uf/ext/valve/mdl.h>
#include <uf/ext/valve/vtf.h>
#include <uf/ext/valve/vpk.h>
#include <uf/ext/valve/common.h>
#include <uf/utils/memory/memcpy.h>

namespace impl {
	constexpr uint32_t IMAGE_FORMAT_RGBA8888 = 0;
	constexpr uint32_t IMAGE_FORMAT_RGB888 = 2;
	constexpr uint32_t IMAGE_FORMAT_BGR888 = 3;
	constexpr uint32_t IMAGE_FORMAT_BGR565 = 4;
	constexpr uint32_t IMAGE_FORMAT_BGRX8888 = 11;
	constexpr uint32_t IMAGE_FORMAT_BGRA8888 = 12;
	constexpr uint32_t IMAGE_FORMAT_DXT1 = 13;
	constexpr uint32_t IMAGE_FORMAT_DXT3 = 14;
	constexpr uint32_t IMAGE_FORMAT_DXT5 = 15;
	constexpr uint32_t IMAGE_FORMAT_RGBA16161616F = 24;

	constexpr uint32_t MATERIAL_VAR_DEBUG = 0x0001; //	$debug
	constexpr uint32_t MATERIAL_VAR_NO_DEBUG_OVERRIDE = 0x0002; //	$no_fullbright
	constexpr uint32_t MATERIAL_VAR_NO_DRAW = 0x0004; //	$no_draw
	constexpr uint32_t MATERIAL_VAR_USE_IN_FILLRATE_MODE = 0x0008; //	$use_in_fillrate_mode
	constexpr uint32_t MATERIAL_VAR_VERTEXCOLOR = 0x0010; //	$vertexcolor
	constexpr uint32_t MATERIAL_VAR_VERTEXALPHA = 0x0020; //	$vertexalpha
	constexpr uint32_t MATERIAL_VAR_SELFILLUM = 0x0040; //	$selfillum
	constexpr uint32_t MATERIAL_VAR_ADDITIVE = 0x0080; //	$additive
	constexpr uint32_t MATERIAL_VAR_ALPHATEST = 0x0100; //	$alphatest
	constexpr uint32_t MATERIAL_VAR_MULTIPASS = 0x0200; //	$multipass
	constexpr uint32_t MATERIAL_VAR_ZNEARER = 0x0400; //	$znearer
	constexpr uint32_t MATERIAL_VAR_MODEL = 0x0800; //	$model
	constexpr uint32_t MATERIAL_VAR_FLAT = 0x1000; //	$flat
	constexpr uint32_t MATERIAL_VAR_NOCULL = 0x2000; //	$nocull
	constexpr uint32_t MATERIAL_VAR_NOFOG = 0x4000; //	$nofog
	constexpr uint32_t MATERIAL_VAR_IGNOREZ = 0x8000; //	$ignorez
	constexpr uint32_t MATERIAL_VAR_DECAL = 0x10000; //	$decal
	constexpr uint32_t MATERIAL_VAR_ENVMAPSPHERE = 0x20000; //	$envmapsphere
	constexpr uint32_t MATERIAL_VAR_NOALPHAMOD = 0x40000; //	$noalphamod
	constexpr uint32_t MATERIAL_VAR_ENVMAPCAMERASPACE = 0x80000; //	$envmapcameraspace
	constexpr uint32_t MATERIAL_VAR_BASEALPHAENVMAPMASK = 0x100000; //	$basealphaenvmapmask
	constexpr uint32_t MATERIAL_VAR_TRANSLUCENT = 0x200000; //	$translucent
	constexpr uint32_t MATERIAL_VAR_NORMALMAPALPHAENVMAPMASK = 0x400000; //	$normalmapalphaenvmapmask
	constexpr uint32_t MATERIAL_VAR_NEEDS_SOFTWARE_SKINNING = 0x800000; //	$softwareskin
	constexpr uint32_t MATERIAL_VAR_OPAQUETEXTURE = 0x1000000; //	$opaquetexture
	constexpr uint32_t MATERIAL_VAR_ENVMAPMODE = 0x2000000; //	$envmapmode
	constexpr uint32_t MATERIAL_VAR_SUPPRESS_DECALS = 0x4000000; //	$nodecal
	constexpr uint32_t MATERIAL_VAR_HALFLAMBERT = 0x8000000; //	$halflambert
	constexpr uint32_t MATERIAL_VAR_WIREFRAME = 0x10000000; //	$wireframe
	constexpr uint32_t MATERIAL_VAR_ALLOWALPHATOCOVERAGE = 0x20000000; //	$allowalphatocoverage
	constexpr uint32_t MATERIAL_VAR_IGNORE_ALPHA_MODULATION = 0x40000000; //	

	constexpr uint32_t TEXTUREFLAGS_POINTSAMPLE = 1;
	constexpr uint32_t TEXTUREFLAGS_TRILINEAR = 2;
	constexpr uint32_t TEXTUREFLAGS_CLAMPS = 4;
	constexpr uint32_t TEXTUREFLAGS_CLAMPT = 8;
	constexpr uint32_t TEXTUREFLAGS_ANISOTROPIC = 16;
	constexpr uint32_t TEXTUREFLAGS_HINT_DXT5 = 32;
	constexpr uint32_t TEXTUREFLAGS_PWL_CORRECTED = 64;
	constexpr uint32_t TEXTUREFLAGS_NORMAL = 128;
	constexpr uint32_t TEXTUREFLAGS_NOMIP = 256;
	constexpr uint32_t TEXTUREFLAGS_NOLOD = 512;
	constexpr uint32_t TEXTUREFLAGS_ALL_MIPS = 1024;
	constexpr uint32_t TEXTUREFLAGS_PROCEDURAL = 2048;
	constexpr uint32_t TEXTUREFLAGS_ONEBITALPHA = 4096;
	constexpr uint32_t TEXTUREFLAGS_EIGHTBITALPHA = 8192;
	constexpr uint32_t TEXTUREFLAGS_ENVMAP = 16384;
	constexpr uint32_t TEXTUREFLAGS_RENDERTARGET = 32768;
	constexpr uint32_t TEXTUREFLAGS_DEPTHRENDERTARGET = 65536;
	constexpr uint32_t TEXTUREFLAGS_NODEBUGOVERRIDE = 131072;
	constexpr uint32_t TEXTUREFLAGS_SINGLECOPY = 262144;
	constexpr uint32_t TEXTUREFLAGS_STAGING_MEMORY = 524288;
	constexpr uint32_t TEXTUREFLAGS_IMMEDIATE_CLEANUP = 1048576;
	constexpr uint32_t TEXTUREFLAGS_IGNORE_PICMIP = 2097152;
	constexpr uint32_t TEXTUREFLAGS_UNUSED_00400000 = 4194304;
	constexpr uint32_t TEXTUREFLAGS_NODEPTHBUFFER = 8388608;
	constexpr uint32_t TEXTUREFLAGS_UNUSED_01000000 = 16777216;
	constexpr uint32_t TEXTUREFLAGS_CLAMPU = 33554432;
	constexpr uint32_t TEXTUREFLAGS_VERTEXTEXTURE = 67108864;
	constexpr uint32_t TEXTUREFLAGS_SSBUMP = 134217728;
	constexpr uint32_t TEXTUREFLAGS_UNUSED_10000000 = 268435456;
	constexpr uint32_t TEXTUREFLAGS_BORDER = 536870912;
	constexpr uint32_t TEXTUREFLAGS_STREAMABLE_COARSE = 1073741824;
	constexpr uint32_t TEXTUREFLAGS_STREAMABLE_FINE = 2147483648;

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

	inline size_t getMipSize( uint32_t format, int width, int height ) {
		int blocksX = (width + 3) / 4;
		int blocksY = (height + 3) / 4;

		switch ( format ) {
			case impl::IMAGE_FORMAT_DXT1:
				return blocksX * blocksY * 8;
			case impl::IMAGE_FORMAT_DXT3:
			case impl::IMAGE_FORMAT_DXT5:
				return blocksX * blocksY * 16;
			case impl::IMAGE_FORMAT_BGR888:
			case impl::IMAGE_FORMAT_RGB888:
				return width * height * 3;
			case impl::IMAGE_FORMAT_BGRA8888:
			case impl::IMAGE_FORMAT_BGRX8888:
			case impl::IMAGE_FORMAT_RGBA8888:
				return width * height * 4;
			case impl::IMAGE_FORMAT_BGR565:
				return width * height * 2;
			case impl::IMAGE_FORMAT_RGBA16161616F:
				return width * height * 8;
			default:
				return 0;
		}
	};

	// to-do: cram this inside the image functions
	inline void decodeRGB565( uint16_t color, uint8_t& r, uint8_t& g, uint8_t& b ) {
		r = (uint8_t)(((color >> 11) & 0x1F) * 255 / 31);
		g = (uint8_t)(((color >> 5) & 0x3F) * 255 / 63);
		b = (uint8_t)((color & 0x1F) * 255 / 31);
	}

	inline uint8_t halfTo8Bit(uint16_t h) {
		uint32_t exp = (h >> 10) & 0x1F;
		uint32_t mant = h & 0x3FF;
		if (exp == 0) return 0;
		if (exp == 31) return 255;

		exp = exp + (127 - 15);
		uint32_t f = (exp << 23) | (mant << 13);
		float val;
		uf::stl::memcpy(&val, &f, sizeof(float));

		return (uint8_t)(std::min(std::max(val, 0.0f), 1.0f) * 255.0f);
	}

	inline float halfToFloat(uint16_t h) {
		uint32_t exp = (h >> 10) & 0x1F;
		uint32_t mant = h & 0x3FF;
		if (exp == 0 && mant == 0) return 0.0f;

		exp = exp + (127 - 15);
		uint32_t f = (exp << 23) | (mant << 13);
		float val;
		uf::stl::memcpy(&val, &f, sizeof(float));
		return val;
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

	void decompressDXT3Block(const uint8_t* block, uint8_t* out, int x, int y, int width, int height) {
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

				uint8_t colorIdx = (colorIndices >> ((py * 4 + px) * 2)) & 0x03;

				int alphaOffset = (py * 4 + px) / 2;
				int alphaShift = ((py * 4 + px) % 2) * 4;
				uint8_t alpha4 = (block[alphaOffset] >> alphaShift) & 0x0F;
				uint8_t alpha = (alpha4 << 4) | alpha4;

				int offset = ((y + py) * width + (x + px)) * 4;
				out[offset + 0] = r[colorIdx];
				out[offset + 1] = g[colorIdx];
				out[offset + 2] = b[colorIdx];
				out[offset + 3] = alpha;
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

	switch ( header->highResImageFormat ) {
		case impl::IMAGE_FORMAT_RGBA8888:
		case impl::IMAGE_FORMAT_RGB888:
		case impl::IMAGE_FORMAT_BGR888:
		case impl::IMAGE_FORMAT_BGR565:
		case impl::IMAGE_FORMAT_BGRX8888:
		case impl::IMAGE_FORMAT_BGRA8888:
		case impl::IMAGE_FORMAT_DXT1:
		case impl::IMAGE_FORMAT_DXT3:
		case impl::IMAGE_FORMAT_DXT5:
		case impl::IMAGE_FORMAT_RGBA16161616F: {
			break;
		}
		default: {
			UF_MSG_ERROR("VTF '{}' has unrecognized format: 0x{:x}", filename, header->highResImageFormat);
		} break;
	}

	size_t singleFaceSize = 0;
	for ( int mip = 0; mip < header->mipmapCount; ++mip ) {
		int mipWidth = std::max(1, header->width >> mip);
		int mipHeight = std::max(1, header->height >> mip);
		singleFaceSize += impl::getMipSize(header->highResImageFormat, mipWidth, mipHeight);
	}

	size_t offset = header->headerSize;
	if ( header->lowResImageFormat != 0xFFFFFFFF ) {
		offset += impl::getMipSize(header->lowResImageFormat, header->lowResImageWidth, header->lowResImageHeight);
	}
	int numFaces = 1;
	int numFrames = std::max<int>(1, header->frames);
	if ( singleFaceSize > 0 && numFrames > 0 ) {
		numFaces = (buffer.size() - offset) / (singleFaceSize * numFrames);
	}

	bool isHDR = (header->highResImageFormat == impl::IMAGE_FORMAT_RGBA16161616F);
	bool isCubemap = (header->flags & impl::TEXTUREFLAGS_ENVMAP) != 0 && numFaces >= 6;

	for ( int mip = header->mipmapCount - 1; mip > 0; --mip ) {
		int mipWidth = std::max(1, header->width >> mip);
		int mipHeight = std::max(1, header->height >> mip);
		offset += impl::getMipSize(header->highResImageFormat, mipWidth, mipHeight) * numFrames * numFaces;
	}

	int outputFaces = isCubemap ? 6 : 1;
	int bytesPerPixel = isHDR ? 8 : 4;
	image.size = { header->width, header->height * outputFaces };
	image.channels = 4;
	image.bpp = 8 * bytesPerPixel;
	image.layers = numFrames;
	image.format = isHDR ? uf::renderer::enums::Format::R16G16B16A16_SFLOAT : uf::renderer::enums::Format::R8G8B8A8_UNORM;
	image.pixels.resize( header->width * header->height * bytesPerPixel * outputFaces * numFrames );

	int faceSize = header->width * header->height;
	int blocksX = (header->width + 3) / 4;
	int blocksY = (header->height + 3) / 4;

	const int faceMap[6] = { 4, 5, 0, 1, 2, 3 };
	const int faceModes[6] = { 2, 0, 6, 1, 1, 3  };
	for ( int frame = 0; frame < numFrames; ++frame ) {
		for ( int face = 0; face < outputFaces; ++face ) {
			const uint8_t* data = buffer.data() + offset;
			int mappedFace = (isCubemap && face < 6) ? faceMap[face] : face;
			size_t outOffset = (frame * outputFaces + mappedFace) * faceSize * bytesPerPixel;
			uint8_t* outPixels = image.pixels.data() + outOffset;

			if ( header->highResImageFormat == impl::IMAGE_FORMAT_DXT1 ) {
				for ( int by = 0; by < blocksY; ++by ) {
					for ( int bx = 0; bx < blocksX; ++bx ) {
						impl::decompressDXT1Block(data + (by * blocksX + bx) * 8, outPixels, bx * 4, by * 4, header->width, header->height);
					}
				}
				offset += blocksX * blocksY * 8;
			} else if ( header->highResImageFormat == impl::IMAGE_FORMAT_DXT3 ) {
				for ( int by = 0; by < blocksY; ++by) {
					for ( int bx = 0; bx < blocksX; ++bx) {
						impl::decompressDXT3Block(data + (by * blocksX + bx) * 16, outPixels, bx * 4, by * 4, header->width, header->height);
					}
				}
				offset += blocksX * blocksY * 16;

			} else if ( header->highResImageFormat == impl::IMAGE_FORMAT_DXT5 ) {
				for ( int by = 0; by < blocksY; ++by) {
					for ( int bx = 0; bx < blocksX; ++bx) {
						impl::decompressDXT5Block(data + (by * blocksX + bx) * 16, outPixels, bx * 4, by * 4, header->width, header->height);
					}
				}
				offset += blocksX * blocksY * 16;
			} else if ( header->highResImageFormat == impl::IMAGE_FORMAT_BGRX8888 || header->highResImageFormat == impl::IMAGE_FORMAT_RGBA8888 ) {
				bool isRgba = (header->highResImageFormat == impl::IMAGE_FORMAT_RGBA8888);
				for ( auto i = 0; i < faceSize; ++i ) {
					outPixels[i * 4 + 0] = data[i * 4 + (isRgba ? 0 : 2)];
					outPixels[i * 4 + 1] = data[i * 4 + 1];
					outPixels[i * 4 + 2] = data[i * 4 + (isRgba ? 2 : 0)];
					outPixels[i * 4 + 3] = isRgba ? data[i * 4 + 3] : 255;
				}
				offset += faceSize * 4;
			} else if ( header->highResImageFormat == impl::IMAGE_FORMAT_BGR888 ) {
				for ( auto i = 0; i < faceSize; ++i ) {
					outPixels[i * 4 + 0] = data[i * 3 + 2];
					outPixels[i * 4 + 1] = data[i * 3 + 1];
					outPixels[i * 4 + 2] = data[i * 3 + 0];
					outPixels[i * 4 + 3] = 255;
				}
				offset += faceSize * 3;
			} else if ( header->highResImageFormat == impl::IMAGE_FORMAT_BGRA8888 ) {
				for ( auto i = 0; i < faceSize; ++i ) {
					outPixels[i * 4 + 0] = data[i * 4 + 2];
					outPixels[i * 4 + 1] = data[i * 4 + 1];
					outPixels[i * 4 + 2] = data[i * 4 + 0];
					outPixels[i * 4 + 3] = data[i * 4 + 3];
				}
				offset += faceSize * 4;
			} else if ( header->highResImageFormat == impl::IMAGE_FORMAT_RGB888 ) {
				for ( auto i = 0; i < faceSize; ++i ) {
					outPixels[i * 4 + 0] = data[i * 3 + 0];
					outPixels[i * 4 + 1] = data[i * 3 + 1];
					outPixels[i * 4 + 2] = data[i * 3 + 2];
					outPixels[i * 4 + 3] = 255;
				}
				offset += faceSize * 3;
			} else if ( header->highResImageFormat == impl::IMAGE_FORMAT_BGR565 ) {
				for ( auto i = 0; i < faceSize; ++i ) {
					uint16_t color = *(const uint16_t*)(data + i * 2);
					impl::decodeRGB565(color, outPixels[i * 4 + 0], outPixels[i * 4 + 1], outPixels[i * 4 + 2]);
					outPixels[i * 4 + 3] = 255;
				}
				offset += faceSize * 2;
			} else if ( header->highResImageFormat == impl::IMAGE_FORMAT_RGBA16161616F ) {
				uf::stl::memcpy(outPixels, data, faceSize * 8);
				offset += faceSize * 8;
			} else {
				UF_MSG_ERROR("VTF '{}' has unimplemented format: 0x{:x}", filename, header->highResImageFormat );
			}

			if ( isCubemap ) {
				int mode = faceModes[mappedFace];
				if ( mode == 0 ) continue;
				uf::stl::vector<uint8_t> temp(faceSize * bytesPerPixel);
				uf::stl::memcpy(temp.data(), outPixels, faceSize * bytesPerPixel);

				int size = header->width;
				int max = size - 1;

				for ( int y = 0; y < size; ++y ) {
					for ( int x = 0; x < size; ++x ) {
						int srcX = x, srcY = y;

						switch ( mode ) {
							case 1: srcX = y; srcY = max - x; break; // 90 CW
							case 2: srcX = max - x; srcY = max - y; break; // 180
							case 3: srcX = max - y; srcY = x; break; // 90 CCW
							case 4: srcX = max - x; srcY = y; break; // Flip Horizontal
							case 5: srcX = x; srcY = max - y; break; // Flip Vertical
							case 6: srcX = y; srcY = x; break; // Transpose
							case 7: srcX = max - y; srcY = max - x; break; // Anti-Transpose
						}

						int srcIdx = (srcY * size + srcX) * bytesPerPixel;
						int dstIdx = (y * size + x) * bytesPerPixel;

						uf::stl::memcpy(&outPixels[dstIdx], &temp[srcIdx], bytesPerPixel);
					}
				}
			}
		}
	}

	return true;
}
#endif