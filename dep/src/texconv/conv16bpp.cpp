#include <uf/config.h>
#if UF_USE_DC_TEXCONV

#include <iostream>

#include <texconv/imagecontainer.h>
#include <texconv/twiddler.h>
#include <texconv/vqtools.h>
#include <texconv/common.h>

static void convertAndWriteTexel(std::ostream& stream, const RGBA& texel, int pixelFormat, bool twiddled);
static void writeStrideData(std::ostream& stream, const Image& img, int pixelFormat);
static void writeUncompressedData(std::ostream& stream, const ImageContainer& images, int pixelFormat);
static void writeCompressedData(std::ostream& stream, const ImageContainer& images, int pixelFormat);

void convert16BPP(std::ostream& stream, const ImageContainer& images, int textureType) {
	const int pixelFormat = (textureType >> PIXELFORMAT_SHIFT) & PIXELFORMAT_MASK;

	if (textureType & FLAG_STRIDED) {
		writeStrideData(stream, images.getByIndex(0), pixelFormat);
	} else if (textureType & FLAG_COMPRESSED) {
		writeCompressedData(stream, images, pixelFormat);
	} else {
		writeUncompressedData(stream, images, pixelFormat);
	}
}

static void convertAndWriteTexel(std::ostream& stream, const RGBA& texel, int pixelFormat, bool twiddled) {
	if (pixelFormat == PIXELFORMAT_YUV422) {
		static int index = 0;
		static RGBA savedTexel[3];

		if (!twiddled && index == 1) {
			uint16_t yuv[2];
			RGBtoYUV422(savedTexel[0], texel, yuv[0], yuv[1]);
			stream.write(reinterpret_cast<char*>(&yuv[0]), 2);
			stream.write(reinterpret_cast<char*>(&yuv[1]), 2);
			index = 0;
		} else if (twiddled && index == 3) {
			uint16_t yuv[4];
			RGBtoYUV422(savedTexel[0], savedTexel[2], yuv[0], yuv[2]);
			RGBtoYUV422(savedTexel[1], texel, yuv[1], yuv[3]);
			stream.write(reinterpret_cast<char*>(&yuv[0]), 2);
			stream.write(reinterpret_cast<char*>(&yuv[1]), 2);
			stream.write(reinterpret_cast<char*>(&yuv[2]), 2);
			stream.write(reinterpret_cast<char*>(&yuv[3]), 2);
			index = 0;
		} else {
			savedTexel[index] = texel;
			index++;
		}
	} else {
		uint16_t val = to16BPP(texel, pixelFormat);
		stream.write(reinterpret_cast<char*>(&val), 2);
	}
}





static void writeStrideData(std::ostream& stream, const Image& img, int pixelFormat) {
	for (int y=0; y<img.height(); y++) {
		for (int x=0; x<img.width(); x++) {
			convertAndWriteTexel(stream, img.pixel(x,y), pixelFormat, false);
		}
	}
}





static void writeUncompressedData(std::ostream& stream, const ImageContainer& images, int pixelFormat) {
	
	if (images.hasMipmaps()) {
		writeZeroes(stream, MIPMAP_OFFSET_16BPP);
	}

	
	for (int i=0; i<images.imageCount(); i++) {
		const Image& img = images.getByIndex(i);

		
		if (img.width()==1 && img.height()==1 && pixelFormat == PIXELFORMAT_YUV422) {
			convertAndWriteTexel(stream, img.pixel(0,0), PIXELFORMAT_RGB565, true);
			continue;
		}

		Twiddler twiddler(img.width(), img.height());
		int pixels = img.width() * img.height();

		
		for (int j=0;j<pixels;j++) {
			int index = twiddler.index(j);
			int x = index % img.width();
			int y = index / img.width();
			convertAndWriteTexel(stream, img.pixel(x,y), pixelFormat, true);
		}
	}
}







static uint64_t packQuad(const RGBA& tl, const RGBA& tr,
						 const RGBA& bl, const RGBA& br, int pixelFormat) {
	uint64_t a,b,c,d;
	if (pixelFormat == PIXELFORMAT_YUV422) {
		uint16_t yuv[4];
		RGBtoYUV422(tl,tr,yuv[0],yuv[1]);
		RGBtoYUV422(bl,br,yuv[2],yuv[3]);
		a=yuv[0]; b=yuv[1]; c=yuv[2]; d=yuv[3];
	} else {
		a=to16BPP(tl,pixelFormat);
		b=to16BPP(tr,pixelFormat);
		c=to16BPP(bl,pixelFormat);
		d=to16BPP(br,pixelFormat);
	}
	return (a<<48)|(b<<32)|(c<<16)|d;
}


