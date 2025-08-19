// Modified from https://github.com/tvspelsfreak/texconv

#include <uf/config.h>
#if UF_USE_DC_TEXCONV

#include <iostream>
#include <fstream>
#include <algorithm>

#include <texconv/common.h>
#include <texconv/palette.h>
#include <texconv/imagecontainer.h>

#include <uf/ext/texconv/texconv.h>

#include <uf/utils/memory/vector_stream.h>

namespace {
	uf::stl::unordered_map<uf::stl::string,int> formats = {
		{"ARGB1555",PIXELFORMAT_ARGB1555},
		{"RGB565",  PIXELFORMAT_RGB565},
		{"ARGB4444",PIXELFORMAT_ARGB4444},
		{"YUV422",  PIXELFORMAT_YUV422},
		{"BUMPMAP", PIXELFORMAT_BUMPMAP},
		{"PAL4BPP", PIXELFORMAT_PAL4BPP},
		{"PAL8BPP", PIXELFORMAT_PAL8BPP}
	};

	Image convert(const uf::Image& image) {
		auto size = image.getDimensions();
		auto& pixels = image.getPixels();

		uf::stl::vector<RGBA> buffer(size.x * size.y);

		for ( auto i = 0; i < buffer.size(); ++i ) {
			buffer[i] = RGBA{
				pixels[i * 4 + 0],
				pixels[i * 4 + 1],
				pixels[i * 4 + 2],
				pixels[i * 4 + 3]
			};
		}

		return Image{ size.x, size.y, std::move(buffer) };
	}

	uf::Image convert(const Image& image) {
		auto width = image.width();
		auto height = image.height();
		auto& pixels = image.pixels();

		uf::Image::container_t buffer;
		buffer.reserve(width * height * 4);

		for (auto& pixel : pixels) {
			buffer.emplace_back(pixel.r);
			buffer.emplace_back(pixel.g);
			buffer.emplace_back(pixel.b);
			buffer.emplace_back(pixel.a);
		}

		return uf::Image{ std::move(buffer), { width, height } };
	}
}

pod::Dtex ext::texconv::convert(  const uf::Image& _image, const uf::stl::string& _format ) {
	pod::TextureOptions opts; // temp cringe

	pod::Dtex dtex;
	uf::stl::vector_stream out(dtex.imageData);

	auto format = _format;

	// automatically deduce best format (for now just pick ARGB4444 if there's transparency)
	if ( format == "auto" ) {
		format = "RGB565";
		auto& pixels = _image.getPixels();
		for ( auto i = 0; i < pixels.size(); i += 4 ) {
			if ( pixels[i+3] == 0xFF ) continue;
			format = "ARGB4444";
			break;
		}

		opts.compress = true; // to mimic my script since flags for compression
	}

	int pixFmt = -1;
	auto it = ::formats.find( format );
	if ( it != ::formats.end() ) pixFmt = it->second;
	if ( pixFmt == -1 ){
		UF_MSG_ERROR( "Unsupported format: {}", format );
		return dtex;
	}

	int textureType = ( pixFmt << PIXELFORMAT_SHIFT );
	if ( opts.mipmap ) textureType |= FLAG_MIPMAPPED;
	if ( opts.compress ) textureType |= FLAG_COMPRESSED;
	if ( opts.stride ) textureType |= ( FLAG_STRIDED | FLAG_NONTWIDDLED );

	int filter = ( isPaletted( textureType ) || opts.nearest ) ? 0 : 1;
	if ( opts.bilinear ) filter = 1;

	Image image = ::convert( _image );
	ImageContainer images;
	if ( !images.load( {image}, textureType, filter ) ) return dtex;

	if ( textureType & FLAG_STRIDED ){
		int stride = images.width() / 32;
		textureType |= stride;
	}

	int expectedSize = writeTextureHeader( out, images.width(), images.height(), textureType );
	auto before = out.tellp();

	if ( isPaletted( textureType ) ){
		auto palette = convertPaletted( out, images, textureType );
		dtex.paletteData = palette.encode();
	} else {
		convert16BPP( out, images, textureType );
	}

	auto after = out.tellp();
	int padding = expectedSize - ( (int) after - (int) before );
	if ( padding > 0 ){
		writeZeroes( out, padding );
	}

	return dtex;
}

