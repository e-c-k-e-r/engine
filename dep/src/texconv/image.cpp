#include <uf/config.h>
#if UF_USE_DC_TEXCONV

//#include <iostream>

#include <stb/stb_image.h>
#include <stb/stb_image_write.h>

#include <texconv/image.h>

Image::Image() : w(0), h(0), indexedMode(false) {}
Image::Image(int width, int height, const uf::stl::vector<RGBA>& pixels) : w(width), h(height), p(pixels), indexedMode(false) {
	p.resize(w*h);
}

bool Image::loadFromBuffer(const uf::stl::vector<RGBA>& pixels, int width, int height) {
	w = width;
	h = height;
	p = pixels;

	return true;
}
bool Image::loadFromFile(const uf::stl::string& path ) {
	int channels;
	uint8_t* pixels = stbi_load(path.c_str(), &w, &h, &channels, STBI_rgb_alpha);

	if ( !pixels ) {
		//std::cerr<<"[ERROR] Failed to load image: "<<path<<"\n";
		return false;
	}

	indexedMode=false;
	p.resize(w*h);
	std::memcpy(p.data(), pixels, w*h*4);
	stbi_image_free(pixels);

	return true;
}


bool Image::saveToFile(const uf::stl::string& path) const {
	uf::stl::vector<uint8_t> pixels(w*h*4);
	for(int y=0;y<h;y++) for(int x=0;x<w;x++) {
		RGBA c=pixel(x,y);
		int idx=(y*w+x)*4;
		pixels[idx+0]=c.r;
		pixels[idx+1]=c.g;
		pixels[idx+2]=c.b;
		pixels[idx+3]=c.a;
	}
	return stbi_write_png(path.c_str(),w,h,4,pixels.data(),w*4)!=0;
}

int Image::width() const { return w; }
int Image::height() const { return h; }
const uf::stl::vector<RGBA>& Image::pixels() const { return p; }

RGBA Image::pixel(int x,int y) const {
	return p[y*w+x];
}


void Image::setPixel(int x,int y, RGBA pixel) {
	if (!indexedMode) {
		p[y*w+x] = pixel;
	}
}

Image Image::scaled(int newW,int newH,bool nearest) const {
	Image out(newW,newH);
	if (nearest) {
		for (int y=0;y<newH;y++) {
			for (int x=0;x<newW;x++) {
				int srcX = x * w / newW;
				int srcY = y * h / newH;
				out.p[y*newW+x] = pixel(srcX,srcY);
			}
		}
	} else {
		for (int y=0;y<newH;y++) {
			for (int x=0;x<newW;x++) {
				float gx = (float)x * (w-1) / (float)(newW-1);
				float gy = (float)y * (h-1) / (float)(newH-1);
				int x0 = (int)gx;
				int y0 = (int)gy;
				int x1 = std::min(x0+1,w-1);
				int y1 = std::min(y0+1,h-1);
				float dx = gx-x0;
				float dy = gy-y0;
				auto lerp=[&](uint8_t a,uint8_t b,float t){ return (uint8_t)(a*(1-t)+b*t); };
				RGBA c00=pixel(x0,y0);
				RGBA c10=pixel(x1,y0);
				RGBA c01=pixel(x0,y1);
				RGBA c11=pixel(x1,y1);
				RGBA top{
					lerp(c00.r,c10.r,dx),
					lerp(c00.g,c10.g,dx),
					lerp(c00.b,c10.b,dx),
					lerp(c00.a,c10.a,dx)};
				RGBA bottom{
					lerp(c01.r,c11.r,dx),
					lerp(c01.g,c11.g,dx),
					lerp(c01.b,c11.b,dx),
					lerp(c01.a,c11.a,dx)};
				out.p[y*newW+x]=RGBA{
					lerp(top.r,bottom.r,dy),
					lerp(top.g,bottom.g,dy),
					lerp(top.b,bottom.b,dy),
					lerp(top.a,bottom.a,dy)};
			}
		}
	}
	return out;
}

void Image::allocateIndexed(int colors) {
	indexedMode=true;
	indexed.assign(w*h,0);
}

void Image::setIndexedPixel(int x,int y,uint8_t index) {
	if(indexedMode) {
		indexed[y*w+x]=index;
	}
}
uint8_t Image::indexedPixelAt(int x,int y) const {
	if(indexedMode) return indexed[y*w+x];
	return 0;
}
#endif