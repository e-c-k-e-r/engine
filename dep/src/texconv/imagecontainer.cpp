#include <uf/config.h>
#if UF_USE_DC_TEXCONV

#include <iostream>
#include <algorithm>

#include <texconv/imagecontainer.h>
#include <texconv/common.h>

bool ImageContainer::load(const uf::stl::vector<Image>& _images, int textureType, int mipmapFilter) {
	bool mipmapped = (textureType & FLAG_MIPMAPPED);

	if ((_images.size() > 1) && !mipmapped) {
		UF_MSG_ERROR("Only one input file may be specified if no mipmap flag is set.");
		return false;
	}

	
	for (const auto& img : _images) {
		if (!isValidSize(img.width(), img.height(), textureType)) {
			UF_MSG_ERROR("Image has invalid texture size {}x{}", img.width(), img.height());
			return false;
		}

		if (mipmapped && img.width() != img.height()) {
			UF_MSG_ERROR("Image is not square. Mipmapped textures require square images.");
			return false;
		}

		textureWidth  = std::max(textureWidth, img.width());
		textureHeight = std::max(textureHeight, img.height());

		images[img.width()] = img;  
		//UF_MSG_DEBUG("[INFO] Loaded image");
	}

	if (mipmapped) {
		if (mipmapFilter == 0) { 
			//UF_MSG_DEBUG("[INFO] Using nearest-neighbor filtering for mipmaps");
		} else {
			//UF_MSG_DEBUG("[INFO] Using bilinear filtering for mipmaps");
		}

		
		for (int size = TEXTURE_SIZE_MAX/2; size >= 1; size /= 2) {
			if (images.count(size*2) && !images.count(size)) {
				Image mipmap = images[size*2].scaled(size, size,
														 mipmapFilter == 0); 
				images[size] = mipmap;
				//UF_MSG_DEBUG("[INFO] Generated {}x{} mipmap", size, size);
			}
		}
	}

	if (textureWidth < TEXTURE_SIZE_MIN || textureHeight < TEXTURE_SIZE_MIN) {
		UF_MSG_ERROR("At least one input image must be 8x8 or larger.");
		return false;
	}

	
	keys.clear();
	for (auto& kv : images) keys.push_back(kv.first);
	std::sort(keys.begin(), keys.end());

	return true;
}
bool ImageContainer::load(const uf::stl::vector<uf::stl::string>& filenames, int textureType, int mipmapFilter) {
	uf::stl::vector<Image> images;

	for ( const auto& filename : filenames ) {
		Image& img = images.emplace_back();
		if (!img.loadFromFile(filename)) { 
			UF_MSG_ERROR("Failed to load image: {}", filename);
			return false;
		}
	}

	return load( images, textureType, mipmapFilter );
}

void ImageContainer::unloadAll() {
	textureWidth = 0;
	textureHeight = 0;
	images.clear();
	keys.clear();
}

const Image& ImageContainer::getByIndex(int index, bool ascending) const {
	if (index >= (int)keys.size()) {
		static Image dummy; 
		return dummy;
	} else {
		int realIdx = ascending ? index : ((int)keys.size() - index - 1);
		int size = keys[realIdx];
		return images.at(size);
	}
}

#endif