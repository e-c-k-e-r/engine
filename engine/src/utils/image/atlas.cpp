#include <uf/utils/image/atlas.h>
#include <uf/utils/memory/memcpy.h>
#include <bit>

#define STB_RECT_PACK_IMPLEMENTATION
#include <stb/stb_rect_pack.h>
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

void uf::atlas::generate( pod::Atlas& atlas, size_t padding ) {
	if ( atlas.tiles.empty() ) return;

	uf::stl::vector<stbrp_rect> rects;
	uf::stl::vector<pod::Atlas::hash_t> hashes;
	rects.reserve(atlas.tiles.size());
	hashes.reserve(atlas.tiles.size());

	size_t area = 0;
	size_t channels = 1;

	for ( auto& [ hash, tile ] : atlas.tiles ) {
		auto& dim = tile.image.size;
		channels = std::max( channels, tile.image.channels );

		stbrp_rect rect;
		rect.id = static_cast<int>(rects.size());
		rect.w = dim.x + padding * 2;
		rect.h = dim.y + padding * 2;
		rects.push_back(rect);
		hashes.push_back(hash);

		area += dim.x * dim.y;
	}

	size_t side = std::sqrt( area );
	pod::Vector2ui size = { std::bit_ceil(side), std::bit_ceil(side) };

	bool all_packed = false;
	uf::stl::vector<stbrp_node> nodes;

	while ( !all_packed ) {
		nodes.resize(size.x);
		stbrp_context context;
		stbrp_init_target(&context, size.x, size.y, nodes.data(), nodes.size());

		all_packed = stbrp_pack_rects(&context, rects.data(), rects.size());

		if ( !all_packed ) {
			size.x *= 2;
			size.y *= 2;
		}
	}

	uf::image::load( atlas.image, NULL, size, 8, channels );
	auto& dstBuffer = atlas.image.pixels;

	memset(dstBuffer.data(), 0, size.x * size.y * channels * sizeof(decltype(dstBuffer[0])));

	for ( size_t i = 0; i < rects.size(); ++i ) {
		const auto& rect = rects[i];
		auto hash = hashes[rect.id];
		auto& tile = atlas.tiles[hash];

		tile.coord = { rect.x + padding	, rect.y + padding	 };
		tile.size  = { rect.w - padding * 2, rect.h - padding * 2 };

		auto& image = tile.image;
		auto& srcBuffer = image.pixels;
		auto srcChannels = image.channels;

		size_t rowSizeSrc = tile.size.x * srcChannels;
		size_t rowSizeDst = tile.size.x * channels;

		for ( size_t y = 0; y < tile.size.y; ++y ) {
			size_t srcIndex = y * tile.size.x * srcChannels;
			size_t dstIndex = ((y + tile.coord.y) * size.x * channels) + (tile.coord.x * channels);

			if ( srcChannels == channels ) {
				uf::stl::memcpy(&dstBuffer[dstIndex], &srcBuffer[srcIndex], rowSizeSrc * sizeof(decltype(dstBuffer[0])));
			} else {
				for ( size_t x = 0; x < tile.size.x; ++x ) {
					for ( size_t c = 0; c < srcChannels; ++c ) {
						dstBuffer[dstIndex + (x * channels) + c] = srcBuffer[srcIndex + (x * srcChannels) + c];
					}
				}
			}

			if ( padding > 0 ) {
				// top and bottom
				for ( size_t py = 1; py <= padding; ++py ) {
					size_t topDstY = tile.coord.y - py;
					size_t topSrcY = tile.coord.y;
					size_t botDstY = tile.coord.y + tile.size.y - 1 + py;
					size_t botSrcY = tile.coord.y + tile.size.y - 1;

					size_t rowBytes = tile.size.x * channels * sizeof(decltype(dstBuffer[0]));
					uf::stl::memcpy( &dstBuffer[topDstY * size.x * channels + tile.coord.x * channels], &dstBuffer[topSrcY * size.x * channels + tile.coord.x * channels], rowBytes );
					uf::stl::memcpy( &dstBuffer[botDstY * size.x * channels + tile.coord.x * channels], &dstBuffer[botSrcY * size.x * channels + tile.coord.x * channels], rowBytes );
				}

				// left and right
				size_t yStart = tile.coord.y - padding;
				size_t yCount = tile.size.y + padding * 2;
				for ( size_t y = 0; y < yCount; ++y ) {
					size_t currentY = yStart + y;
					size_t rowStart = currentY * size.x * channels;

					for ( size_t px = 1; px <= padding; ++px ) {
						size_t leftDstX = tile.coord.x - px;
						size_t leftSrcX = tile.coord.x;
						size_t rightDstX = tile.coord.x + tile.size.x - 1 + px;
						size_t rightSrcX = tile.coord.x + tile.size.x - 1;

						for ( size_t c = 0; c < channels; ++c ) {
							dstBuffer[rowStart + leftDstX * channels + c]  = dstBuffer[rowStart + leftSrcX * channels + c];
							dstBuffer[rowStart + rightDstX * channels + c] = dstBuffer[rowStart + rightSrcX * channels + c];
						}
					}
				}
			}
		}
	}
}
void uf::atlas::generate( pod::Atlas& atlas, const uf::stl::vector<pod::Image>& images, size_t padding  ) {
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

		pod::Vector2f coord = {
			uv.x * (float) tile.size.x + (float) tile.coord.x,
			uv.y * (float) tile.size.y + (float) tile.coord.y
		};
		return pod::Vector2f{ coord.x / (float)size.x, coord.y / (float)size.y };
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