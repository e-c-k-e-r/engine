#include <uf/utils/text/glyph.h>

uf::Glyph::~Glyph() {
	delete[] this->m_buffer;
}

// 	OpenGL ops
uint8_t* uf::Glyph::generate( const std::string& font, unsigned long c, uint size ) {
	ext::freetype::Glyph glyph = ext::freetype::initialize(font);
	return this->generate( glyph, c );
}
uint8_t* uf::Glyph::generate( ext::freetype::Glyph& glyph, unsigned long c, uint size ) {
	ext::freetype::setPixelSizes( glyph, size );
	if ( !ext::freetype::load( glyph, c ) ) return NULL;
	if ( this->m_buffer ) {
		delete[] this->m_buffer;
	}

//	this->m_sdf = false;
//	this->setSize( { static_cast<int>(glyph.face->glyph->metrics.width) >> 6, static_cast<int>(glyph.face->glyph->metrics.height) >> 6 } );
	this->setSize( { static_cast<int>(glyph.face->glyph->bitmap.width), static_cast<int>(glyph.face->glyph->bitmap.rows) } );
	this->setBearing( { glyph.face->glyph->bitmap_left, glyph.face->glyph->bitmap_top } );
	this->setAdvance( {static_cast<int>(glyph.face->glyph->advance.x) >> 6, static_cast<int>(glyph.face->glyph->advance.y) >> 6} );
//	this->setPadding( {4, 4} );

	uint8_t* bitmap = glyph.face->glyph->bitmap.buffer;
	
	pod::Vector2i padding = this->getPadding();
	if ( padding.x > 0 && padding.y > 0 ) {
		uint8_t unheadache[this->m_size.x + padding.x * 2][this->m_size.y + padding.y * 2];
		this->m_buffer = new uint8_t[ ( this->m_size.x + padding.x * 2 ) * ( this->m_size.y + padding.y * 2 ) ];

		// Zero out
		for ( uint y = 0; y < this->m_size.y + padding.y * 2; ++y ) {
			for ( uint x = 0; x < this->m_size.x + padding.x * 2; ++x ) {
				unheadache[x][y] = 0;
			}
		}
		// Fill
		for ( uint y = 0; y < this->m_size.y; ++y ) {
			for ( uint x = 0; x < this->m_size.x; ++x ) {
				unheadache[x + padding.x][y + padding.y] = bitmap[y * this->m_size.x + x];
			}
		}
		// Migrate
		this->setSize( pod::Vector2ui{ this->m_size.x + padding.x * 2, this->m_size.y + padding.y * 2 } );
		for ( uint y = 0; y < this->m_size.y; ++y ) {
			for ( uint x = 0; x < this->m_size.x; ++x ) {
				this->m_buffer[y * this->m_size.x + x] = unheadache[x][y];
			}
		}
	} else {
		std::size_t len = this->m_size.x * this->m_size.y;
		this->m_buffer = new uint8_t[len];
		memcpy(this->m_buffer, bitmap, len);
	}

	if ( this->isSdf() ) this->generateSdf(this->m_buffer);
	return this->m_buffer;
}
uint8_t* uf::Glyph::generate( const std::string& font, const uf::String& c, uint size ) {
	ext::freetype::Glyph glyph = ext::freetype::initialize(font);
	return this->generate( glyph, c );
}
uint8_t* uf::Glyph::generate( ext::freetype::Glyph& glyph, const uf::String& c, uint size ) {
	ext::freetype::setPixelSizes( glyph, size );
	if ( !ext::freetype::load( glyph, c ) ) return NULL;
	if ( this->m_buffer ) {
		delete[] this->m_buffer;
	}

	this->setSize( { static_cast<int>(glyph.face->glyph->bitmap.width), static_cast<int>(glyph.face->glyph->bitmap.rows) } );
	this->setBearing( { glyph.face->glyph->bitmap_left, glyph.face->glyph->bitmap_top } );
	this->setAdvance( {static_cast<int>(glyph.face->glyph->advance.x) >> 6, static_cast<int>(glyph.face->glyph->advance.y) >> 6} );

	uint8_t* bitmap = glyph.face->glyph->bitmap.buffer;
	std::size_t len = this->m_size.x * this->m_size.y;
	this->m_buffer = new uint8_t[len];
	memcpy(this->m_buffer, bitmap, len);

	if ( this->isSdf() ) this->generateSdf(this->m_buffer);

	return this->m_buffer;
}
bool uf::Glyph::generated() {
	return this->m_buffer != NULL;
}
void uf::Glyph::generateSdf( uint8_t* buffer ) { if ( !buffer ) return;
	pod::Vector2ui size = this->getSize();
	this->m_sdf = true;

	int HEIGHT = size.y; int WIDTH = size.x;
	struct Point {
		int dx, dy;
		int DistSq() const { return dx*dx + dy*dy; }
	};
	struct Grid {
		int w, h;
		Point grid[128][128];
	/*
		Point** grid;
		Grid( int w, int h ) { this->w = w; this->h = h; this->grid = new Point*[h]; for (int y = 0; y < h; ++y ) this->grid[y] = new Point[w]; }
		~Grid() { std::cout << this->grid << std::endl; for ( int y = 0; y < this->w; ++y ) if ( this->grid[y] ) delete[] this->grid[y]; if ( this->grid ) delete[] this->grid; }
	*/
	};

	Point inside = { 0, 0 };
	Point empty = { 9999, 9999 };
	Grid grid1; //( WIDTH, HEIGHT );
	Grid grid2; //( WIDTH, HEIGHT );

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


	for ( int y = 0; y < this->m_size.y; ++y ) {
		for ( int x = 0; x < this->m_size.x; ++x ) {
			int a = buffer[y * this->m_size.x + x];
			Put( grid1, x, y, a < 128 ? inside : empty );
			Put( grid2, x, y, a < 128 ? empty : inside );
		}
	}

	GenerateSDF( grid1 );
	GenerateSDF( grid2 );

	int lowest = 255;
	int highest = 0;
	for ( uint y = 0; y < this->m_size.y; ++y ) {
		for ( uint x = 0; x < this->m_size.x; ++x ) {
			int dist1 = (int)( sqrt( (double)Get( grid1, x, y ).DistSq() ) );
			int dist2 = (int)( sqrt( (double)Get( grid2, x, y ).DistSq() ) );
			int dist = dist1 - dist2;

			lowest = std::min( lowest, dist );
			highest = std::max( highest, dist );

			buffer[y * this->m_size.x + x] = dist * this->getSpread() + 128;
		}
	}
}
// 	Get
const pod::Vector2ui& uf::Glyph::getSize() const {
	return this->m_size;
}
const pod::Vector2i& uf::Glyph::getBearing() const {
	return this->m_bearing;
}
const pod::Vector2i& uf::Glyph::getAdvance() const {
	return this->m_advance;
}
const pod::Vector2i& uf::Glyph::getPadding() const {
	return this->m_padding;
}
const uint8_t* uf::Glyph::getBuffer() const {
	return this->m_buffer;
}
int uf::Glyph::getSpread() const {
	return this->m_spread;
}
bool uf::Glyph::isSdf() const {
	return this->m_sdf;
}
//	Set
void uf::Glyph::setSize( const pod::Vector2ui& size ) {
	this->m_size = size;
}
void uf::Glyph::setBearing( const pod::Vector2i& bearing ) {
	this->m_bearing = bearing;
}
void uf::Glyph::setAdvance( const pod::Vector2i& advance ) {
	this->m_advance = advance;
}
void uf::Glyph::setPadding( const pod::Vector2i& padding ) {
	this->m_padding = padding;
}
void uf::Glyph::setSpread( int spread ) {
	this->m_spread = spread;
} 
void uf::Glyph::useSdf( bool b ) {
	this->m_sdf = b;
}