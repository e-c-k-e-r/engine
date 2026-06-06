#include <uf/utils/text/glyph.h>
#include <iostream>
#if UF_USE_FREETYPE


uint8_t* uf::glyph::generate( pod::Glyph& glyph, pod::FT_Glyph& g, uint64_t c, size_t size ) {
	ext::freetype::setPixelSizes( g, size );
	if ( !ext::freetype::load( g, c ) ) return NULL;
	return uf::glyph::generate( glyph, g );
}

uint8_t* uf::glyph::generate( pod::Glyph& glyph, pod::FT_Glyph& g, const uf::stl::string& s, size_t size ) {
	ext::freetype::setPixelSizes( g, size );
	if ( !ext::freetype::load( g, s ) ) return NULL;
	return uf::glyph::generate( glyph, g );
}

uint8_t* uf::glyph::generate( pod::Glyph& glyph, pod::FT_Glyph& g ) {
	if ( glyph.spread ) ext::freetype::setRenderMode( g, FT_RENDER_MODE_SDF );

	glyph.size = { g.face->glyph->bitmap.width, g.face->glyph->bitmap.rows };
	glyph.bearing = { g.face->glyph->bitmap_left, g.face->glyph->bitmap_top };
	glyph.advance = { g.face->glyph->advance.x >> 6, g.face->glyph->advance.y >> 6 };

	glyph.buffer.clear();

	const uint8_t* buffer = g.face->glyph->bitmap.buffer;

	auto size = glyph.size + glyph.padding * 2;
	glyph.buffer.assign(size.x * size.y, 0);
	for ( size_t y = 0; y < glyph.size.y; ++y ) {
		const uint8_t* src = buffer + y * g.face->glyph->bitmap.pitch;
		for ( size_t x = 0; x < glyph.size.x; ++x ) {
			size_t dst = (y + glyph.padding.y) * size.x + (x + glyph.padding.x);
			glyph.buffer[dst] = src[x];
		}
	}

	glyph.size = size;

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