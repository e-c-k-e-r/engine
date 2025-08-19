#pragma once

#include <texconv/common.h>

class ImageContainer;

class Palette {
public:
	Palette() = default;
	Palette(const ImageContainer& images);

	int colorCount() const { return (int)colorsVec.size(); }
	void clear() { colorsMap.clear(); colorsVec.clear(); }

	void insert(uint32_t argb);

	int indexOf(uint32_t argb) const;
	uint32_t colorAt(int index) const;

	bool load(const uf::stl::string& filename);
	bool save(const uf::stl::string& filename) const;

private:
	uf::stl::unordered_map<uint32_t,int> colorsMap;
	uf::stl::vector<uint32_t> colorsVec;
};