static int encodeLossless(const ImageContainer& images,
						  int pixelFormat,
						  uf::stl::vector<Image>& indexedImages,
						  uf::stl::vector<uint64_t>& codebook,
						  int maxCodes) {
	uf::stl::unordered_map<uint64_t,int> uniqueQuads;

	for (int i=0;i<images.imageCount();i++) {
		const Image& img=images.getByIndex(i);

		if (img.width()<MIN_MIPMAP_VQ || img.height()<MIN_MIPMAP_VQ)
			continue;

		Image indexed(img.width()/2, img.height()/2); 
		indexed.allocateIndexed(256); 

		for (int y=0;y<img.height();y+=2) {
			for (int x=0;x<img.width();x+=2) {
				uint64_t quad = packQuad(img.pixel(x,y),
										 img.pixel(x+1,y),
										 img.pixel(x,y+1),
										 img.pixel(x+1,y+1),
										 pixelFormat);
				if (uniqueQuads.find(quad)==uniqueQuads.end())
					uniqueQuads[quad]=(int)uniqueQuads.size();

				if ((int)uniqueQuads.size()<=maxCodes)
					indexed.setIndexedPixel(x/2,y/2,uniqueQuads[quad]);
			}
		}

		if ((int)uniqueQuads.size()<=maxCodes)
			indexedImages.push_back(indexed);
	}

	if ((int)uniqueQuads.size()<=maxCodes) {
		codebook.resize(uniqueQuads.size());
		for (auto& kv:uniqueQuads) codebook[kv.second]=kv.first;
	} else {
		indexedImages.clear();
	}

	return (int)uniqueQuads.size();
}



static void writeCompressedData(std::ostream& stream, const ImageContainer& images, int pixelFormat) {
	uf::stl::vector<Image> indexedImages;
	uf::stl::vector<uint64_t> codebook;

	int numQuads = encodeLossless(images, pixelFormat, indexedImages, codebook, 256);
	std::cout << "Source images contain " << numQuads << " unique quads\n";

	if (numQuads > 256) {
		if ((pixelFormat != PIXELFORMAT_ARGB1555) && (pixelFormat != PIXELFORMAT_ARGB4444)) {
			
			uf::stl::vector<Vec<12>> vectors;
			VectorQuantizer<12> vq;
			
			for (int i=0; i<images.imageCount(); i++) {
				const Image& img=images.getByIndex(i);
				if (img.width()<MIN_MIPMAP_VQ || img.height()<MIN_MIPMAP_VQ) continue;

				for (int y=0;y<img.height();y+=2) {
					for(int x=0;x<img.width();x+=2) {
						Vec<12> vec;
						int offset=0;
						for(int yy=0;yy<2;yy++){
							for(int xx=0;xx<2;xx++){
								RGBA px=img.pixel(x+xx,y+yy);
								vec[offset+0]=px.r/255.0f;
								vec[offset+1]=px.g/255.0f;
								vec[offset+2]=px.b/255.0f;
								offset+=3;
							}
						}
						vectors.push_back(vec);
					}
				}
			}
			vq.compress(vectors,256);

			
			for (int i=0;i<vq.codeCount();i++) {
				const Vec<12>& vec=vq.codeVector(i);
				RGBA tl{(uint8_t)(vec[0]*255),(uint8_t)(vec[1]*255),(uint8_t)(vec[2]*255),255};
				RGBA tr{(uint8_t)(vec[3]*255),(uint8_t)(vec[4]*255),(uint8_t)(vec[5]*255),255};
				RGBA bl{(uint8_t)(vec[6]*255),(uint8_t)(vec[7]*255),(uint8_t)(vec[8]*255),255};
				RGBA br{(uint8_t)(vec[9]*255),(uint8_t)(vec[10]*255),(uint8_t)(vec[11]*255),255};
				codebook.push_back(packQuad(tl,tr,bl,br,pixelFormat));
			}

			
			for (int i=0; i<images.imageCount(); i++) {
				const Image& src=images.getByIndex(i);
				if (src.width()<MIN_MIPMAP_VQ || src.height()<MIN_MIPMAP_VQ) continue;
				Image idx(src.width()/2, src.height()/2);
				idx.allocateIndexed(256);
				for(int y=0;y<src.height();y+=2){
					for(int x=0;x<src.width();x+=2){
						Vec<12> v;
						int off=0;
						for(int yy=0;yy<2;yy++)for(int xx=0;xx<2;xx++){
							RGBA p=src.pixel(x+xx,y+yy);
							v[off+0]=p.r/255.0f;
							v[off+1]=p.g/255.0f;
							v[off+2]=p.b/255.0f;
							off+=3;
						}
						int codeIdx=vq.findClosest(v);
						idx.setIndexedPixel(x/2,y/2,(uint8_t)codeIdx);
					}
				}
				indexedImages.push_back(idx);
			}
		} else {
			
			std::cerr<<"ARGB VQ compression not yet implemented!\n";
		}
	}

	
	uint16_t codes[256*4];
	memset(codes,0,sizeof(codes));
	for (int i=0;i<(int)codebook.size();i++) {
		uint64_t quad=codebook[i];
		codes[i*4+0]=(uint16_t)((quad>>48)&0xFFFF);
		codes[i*4+1]=(uint16_t)((quad>>32)&0xFFFF);
		codes[i*4+2]=(uint16_t)((quad>>16)&0xFFFF);
		codes[i*4+3]=(uint16_t)((quad>> 0)&0xFFFF);
	}

	
	for(int i=0;i<1024;i++){
		stream.write((char*)&codes[i],2);
	}

	
	if(images.imageCount()>1)
		writeZeroes(stream,1);

	
	for(const auto& img:indexedImages){
		Twiddler twiddler(img.width(),img.height());
		int pixels=img.width()*img.height();
		for(int j=0;j<pixels;j++){
			int idx=twiddler.index(j);
			uint8_t val=img.indexedPixelAt(idx%img.width(),idx/img.width());
			stream.write((char*)&val,1);
		}
	}
}
#endif