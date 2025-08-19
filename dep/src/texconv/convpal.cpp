#include <uf/config.h>
#if UF_USE_DC_TEXCONV

#include <iostream>

#include <texconv/imagecontainer.h>
#include <texconv/twiddler.h>
#include <texconv/palette.h>
#include <texconv/vqtools.h>
#include <texconv/common.h>

static void vectorizeARGB(const ImageContainer& images, uf::stl::vector<Vec<4>>& vectors) {
	for (int i=0;i<images.imageCount();i++) {
		const Image& img=images.getByIndex(i);
		for (int y=0;y<img.height();y++) {
			for (int x=0;x<img.width();x++) {
				RGBA px=img.pixel(x,y);
				Vec<4> vec;
				vec[0]=px.a/255.f;
				vec[1]=px.r/255.f;
				vec[2]=px.g/255.f;
				vec[3]=px.b/255.f;
				vectors.push_back(vec);
			}
		}
	}
}


static void devectorizeARGB(const ImageContainer& srcImages, const uf::stl::vector<Vec<4>>& vectors,
							const VectorQuantizer<4>& vq, uf::stl::vector<Image>& indexedImages, Palette& palette) {
	int vindex=0;
	for (int i=0;i<srcImages.imageCount();i++) {
		const Image& src=srcImages.getByIndex(i);
		Image dst(src.width(),src.height());
		dst.allocateIndexed(256);

		for (int y=0;y<src.height();y++) {
			for (int x=0;x<src.width();x++) {
				const Vec<4>& vec=vectors[vindex++];
				int codeIndex=vq.findClosest(vec);
				dst.setIndexedPixel(x,y,(uint8_t)codeIndex);
			}
		}
		indexedImages.push_back(dst);
	}
	
	for (int i=0;i<vq.codeCount();i++) {
		const Vec<4>& v=vq.codeVector(i);
		uint32_t color=(uint8_t)(v[0]*255)<<24 | (uint8_t)(v[1]*255)<<16 |
					   (uint8_t)(v[2]*255)<<8 | (uint8_t)(v[3]*255);
		palette.insert(color);
	}
}

void convertToIndexedImages(const ImageContainer& src, const Palette& pal, uf::stl::vector<Image>& dst);
void writeUncompressed4BPPData(std::ostream& stream, const uf::stl::vector<Image>& indexedImages);
void writeUncompressed8BPPData(std::ostream& stream, const uf::stl::vector<Image>& indexedImages);
void writeUncompressedPreview(const uf::stl::string& filename, const uf::stl::vector<Image>& indexedImages, const Palette& palette);
void writeCompressed4BPPData(std::ostream& stream, const uf::stl::vector<Image>& indexedImages, const Palette& palette);
void writeCompressed8BPPData(std::ostream& stream, const uf::stl::vector<Image>& indexedImages, const Palette& palette);

/*
 * This conversion basically has three modes:
 *
 * 1. The source images contain <= unique colors than the requested mode
 *    needs, so conversion will be quick and lossless.
 *
 * 2. The source images contain > unique colors than the requested mode
 *    needs. In this case we utilize vector quantization to reduce the
 *    color count.
 *
 * 3. The user has requested for the image to be compressed. This is a two
 *    stage process. First, reduce the input images to the color count needed.
 *    Then, using the reduced images as input, perform vector quantization
 *    with a vector dimension of 32 or 64 (2x4 or 4x4 pixel blocks).
 */

