#include <uf/utils/text/glyph.h>
#include <stb_truetype.h>
#include <cmath>
#include <algorithm>

#if UF_USE_TRUETYPE

uint8_t* uf::glyph::generate( pod::Glyph& glyph, pod::TrueTypeFont& g, uint64_t c, size_t size ) {
	ext::truetype::setPixelSizes( g, size );
	if ( !ext::truetype::load( g, c ) ) return NULL;
	return uf::glyph::generate( glyph, g );
}

uint8_t* uf::glyph::generate( pod::Glyph& glyph, pod::TrueTypeFont& g, const uf::stl::string& s, size_t size ) {
	ext::truetype::setPixelSizes( g, size );
	if ( !ext::truetype::load( g, s ) ) return NULL;
	return uf::glyph::generate( glyph, g );
}

uint8_t* uf::glyph::generate( pod::Glyph& glyph, pod::TrueTypeFont& g ) {
	if ( !g.info ) return nullptr;
	stbtt_fontinfo* info = static_cast<stbtt_fontinfo*>(g.info);
	int c = static_cast<int>(g.current_codepoint);

	int advance, lsb;
	stbtt_GetCodepointHMetrics(info, c, &advance, &lsb);

	int width, height, xoff, yoff;
	unsigned char* bitmap = nullptr;

	// this should never ever be called because I'll probably never get around to implementing SDFs in pure fixed-function OpenGL
	// but this causes issues with -flto on it
#if !UF_ENV_DREAMCAST
	if ( glyph.spread > 0 ) {
		bitmap = stbtt_GetCodepointSDF(info, g.scale, c, glyph.padding.x, 128, static_cast<float>(glyph.spread), &width, &height, &xoff, &yoff);
	} else 
#endif
	{
		bitmap = stbtt_GetCodepointBitmap(info, 0, g.scale, c, &width, &height, &xoff, &yoff);
	}

	if ( !bitmap ) {
		width = height = xoff = yoff = 0;
	}

	glyph.size = { static_cast<unsigned int>(width), static_cast<unsigned int>(height) };
	glyph.bearing = { xoff, -yoff };
	glyph.advance = { static_cast<int>(std::round(advance * g.scale)), 0 };

	auto size_padded = glyph.size + glyph.padding * 2;
	glyph.buffer.assign(size_padded.x * size_padded.y, 0);

	if ( bitmap ) {
		for ( size_t y = 0; y < glyph.size.y; ++y ) {
			const uint8_t* src = bitmap + y * width;
			for ( size_t x = 0; x < glyph.size.x; ++x ) {
				size_t dst = (y + glyph.padding.y) * size_padded.x + (x + glyph.padding.x);
				glyph.buffer[dst] = src[x];
			}
		}
		stbtt_FreeBitmap(bitmap, nullptr);
	}

	glyph.size = size_padded;

#if UF_ENV_DREAMCAST
	//if ( glyph.spread > 0 ) uf::glyph::generateSdf( glyph );
#endif

	return glyph.buffer.data();
}

void uf::glyph::generateSdf( pod::Glyph& glyph ) {
	auto* buffer = glyph.buffer.data();
	auto size = glyph.size;

	int HEIGHT = size.y;
	int WIDTH = size.x;
	struct Point {
		int dx, dy;
		int DistSq() const { return dx*dx + dy*dy; }
	};
	struct Grid {
		int w, h;
		Point grid[128][128];
	};

	Point inside = { 0, 0 };
	Point empty = { 9999, 9999 };
	Grid grid1;
	Grid grid2;

	auto Get = [&]( Grid &g, int x, int y )->Point{
		return ( x >= 0 && y >= 0 && x < WIDTH && y < HEIGHT ) ? g.grid[y][x] : empty;
	};
	auto Put = [&]( Grid &g, int x, int y, const Point &p )->void{
		g.grid[y][x] = p;
	};
	auto Compare = [&]( Grid &g, Point &p, int x, int y, int offsetx, int offsety )->void{
		Point other = Get( g, x+offsetx, y+offsety );
		other.dx += offsetx;
		other.dy += offsety;
		if (other.DistSq() < p.DistSq()) p = other;
	};
	auto GenerateSDF = [&]( Grid &g )->void{
		// Pass 0
		for (int y=0;y<HEIGHT;y++) {
			for (int x=0;x<WIDTH;x++) {
				Point p = Get( g, x, y );
				Compare( g, p, x, y, -1,  0 );
				Compare( g, p, x, y,  0, -1 );
				Compare( g, p, x, y, -1, -1 );
				Compare( g, p, x, y,  1, -1 );
				Put( g, x, y, p );
			}
			for (int x=WIDTH-1;x>=0;x--) {
				Point p = Get( g, x, y );
				Compare( g, p, x, y, 1, 0 );
				Put( g, x, y, p );
			}
		}
		// Pass 1
		for (int y=HEIGHT-1;y>=0;y--) {
			for (int x=WIDTH-1;x>=0;x--) {
				Point p = Get( g, x, y );
				Compare( g, p, x, y,  1,  0 );
				Compare( g, p, x, y,  0,  1 );
				Compare( g, p, x, y, -1,  1 );
				Compare( g, p, x, y,  1,  1 );
				Put( g, x, y, p );
			}
			for (int x=0;x<WIDTH;x++) {
				Point p = Get( g, x, y );
				Compare( g, p, x, y, -1, 0 );
				Put( g, x, y, p );
			}
		}
	};


	for ( int y = 0; y < HEIGHT; ++y ) {
		for ( int x = 0; x < WIDTH; ++x ) {
			int a = buffer[y * WIDTH + x];
			Put( grid1, x, y, a < 128 ? inside : empty );
			Put( grid2, x, y, a < 128 ? empty : inside );
		}
	}

	GenerateSDF( grid1 );
	GenerateSDF( grid2 );

	int lowest = 255;
	int highest = 0;
	for ( uint y = 0; y < HEIGHT; ++y ) {
		for ( uint x = 0; x < WIDTH; ++x ) {
			int dist1 = (int)( sqrt( (double)Get( grid1, x, y ).DistSq() ) );
			int dist2 = (int)( sqrt( (double)Get( grid2, x, y ).DistSq() ) );
			int dist = dist1 - dist2;

			lowest = std::min( lowest, dist );
			highest = std::max( highest, dist );
		
			{
				int value = dist * glyph.spread + 128;
				uint8_t uvalue = std::max( 0, std::min(255, value) );
				buffer[y * WIDTH + x] = uvalue;
			}
		}
	}
}
#endif