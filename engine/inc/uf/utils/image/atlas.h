#pragma once

#include <uf/utils/image/image.h>
#include <binpack2d/binpack2d.hpp>

namespace uf {
	class UF_API Atlas {
	public:
		struct Identifier {
			size_t index;
		};
		typedef std::vector<uf::Image> images_t;
		typedef Identifier identifier_t;
		typedef BinPack2D::CanvasArray<identifier_t> atlas_t;
	protected:
		uf::Image m_image;
		images_t m_images;
		atlas_t m_atlas;
	public:
		void addImage( const uf::Image&, bool = false );
		void addImage( uf::Image&&, bool = false );
		void addImage( const uint8_t*, const pod::Vector2ui&, std::size_t, std::size_t, bool = false, bool = false );
		
		void generate();
		
		pod::Vector2f mapUv( const pod::Vector2f&, size_t );
		pod::Vector3f mapUv( const pod::Vector3f& );
		
		uf::Image& getAtlas();
	 	images_t& getImages();
		const images_t& getImages() const;
	};
}