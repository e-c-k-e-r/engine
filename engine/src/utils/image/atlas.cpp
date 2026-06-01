#include <uf/utils/image/atlas.h>
#include <binpack2d/binpack2d.hpp>
#include <iostream>
#include <bit>

pod::Atlas::hash_t uf::atlas::add( pod::Atlas& atlas, const pod::Image& image, const pod::Atlas::hash_t& hash ) {
	size_t index = atlas.tiles.size();
	if ( atlas.tiles.count( hash ) > 0 ) return hash;

	auto& tile = atlas.tiles[hash];
	tile.image = image;
	return hash;
}
pod::Atlas::hash_t uf::atlas::add( pod::Atlas& atlas, const pod::Image& image ) {
	return uf::atlas::add( atlas, image, uf::image::hash( image ) );
}

void uf::atlas::generate( pod::Atlas& atlas, float padding ) {
	if ( atlas.tiles.empty() ) return;

	BinPack2D::CanvasArray<pod::Atlas::hash_t> internalAtlas;
	BinPack2D::ContentAccumulator<pod::Atlas::hash_t> queue, stored, remainder;
	pod::Vector2ui size = {};
	pod::Vector3ui largest = {};
	size_t index = 0;
	size_t area = 0;
	size_t channels = 1;
	for ( auto& [ hash, tile ] : atlas.tiles ) {
		auto& dim = tile.image.size;
		channels = std::max( channels, tile.image.channels );
		queue += BinPack2D::Content<pod::Atlas::hash_t>(hash, BinPack2D::Coord(), BinPack2D::Size(dim.x, dim.y), false );
		size += dim;
		area += dim.x * dim.y;
		if ( area >= largest.z ) {
			largest.x = dim.x;
			largest.y = dim.y;
		}
	}
	size_t tries = 16;
	do {
		size_t side = std::sqrt( area ) * padding;
		size = { std::bit_ceil(side), std::bit_ceil(side) }; // to-do: non-C++20 method
		queue.Sort();
		internalAtlas = BinPack2D::UniformCanvasArrayBuilder<pod::Atlas::hash_t>(size.x, size.y, 1).Build();
		bool success = internalAtlas.Place( queue, remainder );
		if ( success && remainder.Get().empty() ) break;
		// increase padding
		padding += 0.10f;
	} while ( --tries );
	internalAtlas.CollectContent( stored );

	uf::image::load( atlas.image, NULL, size, 8, channels );
	auto& dstBuffer = atlas.image.pixels;
	for ( size_t i = 0; i < size.x * size.y * channels; ++i ) dstBuffer[i] = 0;
	for ( auto& it : stored.Get() ) {
		auto& tile = atlas.tiles[it.content];
		tile.coord = { it.coord.x, it.coord.y };
		tile.size = { it.size.w, it.size.h };

		auto& image = tile.image;
		auto& srcBuffer = image.pixels;
		auto srcChannels = image.channels;

		for ( size_t y = 0; y < tile.size.y; ++y ) {
		for ( size_t x = 0; x < tile.size.x; ++x ) {
			size_t src = (y *  tile.size.x * srcChannels) + (x * srcChannels);
			size_t dst = ((y + tile.coord.y) * size.x * channels) + ((x + tile.coord.x) * channels);
			for ( size_t i = 0; i < srcChannels; ++i ) {
				dstBuffer[dst+i] = srcBuffer[src+i];
			}
		}
		}
	}
}
void uf::atlas::generate( pod::Atlas& atlas, const uf::stl::vector<pod::Image>& images, float padding  ) {
	for ( auto& image : images ) uf::atlas::add( atlas, image );
	uf::atlas::generate( atlas, padding );
}
void uf::atlas::clear( pod::Atlas& atlas, bool full ) {
	if ( !full ) {
		for ( auto& [ hash, tile ] : atlas.tiles ) tile.image.pixels.clear();
		return;
	}
	atlas.tiles.clear();
	atlas.image.pixels.clear();
}
bool uf::atlas::has( const pod::Atlas& atlas, const pod::Atlas::hash_t& hash ) {
	return atlas.tiles.count( hash ) > 0;
}

pod::Vector2f uf::atlas::mapUv( const pod::Atlas& atlas, const pod::Vector2f& uv, const pod::Atlas::hash_t& hash ) {
	auto it = atlas.tiles.find(hash);
	if ( it != atlas.tiles.end() ) {
		auto& tile = it->second;
		auto& size = atlas.image.size;
		pod::Vector2ui coord = {
			uv.x * tile.size.x + tile.coord.x,
			uv.y * tile.size.y + tile.coord.y
		};
		return pod::Vector2f{ (float) coord.x / (float) size.x, (float) coord.y / (float) size.y };
	}
	return uv;
}
pod::Image& uf::atlas::get( pod::Atlas& atlas ) {
	return atlas.image;
}
const pod::Image& uf::atlas::get( const pod::Atlas& atlas ) {
	return atlas.image;
}

pod::Atlas::hash_t uf::Atlas::addImage( const pod::Image& image, const pod::Atlas::hash_t& hash ) {
	return uf::atlas::add( *this, image, hash );
}
pod::Atlas::hash_t uf::Atlas::addImage( const pod::Image& image ) {
	return uf::atlas::add( *this, image, uf::image::hash( image ) );
}
void uf::Atlas::generate( const uf::Atlas::images_t& images, float padding ) {
	uf::atlas::generate( *this, images, padding );
}
void uf::Atlas::generate( float padding ) {
	uf::atlas::generate( *this, padding );
}
bool uf::Atlas::has( const pod::Atlas::hash_t& hash ) const {
	return uf::atlas::has( *this, hash );
}
bool uf::Atlas::generated() const {
	return !this->image.pixels.empty();
}
void uf::Atlas::clear( bool full ) {
	uf::atlas::clear( *this, full );
}
pod::Vector2f uf::Atlas::mapUv( const pod::Vector2f& uv, const pod::Atlas::hash_t& hash ) const {
	return uf::atlas::mapUv( *this, uv, hash );
}
pod::Image& uf::Atlas::getAtlas() {
	return uf::atlas::get( *this );
}
const pod::Image& uf::Atlas::getAtlas() const {
	return uf::atlas::get( *this );
}
uf::Atlas::atlas_t& uf::Atlas::getImages() {
	return this->tiles;
}
const uf::Atlas::atlas_t& uf::Atlas::getImages() const {
	return this->tiles;
}