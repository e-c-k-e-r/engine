#include <uf/config.h>
#if UF_USE_DC_TEXCONV

#include <fstream>
#include <iostream>

#include <texconv/palette.h>
#include <texconv/imagecontainer.h>

#include <uf/utils/memory/vector_stream.h>

Palette::Palette(const ImageContainer& images) {
	for (int i=0;i<images.imageCount();i++) {
		const Image& img=images.getByIndex(i);
		for (int y=0;y<img.height();y++)
			for (int x=0;x<img.width();x++)
				insert(packColor(img.pixel(x,y)));
	}
}

void Palette::insert(uint32_t color) {
	if (colorsMap.find(color)==colorsMap.end()) {
		int idx=(int)colorsVec.size();
		colorsMap[color]=idx;
		colorsVec.emplace_back(color);
	}
}

int Palette::indexOf(uint32_t argb) const {
	auto it=colorsMap.find(argb);
	return (it!=colorsMap.end()) ? it->second : 0;
}

uint32_t Palette::colorAt(int index) const {
	if (index>=0 && index<(int)colorsVec.size())
		return colorsVec[index];
	return 0xFF000000; 
}

bool Palette::save(const uf::stl::string& filename) const {
	std::ofstream out(filename,std::ios::binary);
	if (!out.is_open()) {
		UF_MSG_ERROR("Failed to open {} for writing", filename);
		return false;
	}

	
	out.write(TEXTURE_MAGIC,4); 
	int32_t n=colorCount();
	out.write((char*)&n,sizeof(int32_t));

	
	for (uint32_t c:colorsVec)
		out.write((char*)&c,sizeof(uint32_t));

	out.close();
	return true;
}

bool Palette::load(const uf::stl::string& filename) {
	std::ifstream in(filename,std::ios::binary);
	if (!in.is_open()) {
		UF_MSG_ERROR("Failed to open {} for reading", filename);
		return false;
	}
	char magic[4];
	in.read(magic,4);
	if (memcmp(magic,PALETTE_MAGIC,4)!=0) {
		UF_MSG_ERROR("{} is not a valid palette file", filename);
		return false;
	}
	int32_t numColors=0;
	in.read((char*)&numColors,sizeof(int32_t));

	clear();
	for (int i=0;i<numColors;i++) {
		uint32_t c;
		in.read((char*)&c,sizeof(uint32_t));
		insert(c);
	}
	return true;
}

uf::stl::vector<uint8_t> Palette::encode() const {
	uf::stl::vector<uint8_t> data;
	uf::stl::vector_stream out(data);

	
	out.write(TEXTURE_MAGIC,4); 
	int32_t n = colorCount();
	out.write((char*)&n,sizeof(int32_t));

	for (uint32_t c:colorsVec)
		out.write((char*)&c,sizeof(uint32_t));

	return data;
}
#endif