void convertPaletted(std::ostream& stream, const ImageContainer& images, int textureType, const uf::stl::string& palFilename) {
	const int maxColors = isFormat(textureType, PIXELFORMAT_PAL4BPP) ? 16 : 256;
	Palette palette(images);
	uf::stl::vector<Image> indexedImages;

	std::cout<<"Palette contains "<<palette.colorCount()<<" colors\n";
	if (palette.colorCount()>maxColors) {
		std::cout<<"Reducing palette to "<<maxColors<<" colors\n";
		palette.clear();
		VectorQuantizer<4> vq;
		uf::stl::vector<Vec<4>> vectors;
		vectorizeARGB(images,vectors);
		vq.compress(vectors,maxColors);
		devectorizeARGB(images,vectors,vq,indexedImages,palette);
	} else {
		
		for (int i=0;i<images.imageCount();i++) {
			const Image& img=images.getByIndex(i);
			Image idx(img.width(),img.height());
			idx.allocateIndexed(maxColors);
			for (int y=0;y<img.height();y++)
				for (int x=0;x<img.width();x++) {
					uint32_t color=(img.pixel(x,y).a<<24)|(img.pixel(x,y).r<<16)|(img.pixel(x,y).g<<8)|img.pixel(x,y).b;
					idx.setIndexedPixel(x,y,(uint8_t)palette.indexOf(color));
				}
			indexedImages.push_back(idx);
		}
	}

	palette.save(palFilename);

	if (textureType & FLAG_COMPRESSED) {
		if (isFormat(textureType, PIXELFORMAT_PAL4BPP))
			writeCompressed4BPPData(stream, indexedImages, palette);
		if (isFormat(textureType, PIXELFORMAT_PAL8BPP))
			writeCompressed8BPPData(stream, indexedImages, palette);
	} else {
		if (isFormat(textureType, PIXELFORMAT_PAL4BPP))
			writeUncompressed4BPPData(stream, indexedImages);
		if (isFormat(textureType, PIXELFORMAT_PAL8BPP))
			writeUncompressed8BPPData(stream, indexedImages);
	}
}




void convertToIndexedImages(const ImageContainer& src, const Palette& pal, uf::stl::vector<Image>& dst) {
	dst.clear();
	for (int i=0; i<src.imageCount(); i++) {
		const Image& img = src.getByIndex(i);
		Image dstImg(img.width(), img.height());
		dstImg.allocateIndexed(pal.colorCount());

		for (int y=0; y<img.height(); y++) {
			for (int x=0; x<img.width(); x++) {
				RGBA px = img.pixel(x,y);
				uint32_t argb = (uint32_t(px.a)<<24)|(uint32_t(px.r)<<16)|(uint32_t(px.g)<<8)|(uint32_t)px.b;
				uint8_t index = (uint8_t)pal.indexOf(argb);
				dstImg.setIndexedPixel(x,y,index);
			}
		}
		dst.push_back(dstImg);
	}
}

void writeUncompressed4BPPData(std::ostream& stream, const uf::stl::vector<Image>& indexedImages) {
	if (indexedImages.size() > 1) {
		writeZeroes(stream, MIPMAP_OFFSET_4BPP);
	}
	for (size_t i=0;i<indexedImages.size();i++) {
		const Image& img=indexedImages[i];

		
		if (img.width()==1) {
			uint8_t val=img.indexedPixelAt(0,0);
			stream.write((char*)&val,1);
			continue;
		}

		Twiddler twiddler(img.width(),img.height());
		int pixels=img.width()*img.height();
		for (int j=0;j<pixels;j+=2) {
			uint8_t vals[2];
			for(int k=0;k<2;k++){
				int tIdx=twiddler.index(j+k);
				int x=tIdx%img.width();
				int y=tIdx/img.width();
				vals[k]=img.indexedPixelAt(x,y)&0xF;
			}
			uint8_t packed=(vals[1]<<4)|(vals[0]);
			stream.write((char*)&packed,1);
		}
	}
}

void writeUncompressed8BPPData(std::ostream& stream, const uf::stl::vector<Image>& indexedImages) {
	if (indexedImages.size() > 1) {
		writeZeroes(stream, MIPMAP_OFFSET_8BPP);
	}
	for (size_t i=0;i<indexedImages.size();i++) {
		const Image& img=indexedImages[i];
		Twiddler twiddler(img.width(),img.height());
		int pixels=img.width()*img.height();
		for (int j=0;j<pixels;j++) {
			int tIdx=twiddler.index(j);
			int x=tIdx%img.width();
			int y=tIdx/img.width();
			uint8_t val=img.indexedPixelAt(x,y);
			stream.write((char*)&val,1);
		}
	}
}



#define STORE_FULL	0	
#define STORE_LEFT	1	
#define STORE_RIGHT	2	

