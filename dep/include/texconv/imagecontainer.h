#pragma once

#include <texconv/image.h>

class ImageContainer {
public:
	
	bool load( const uf::stl::vector<Image>& images, int textureType, int mipmapFilter );
	bool load( const uf::stl::vector<uf::stl::string>& filenames, int textureType, int mipmapFilter );

	void unloadAll();

	bool hasMipmaps() const { return images.size() > 1; }
	bool hasSize(int size) const { return images.find(size) != images.end(); }

	const Image& getByIndex(int index, bool ascending=true) const;
	const Image& getBySize(int size) const { return images.at(size); }

	int imageCount() const { return (int)images.size(); }
	int width() const { return textureWidth; }
	int height() const { return textureHeight; }

private:
	int textureWidth = 0;
	int textureHeight = 0;
	uf::stl::unordered_map<int, Image> images;   
	uf::stl::vector<int> keys;			 
};