bool UF_API ext::texconv::save( const pod::Dtex& dtex, const uf::stl::string& filename, bool preview ) {
	uf::stl::string outputFilename = filename + ".dtex";
	uf::stl::string paletteFilename = filename + ".pal";
	uf::stl::string previewFilename = filename + ".preview.png";

	if ( !dtex.imageData.empty() ) {
		uf::io::write( outputFilename, dtex.imageData );
	}
	if ( !dtex.paletteData.empty() ) {
		uf::io::write( outputFilename, dtex.paletteData );
	}
	if ( preview ) {
		generatePreview( outputFilename, paletteFilename, previewFilename, "" );
	}

	return true;
}

// maintains original main()
bool ext::texconv::convert(  const pod::TextureOptions& opts  ) {
	if ( opts.input.empty() ) {
		UF_MSG_ERROR( "No input file(s) specified" );
		return false;
	}
	if ( opts.output.empty() ) {
		UF_MSG_ERROR( "No output file specified" );
		return false;
	}

	int pixFmt = -1;
	auto it = ::formats.find( opts.format );
	if ( it != ::formats.end() ) pixFmt = it->second;
	if ( pixFmt == -1 ){
		UF_MSG_ERROR( "Unsupported format: {}", opts.format );
		return false;
	}

	uf::stl::string palFile = opts.output + ".pal";

	int textureType = ( pixFmt << PIXELFORMAT_SHIFT );
	if ( opts.mipmap ) textureType |= FLAG_MIPMAPPED;
	if ( opts.compress ) textureType |= FLAG_COMPRESSED;
	if ( opts.stride ) textureType |= ( FLAG_STRIDED | FLAG_NONTWIDDLED );

	int filter = ( isPaletted( textureType ) || opts.nearest ) ? 0 : 1;
	if ( opts.bilinear ) filter = 1;

	ImageContainer images;
	if ( !images.load( {opts.input}, textureType, filter ) ) return false;

	if ( textureType & FLAG_STRIDED ){
		int stride = images.width() / 32;
		textureType |= stride;
	}

	std::ofstream out( opts.output, std::ios::binary );
	if ( !out.is_open() ){
		UF_MSG_ERROR( "Failed to open output file {}", opts.output );
		return false;
	}

	int expectedSize = writeTextureHeader( out, images.width(), images.height(), textureType );
	auto before = out.tellp();

	if ( isPaletted( textureType ) ){
		convertPaletted( out, images, textureType, palFile );
	} else {
		convert16BPP( out, images, textureType );
	}

	auto after = out.tellp();
	int padding = expectedSize - ( (int) after - (int) before );
	if ( padding > 0 ){
		writeZeroes( out, padding );
		UF_MSG_INFO( "Added {} padding bytes", std::to_string( padding ) );
	}
	out.close();
	UF_MSG_INFO( "Wrote texture {}", opts.output );

	// Preview & Code usage
	if ( !opts.previewFile.empty() || ( !opts.codeUsageFile.empty() && ( textureType & FLAG_COMPRESSED ) )  ){
		if (  generatePreview( opts.output, palFile, opts.previewFile, opts.codeUsageFile )  ){
			if ( !opts.previewFile.empty() ) UF_MSG_INFO( "Saved preview {}", opts.previewFile );
			if ( !opts.codeUsageFile.empty() ) UF_MSG_INFO( "Saved code usage {}", opts.codeUsageFile );
		} else {
			if ( !opts.previewFile.empty() ) UF_MSG_ERROR( "Failed to save preview {}", opts.previewFile );
			if ( !opts.codeUsageFile.empty() ) UF_MSG_ERROR( "Failed to save code usage {}", opts.codeUsageFile );
			return false;
		}
	}

	return true;
}
#endif