template<uint N>
static void grab2x4Block(const Image& img,const Palette& pal,int x,int y,Vec<N>& vec,int storeMethod){
	static const int indexLUT[3][8]={
		{0,4,8,12,16,20,24,28}, 
		{0,4,16,20,32,36,48,52},
		{8,12,24,28,40,44,56,60}
	};
	int idx=0;
	uint32_t seed=vec.hash();
	for(int yy=y;yy<y+4;yy++){
		for(int xx=x;xx<x+2;xx++){
			uint32_t color=pal.colorAt(img.indexedPixelAt(xx,yy)); 
			RGBA c{(uint8_t)((color>>16)&0xFF),(uint8_t)((color>>8)&0xFF),(uint8_t)(color&0xFF),(uint8_t)((color>>24)&0xFF)};
			vec[indexLUT[storeMethod][idx]+0]=c.a/255.f;
			vec[indexLUT[storeMethod][idx]+1]=c.r/255.f;
			vec[indexLUT[storeMethod][idx]+2]=c.g/255.f;
			vec[indexLUT[storeMethod][idx]+3]=c.b/255.f;
			seed=combineHash(c,seed);
			idx++;
		}
	}
	vec.setHash(seed);
}

static void vectorizePalette(const Palette& pal, uf::stl::vector<Vec<4>>& vectors) {
	for (int i=0; i<pal.colorCount(); i++) {
		Vec<4> vec;
		argb2vec(pal.colorAt(i), vec);
		vectors.push_back(vec);
	}
}

static uint8_t findClosest(const uf::stl::vector<Vec<4>>& vectors, const Vec<4>& vec) {
	uint8_t closestIndex = 0;
	float closestDistance = Vec<4>::distanceSquared(vectors[0], vec);
	for (int i=1; i<vectors.size(); i++) {
		float distance = Vec<4>::distanceSquared(vectors[i], vec);
		if (distance < closestDistance)	{
			closestIndex = (uint8_t)i;
			closestDistance = distance;
		}
	}
	return closestIndex;
}

void writeCompressed4BPPData(std::ostream& stream,const uf::stl::vector<Image>& indexedImages,const Palette& palette){
	VectorQuantizer<64> vq;
	uf::stl::vector<Vec<64>> vectors;

	if(indexedImages.size()>1){
		Vec<64> vec; vec.zero();
		for(size_t i=0;i<indexedImages.size();i++){
			const Image& img=indexedImages[i];
			if(img.width()<MIN_MIPMAP_PALVQ||img.height()<MIN_MIPMAP_PALVQ) continue;
			int blocks=(img.width()*img.height())/16;
			Twiddler twiddler(img.width()/4,img.height()/4);
			for(int j=0;j<blocks;j++){
				int tw=twiddler.index(j);
				int x=(tw%(img.width()/4))*4;
				int y=(tw/(img.width()/4))*4;
				if(vectors.empty()){
					grab2x4Block(img,palette,x,y,vec,STORE_LEFT);
				}
				grab2x4Block(img,palette,x,y,vec,STORE_RIGHT);
				vectors.push_back(vec);
				vec.setHash(0);
				grab2x4Block(img,palette,x+2,y,vec,STORE_LEFT);
				if(i==indexedImages.size()-1 && j==blocks-1){
					grab2x4Block(img,palette,x+2,y,vec,STORE_RIGHT);
					vectors.push_back(vec);
				}
			}
		}
	}else{
		const Image& img=indexedImages[0];
		int blocks=(img.width()*img.height())/16;
		Twiddler twiddler(img.width()/4,img.height()/4);
		for(int j=0;j<blocks;j++){
			int tw=twiddler.index(j);
			int x=(tw%(img.width()/4))*4;
			int y=(tw/(img.width()/4))*4;
			Vec<64> vec; vec.zero();
			grab2x4Block(img,palette,x,y,vec,STORE_LEFT);
			grab2x4Block(img,palette,x+2,y,vec,STORE_RIGHT);
			vectors.push_back(vec);
		}
	}

	vq.compress(vectors,256);

	
	uf::stl::vector<Vec<4>> paletteVecs;
	for(int i=0;i<palette.colorCount();i++){
		Vec<4> v; v[0]=((palette.colorAt(i)>>24)&0xFF)/255.f;
		v[1]=((palette.colorAt(i)>>16)&0xFF)/255.f;
		v[2]=((palette.colorAt(i)>>8)&0xFF)/255.f;
		v[3]=((palette.colorAt(i)>>0)&0xFF)/255.f;
		paletteVecs.push_back(v);
	}

	uint8_t codebook[2048]; memset(codebook,0,sizeof(codebook));
	Twiddler nibbleLUT(4,4);
	for(int i=0;i<vq.codeCount();i++){
		const Vec<64>& vec=vq.codeVector(i);
		for(int j=0;j<16;j++){
			Vec<4> col;
			int base=nibbleLUT.index(j)*4;
			for(int k=0;k<4;k++) col[k]=vec[base+k];
			
			int closest=0;
			float best=Vec<4>::distanceSquared(paletteVecs[0],col);
			for(size_t pi=1;pi<paletteVecs.size();pi++){
				float d=Vec<4>::distanceSquared(paletteVecs[pi],col);
				if(d<best){best=d;closest=(int)pi;}
			}
			int byte=j/2, nib=j%2;
			if(nib==1) codebook[i*8+byte]|=(closest&0xF)<<4;
			else	   codebook[i*8+byte]|=(closest&0xF);
		}
	}

	stream.write((char*)codebook,2048);

	
	for(const auto& vec:vectors){
		int codeIdx=vq.findClosest(vec);
		uint8_t c=(uint8_t)codeIdx;
		stream.write((char*)&c,1);
	}
}



