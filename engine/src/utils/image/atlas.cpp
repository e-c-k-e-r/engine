#include <uf/utils/image/atlas.h>
#include <iostream>

void uf::Atlas::addImage( const uf::Image& _image, bool regenerate ) {
	auto& image = this->m_images.emplace_back( _image );
	if ( regenerate ) this->generate();
}
void uf::Atlas::addImage( uf::Image&& _image, bool regenerate ) {
	auto& image = this->m_images.emplace_back( _image );
	if ( regenerate ) this->generate();
}
void uf::Atlas::addImage( const uint8_t* pointer, const pod::Vector2ui& size, std::size_t bpp, std::size_t channels, bool flip, bool regenerate ) {
	auto& image = this->m_images.emplace_back();
	image.loadFromBuffer( pointer, size, bpp, channels, flip );
	if ( regenerate ) this->generate();

}
void uf::Atlas::generate() {
	if ( this->m_images.empty() ) return;
	// destroy atlas
	this->m_image.clear();

	BinPack2D::ContentAccumulator<uf::Atlas::identifier_t> queue;
	pod::Vector2ui size = {0,0};
	size_t index = 0;
	size_t area = 0;
	for ( auto& image : this->m_images ) {
		auto& dim = image.getDimensions();
		size += dim;
		area += dim.x * dim.y;
		queue += BinPack2D::Content<uf::Atlas::identifier_t>({index++}, BinPack2D::Coord(), BinPack2D::Size(dim.x, dim.y), false );
	}
	{
		size_t side = std::sqrt( area );
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
		this->m_image.loadFromBuffer( NULL, size, 8, 4 );
	}
	queue.Sort();
	this->m_atlas = BinPack2D::UniformCanvasArrayBuilder<uf::Atlas::identifier_t>(size.x, size.y ,1).Build();
	BinPack2D::ContentAccumulator<uf::Atlas::identifier_t> stored, remainder;
	bool success = this->m_atlas.Place( queue, remainder );
	this->m_atlas.CollectContent( stored );

	auto& dstBuffer = this->m_image.getPixels();

	for ( size_t i = 0; i < size.x * size.y * 4; i+=4 ) {
		dstBuffer[i+0] = 0;
		dstBuffer[i+1] = 0;
		dstBuffer[i+2] = 0;
		dstBuffer[i+3] = 0;
	}

	for ( auto& it : stored.Get() ) {
		size_t index = it.content.index;
		auto& image = this->m_images[index];
		auto& dim = image.getDimensions();
		auto channels = image.getChannels();
		auto& srcBuffer = image.getPixels();

		for ( size_t y = 0; y < dim.y; ++y ) {
		for ( size_t x = 0; x < dim.x; ++x ) {
			size_t src = (y *  dim.x * channels) + (x * channels);
			size_t dst = ((y + it.coord.y) * size.x * 4) + ((x + it.coord.x) * 4);
			for ( size_t i = 0; i < channels; ++i ) {
				dstBuffer[dst+i] = srcBuffer[src+i];
			}
		}
		}
	}
}
void uf::Atlas::clear() {
	this->m_images.clear();
	this->m_image.clear();
	this->m_atlas = uf::Atlas::atlas_t{};
}
pod::Vector2f uf::Atlas::mapUv( const pod::Vector2f& uv, size_t index ) {
	BinPack2D::ContentAccumulator<uf::Atlas::identifier_t> stored;
	this->m_atlas.CollectContent( stored );
	auto& size = this->m_image.getDimensions();
	for ( auto& it : stored.Get() ) {
		if ( it.content.index != index ) continue;
		auto& image = this->m_images[index];
		auto& dim = image.getDimensions();
		pod::Vector2f nuv = uv;
		if ( nuv.x > 1.0f ) nuv.x = std::fmod( nuv.x, 1.0f );
		if ( nuv.y > 1.0f ) nuv.y = std::fmod( nuv.y, 1.0f );
	//	while ( nuv.x > 1.0f ) nuv.x -= 1.0f;
	//	while ( nuv.y > 1.0f ) nuv.y -= 1.0f;

		pod::Vector2ui coord = { uv.x * dim.x + it.coord.x, uv.y * dim.y + it.coord.y };
		nuv = { (float) coord.x / (float) size.x, (float) coord.y / (float) size.y };
		nuv.y = 1.0f - nuv.y;
		return nuv;
	}
	return uv;
}
pod::Vector3f uf::Atlas::mapUv( const pod::Vector3f& uv ) {
	pod::Vector2f nuv = mapUv( { uv.x, uv.y }, uv.z );
	return { nuv.x, nuv.y, uv.z };
}
uf::Image& uf::Atlas::getAtlas() {
	return this->m_image;
}
uf::Atlas::images_t& uf::Atlas::getImages() {
	return this->m_images;
}
const uf::Atlas::images_t& uf::Atlas::getImages() const {
	return this->m_images;
}