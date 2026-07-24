#if UF_USE_LGS
#include <uf/ext/lgs/common.h>
#include <uf/ext/lgs/pcx.h>

bool ext::lgs::loadPalette( const uf::stl::string& family, uf::stl::vector<uint8_t>& palette ) {
	if ( family.empty() ) return false;
	if ( !palette.empty() ) return true;

	uf::stl::vector<uint8_t> buffer;
	if ( uf::io::readAsBuffer(buffer, "FAM://" + family + "/full.pcx")) {
		if ( buffer.size() >= 769 && buffer[buffer.size() - 769] == 0x0C ) {
			palette.assign(buffer.end() - 768, buffer.end());
			return true;
		}
		buffer.clear();
	}

	if ( !uf::io::readAsBuffer(buffer, "FAM://" + family + "/full.gif") )
		return false;
	if ( buffer.size() < 13 || buffer[0] != 'G' || buffer[1] != 'I' || buffer[2] != 'F')
		return false;
	
	uint8_t packed = buffer[10];
	if ( !(packed & 0x80) ) return false;

	int gctSize = 2 << (packed & 0x07);
	int gctBytes = gctSize * 3;

	if ( buffer.size() < 13 + gctBytes) return false;

	palette.assign(buffer.begin() + 13, buffer.begin() + 13 + gctBytes);
	while ( palette.size() < 768 ) palette.emplace_back(0);
	return true;
}

bool ext::lgs::loadPcx( pod::Image& image, const uf::stl::vector<uint8_t>& buffer, const uint8_t* paletteData ) {
	if ( buffer.size() < 128 ) return false;
	if ( buffer[0] != 10 || buffer[2] != 1 ) return false;

	uint16_t xmin = buffer[4] | (buffer[5] << 8);
	uint16_t ymin = buffer[6] | (buffer[7] << 8);
	uint16_t xmax = buffer[8] | (buffer[9] << 8);
	uint16_t ymax = buffer[10] | (buffer[11] << 8);

	int width = xmax - xmin + 1;
	int height = ymax - ymin + 1;
	uint16_t bytesPerLine = buffer[66] | (buffer[67] << 8);

	uf::stl::vector<uint8_t> indices(bytesPerLine * height);
	size_t offset = 128;
	size_t dest = 0;

	while ( dest < indices.size() && offset < buffer.size() ) {
		uint8_t data = buffer[offset++];
		if ( (data & 0xC0) == 0xC0 ) {
			uint8_t runLength = data & 0x3F;
			uint8_t colorIndex = buffer[offset++];
			for ( int i = 0; i < runLength && dest < indices.size(); ++i ) {
				indices[dest++] = colorIndex;
			}
		} else {
			indices[dest++] = data;
		}
	}

	if ( buffer.size() >= offset + 769 && buffer[buffer.size() - 769] == 0x0C ) {
		paletteData = &buffer[buffer.size() - 768];
	}
	if ( !paletteData ) {
		return false;
	}

	image.pixels.resize(width * height * 4);
	for ( int y = 0; y < height; ++y ) {
		for ( int x = 0; x < width; ++x ) {
			uint8_t index = indices[y * bytesPerLine + x];
			int outIdx = (y * width + x) * 4;

			if ( index == 0 ) {
				image.pixels[outIdx + 0] = 0;
				image.pixels[outIdx + 1] = 0;
				image.pixels[outIdx + 2] = 0;
				image.pixels[outIdx + 3] = 0;
			} else {
				image.pixels[outIdx + 0] = paletteData[index * 3 + 0];
				image.pixels[outIdx + 1] = paletteData[index * 3 + 1];
				image.pixels[outIdx + 2] = paletteData[index * 3 + 2];
				image.pixels[outIdx + 3] = 255;
			}
		}
	}

	image.size = { width, height };
	image.channels = 4;
	image.bpp = 32;
	return true;
}
#endif