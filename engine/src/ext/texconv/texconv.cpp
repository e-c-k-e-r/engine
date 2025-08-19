// Modified from https://github.com/tvspelsfreak/texconv

#include <uf/config.h>
#if UF_USE_DC_TEXCONV

#include <iostream>
#include <fstream>
#include <algorithm>

#include <texconv/common.h>
#include <texconv/imagecontainer.h>

#include <uf/ext/texconv/texconv.h>

bool ext::texconv::convertTexture(  const ext::texconv::TextureOptions& opts  ) {
	uf::stl::unordered_map<uf::stl::string,int> formats = {
		{"ARGB1555",PIXELFORMAT_ARGB1555},
		{"RGB565",  PIXELFORMAT_RGB565},
		{"ARGB4444",PIXELFORMAT_ARGB4444},
		{"YUV422",  PIXELFORMAT_YUV422},
		{"BUMPMAP", PIXELFORMAT_BUMPMAP},
		{"PAL4BPP", PIXELFORMAT_PAL4BPP},
		{"PAL8BPP", PIXELFORMAT_PAL8BPP}
	};

	if ( opts.inputs.empty() ) {
		UF_MSG_ERROR( "No input file(s) specified" );
		return false;
	}
	if ( opts.output.empty() ) {
		UF_MSG_ERROR( "No output file specified" );
		return false;
	}

	int pixFmt = -1;
	auto it = formats.find( opts.format );
	if ( it != formats.end() ) pixFmt = it->second;
	if ( pixFmt == -1 ){
		UF_MSG_ERROR( "Unsupported format: {}", opts.format );
		return false;
	}

	uf::stl::string palFile = opts.output + ".pal";

	int textureType = ( pixFmt<<PIXELFORMAT_SHIFT );
	if ( opts.mipmap ) textureType |= FLAG_MIPMAPPED;
	if ( opts.compress ) textureType |= FLAG_COMPRESSED;
	if ( opts.stride ) textureType |= ( FLAG_STRIDED|FLAG_NONTWIDDLED );

	int filter = ( isPaletted( textureType ) || opts.nearest ) ? 0 : 1;
	if ( opts.bilinear ) filter = 1;

	ImageContainer images;
	if ( !images.load( opts.inputs, textureType, filter ) ) return false;

	if ( textureType&FLAG_STRIDED ){
		int stride = images.width()/32;
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
	if (  !opts.previewFile.empty() || ( !opts.codeUsageFile.empty() && ( textureType&FLAG_COMPRESSED ) )  ){
		if (  generatePreview( opts.output,palFile,opts.previewFile,opts.codeUsageFile )  ){
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