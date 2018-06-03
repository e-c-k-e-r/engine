#include <uf/gl/glyph/glyph.h>

// 	OpenGL ops
void spec::ogl::GlyphTexture::generate( const std::string& font, unsigned long c, uint size ) {
	ext::freetype::Glyph glyph = ext::freetype::initialize(font);
	this->generate( glyph, c );
}
void spec::ogl::GlyphTexture::generate( ext::freetype::Glyph& glyph, unsigned long c, uint size ) {
	ext::freetype::setPixelSizes( glyph, size );
	if ( !ext::freetype::load( glyph, c ) ) return;

	if ( this->m_index ) this->destroy();

	this->m_sdf = false;
	this->setSize( { glyph.face->glyph->bitmap.width, glyph.face->glyph->bitmap.rows } );
	this->setBearing( { glyph.face->glyph->bitmap_left, glyph.face->glyph->bitmap_top } );
	this->setAdvance( {glyph.face->glyph->advance.x, glyph.face->glyph->advance.y} );
//	this->setPadding( {4, 4} );

	GLuint type, format = GL_RED, internalFormat = GL_RED;
	type = GL_UNSIGNED_BYTE;
	uint8_t* buffer = glyph.face->glyph->bitmap.buffer;
	
	bool cleanup = false;
	pod::Vector2ui padding = this->getPadding();
	if ( padding.x > 0 && padding.y > 0 ) { cleanup = true;
		uint8_t* bitmap = buffer;
		uint8_t unheadache[this->m_size.x + padding.x * 2][this->m_size.y + padding.y * 2];
		buffer = new uint8_t[ ( this->m_size.x + padding.x * 2 ) * ( this->m_size.y + padding.y * 2 ) ];

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
				buffer[y * this->m_size.x + x] = unheadache[x][y];
			}
		}
	}

	if ( this->isSdf() ) this->generateSdf(buffer);
	
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glGenTextures( 1, &this->m_index );
	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, this->m_index );
	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		internalFormat,
		this->m_size.x,
		this->m_size.y,
		0,
		format,
		type,
		buffer
	);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	if ( cleanup ) delete[] buffer;
}
void spec::ogl::GlyphTexture::generate( const std::string& font, const uf::String& c, uint size ) {
	ext::freetype::Glyph glyph = ext::freetype::initialize(font);
	this->generate( glyph, c );
}
void spec::ogl::GlyphTexture::generate( ext::freetype::Glyph& glyph, const uf::String& c, uint size ) {
	ext::freetype::setPixelSizes( glyph, size );
	if ( !ext::freetype::load( glyph, c ) ) return;

	if ( this->m_index ) this->destroy();

	this->setSize( { glyph.face->glyph->bitmap.width, glyph.face->glyph->bitmap.rows } );
	this->setBearing( { glyph.face->glyph->bitmap_left, glyph.face->glyph->bitmap_top } );
	this->setAdvance( {glyph.face->glyph->advance.x, glyph.face->glyph->advance.y} );

	GLuint type, format = GL_RED, internalFormat = GL_RED;
	type = GL_UNSIGNED_BYTE;
	uint8_t* buffer = glyph.face->glyph->bitmap.buffer;

	if ( this->isSdf() ) this->generateSdf(buffer);
	
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glGenTextures( 1, &this->m_index );
	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, this->m_index );
	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		internalFormat,
		this->m_size.x,
		this->m_size.y,
		0,
		format,
		type,
		buffer
	);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}
void spec::ogl::GlyphTexture::generateSdf( uint8_t* buffer ) { if ( !buffer ) return;
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
const pod::Vector2ui& spec::ogl::GlyphTexture::getSize() const {
	return this->m_size;
}
const pod::Vector2ui& spec::ogl::GlyphTexture::getBearing() const {
	return this->m_bearing;
}
const pod::Vector2i& spec::ogl::GlyphTexture::getAdvance() const {
	return this->m_advance;
}
const pod::Vector2ui& spec::ogl::GlyphTexture::getPadding() const {
	return this->m_padding;
}
int spec::ogl::GlyphTexture::getSpread() const {
	return this->m_spread;
}
bool spec::ogl::GlyphTexture::isSdf() const {
	return this->m_sdf;
}
//	Set
void spec::ogl::GlyphTexture::setSize( const pod::Vector2ui& size ) {
	this->m_size = size;
}
void spec::ogl::GlyphTexture::setBearing( const pod::Vector2ui& bearing ) {
	this->m_bearing = bearing;
}
void spec::ogl::GlyphTexture::setAdvance( const pod::Vector2i& advance ) {
	this->m_advance = advance;
}
void spec::ogl::GlyphTexture::setPadding( const pod::Vector2ui& padding ) {
	this->m_padding = padding;
}
void spec::ogl::GlyphTexture::setSpread( int spread ) {
	this->m_spread = spread;
} 
void spec::ogl::GlyphTexture::useSdf( bool b ) {
	this->m_sdf = b;
} 

// OpenGL ops
void spec::ogl::GlyphMesh::generate() {
	if ( this->generated() ) {
		this->m_vbo.subBuffer();
		return;
	}
	this->m_vao.generate();
	this->m_vao.bind();
	if ( !this->m_vbo.generated() ) this->m_vbo.generate(GL_DYNAMIC_DRAW, false);
	this->bindAttributes();
}
void spec::ogl::GlyphMesh::bindAttributes() {
	GLuint i = 0;
	if ( this->m_vbo.generated() ) {
		this->m_vbo.bindAttribute(i);
		this->m_vao.bindAttribute(i);
	}
}
void spec::ogl::GlyphMesh::render() {
	this->m_vao.render( this->m_vbo );
}
// OpenGL Getters
bool spec::ogl::GlyphMesh::loaded() const {
	return this->m_vbo.loaded();
}
bool spec::ogl::GlyphMesh::generated() const {
	return this->m_vao.generated();
}
// Move Setters
void spec::ogl::GlyphMesh::setPoints(uf::Vertices4f&& points ) {
	this->m_vbo = std::move(points);
}
// Copy Setters
void spec::ogl::GlyphMesh::setPoints( const uf::Vertices4f& points ) {
	this->m_vbo = points;
}
// Non-const Getters
uf::Vertices4f& spec::ogl::GlyphMesh::getPoints() {
	return this->m_vbo;
}
// Const Getters
const uf::Vertices4f& spec::ogl::GlyphMesh::getPoints() const {
	return this->m_vbo;
}