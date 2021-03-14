#include <uf/utils/image/atlas.h>
#include <binpack2d/binpack2d.hpp>
#include <iostream>

uf::Atlas::hash_t uf::Atlas::addImage( const uf::Image& _image, bool regenerate ) {
	size_t index = this->m_tiles.size();
	auto hash = _image.getHash();
	if ( this->m_tiles.count( hash ) > 0 ) return hash;

	auto& tile = this->m_tiles[hash];
	tile.image = _image;
	tile.identifier.hash = hash;
	tile.identifier.index = index;
	if ( regenerate ) this->generate();
	return hash;
}
uf::Atlas::hash_t uf::Atlas::addImage( uf::Image&& _image, bool regenerate ) {
	size_t index = this->m_tiles.size();
	auto hash = _image.getHash();
	if ( this->m_tiles.count( hash ) > 0 ) return hash;

	auto& tile = this->m_tiles[hash];
	tile.image = _image;
	tile.identifier.hash = hash;
	tile.identifier.index = index;
	if ( regenerate ) this->generate();
	return hash;
}
uf::Atlas::hash_t uf::Atlas::addImage( const uint8_t* pointer, const pod::Vector2ui& size, std::size_t bpp, std::size_t channels, bool flip, bool regenerate ) {
	uf::Image image;
	image.loadFromBuffer( pointer, size, bpp, channels, flip );
	return this->addImage( std::move( image ), regenerate );
}
void uf::Atlas::generate( const uf::Atlas::images_t& images, float padding ) {
	for ( auto& image : images ) this->addImage( image, false );
	generate( padding );
}
void uf::Atlas::generate( float padding ) {
	if ( this->m_tiles.empty() ) return;

	BinPack2D::CanvasArray<Identifier> internalAtlas;
	BinPack2D::ContentAccumulator<uf::Atlas::Identifier> queue, stored, remainder;
	pod::Vector2ui size = {0,0};
	pod::Vector3ui largest = {0,0,0};
	size_t index = 0;
	size_t area = 0;
	size_t channels = 1;
	for ( auto pair : this->m_tiles ) {
		auto& tile = pair.second;
		auto& dim = tile.image.getDimensions();
		channels = std::max( channels, tile.image.getChannels() );
		queue += BinPack2D::Content<uf::Atlas::Identifier>(tile.identifier, BinPack2D::Coord(), BinPack2D::Size(dim.x, dim.y), false );
		size += dim;
		area += dim.x * dim.y;
		if ( area >= largest.z ) {
			largest.x = dim.x;
			largest.y = dim.y;
		}
	}
	size_t tries = 16;
	do {
		{
		//	size_t area = largest.x * largest.y * this->m_tiles.size();
			size_t side = std::sqrt( area ) * padding;
			size = { side, side };
			{
				size.x--;
				size.x |= size.x >> 1;
				size.x |= size.x >> 2;
				size.x |= size.x >> 4;
				size.x |= size.x >> 8;
				size.x |= size.x >> 16;
				size.x++;
				size.y--;
				size.y |= size.y >> 1;
				size.y |= size.y >> 2;
				size.y |= size.y >> 4;
				size.y |= size.y >> 8;
				size.y |= size.y >> 16;
				size.y++;
			}
		}
		queue.Sort();
		internalAtlas = BinPack2D::UniformCanvasArrayBuilder<uf::Atlas::Identifier>(size.x, size.y, 1).Build();
		bool success = internalAtlas.Place( queue, remainder );
		if ( success && remainder.Get().empty() ) break;
		// increase padding
		padding += 0.10f;
	} while ( --tries );
	internalAtlas.CollectContent( stored );

	this->m_atlas.loadFromBuffer( NULL, size, 8, channels );
	auto& dstBuffer = this->m_atlas.getPixels();
	for ( size_t i = 0; i < size.x * size.y * channels; ++i ) dstBuffer[i] = 0;
	for ( auto& it : stored.Get() ) {
		auto& tile = this->m_tiles[it.content.hash];
		tile.coord = { it.coord.x, it.coord.y };
		tile.size = { it.size.w, it.size.h };
/*
	}
	for ( auto pair : this->m_tiles ) {
		auto& tile = pair.second;
*/
		auto& image = tile.image;
		auto& srcBuffer = image.getPixels();
		auto srcChannels = image.getChannels();

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
bool uf::Atlas::generated() const {
	return !this->m_atlas.getPixels().empty();
}
void uf::Atlas::clear( bool full ) {
	if ( !full ) {
		for ( auto pair : this->m_tiles ) pair.second.image.clear();
		return;
	}
	this->m_tiles.clear();
	this->m_atlas.clear();
}
pod::Vector2f uf::Atlas::mapUv( const pod::Vector2f& uv, const uf::Atlas::hash_t& hash ) {
	auto& size = this->m_atlas.getDimensions();
	for ( auto pair : this->m_tiles ) {
		auto& tile = pair.second;
		if ( tile.identifier.hash != hash ) continue;
		pod::Vector2ui coord = { uv.x * tile.size.x + tile.coord.x, uv.y * tile.size.y + tile.coord.y };
		return pod::Vector2f{ (float) coord.x / (float) size.x, (float) coord.y / (float) size.y };
	}
	return uv;
}
pod::Vector2f uf::Atlas::mapUv( const pod::Vector2f& uv, size_t index ) {
	auto& size = this->m_atlas.getDimensions();
	for ( auto pair : this->m_tiles ) {
		auto& tile = pair.second;
		if ( tile.identifier.index != index ) continue;
		pod::Vector2ui coord = { uv.x * tile.size.x + tile.coord.x, uv.y * tile.size.y + tile.coord.y };
		return pod::Vector2f{ (float) coord.x / (float) size.x, (float) coord.y / (float) size.y };
	}
	return uv;
}
pod::Vector3f uf::Atlas::mapUv( const pod::Vector3f& uv ) {
	pod::Vector2f nuv = mapUv( { uv.x, uv.y }, uv.z );
	return { nuv.x, nuv.y, uv.z };
}
uf::Image& uf::Atlas::getAtlas() {
	return this->m_atlas;
}
const uf::Image& uf::Atlas::getAtlas() const {
	return this->m_atlas;
}
uf::Atlas::atlas_t& uf::Atlas::getImages() {
	return this->m_tiles;
}
const uf::Atlas::atlas_t& uf::Atlas::getImages() const {
	return this->m_tiles;
}