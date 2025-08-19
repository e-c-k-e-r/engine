#pragma once

#include <texconv/common.h>

class Image {
public:
	Image();
	Image(int width, int height);

	bool loadFromFile(const uf::stl::string& path);
	bool saveToFile(const uf::stl::string& path) const;

	int width() const;
	int height() const;

	RGBA pixel(int x,int y) const;
	void setPixel(int x,int y, RGBA pixel);

	Image scaled(int newW,int newH,bool nearest) const;

	void allocateIndexed(int colors);
	void setIndexedPixel(int x,int y, uint8_t index);
	uint8_t indexedPixelAt(int x,int y) const;

	bool isIndexed() const { return indexedMode; }

private:
	int w,h;
	bool indexedMode;
	uf::stl::vector<RGBA> pixels;
	uf::stl::vector<uint8_t> indexed;
};