void writeCompressed8BPPData(std::ostream& stream,const uf::stl::vector<Image>& indexedImages,const Palette& palette){
	VectorQuantizer<32> vq;
	uf::stl::vector<Vec<32>> vectors;

	for(const auto& img:indexedImages){
		if(img.width()<MIN_MIPMAP_PALVQ||img.height()<MIN_MIPMAP_PALVQ) continue;
		int blocks=(img.width()*img.height())/16;
		Twiddler twiddler(img.width()/4,img.height()/4);
		for(int j=0;j<blocks;j++){
			int tw=twiddler.index(j);
			int x=(tw%(img.width()/4))*4; int y=(tw/(img.width()/4))*4;
			Vec<32> v1; v1.zero(); grab2x4Block(img,palette,x,y,v1,STORE_FULL); vectors.push_back(v1);
			Vec<32> v2; v2.zero(); grab2x4Block(img,palette,x+2,y,v2,STORE_FULL); vectors.push_back(v2);
		}
	}
	vq.compress(vectors,256);

	
	uf::stl::vector<Vec<4>> paletteVecs;
	for(int i=0;i<palette.colorCount();i++){
		Vec<4> v; v[0]=((palette.colorAt(i)>>24)&0xFF)/255.f;
		v[1]=((palette.colorAt(i)>>16)&0xFF)/255.f;
		v[2]=((palette.colorAt(i)>>8)&0xFF)/255.f;
		v[3]=((palette.colorAt(i)>>0)&0xFF)/255.f;
		paletteVecs.push_back(v);
	}

	uint8_t codebook[2048]; memset(codebook,0,sizeof(codebook));
	Twiddler nibbleLUT(2,4);
	for(int i=0;i<vq.codeCount();i++){
		const Vec<32>& vec=vq.codeVector(i);
		for(int j=0;j<8;j++){
			Vec<4> col;
			int base=nibbleLUT.index(j)*4;
			for(int k=0;k<4;k++) col[k]=vec[base+k];
			int closest=0;
			float best=Vec<4>::distanceSquared(paletteVecs[0],col);
			for(size_t pi=1;pi<paletteVecs.size();pi++){
				float d=Vec<4>::distanceSquared(paletteVecs[pi],col);
				if(d<best){best=d;closest=(int)pi;}
			}
			codebook[i*8+j]=(uint8_t)closest;
		}
	}
	stream.write((char*)codebook,2048);

	
	if(indexedImages.size()>1) writeZeroes(stream,1);

	
	for(const auto& vec:vectors){
		uint8_t idx=(uint8_t)vq.findClosest(vec);
		stream.write((char*)&idx,1);
	}
}
#endif