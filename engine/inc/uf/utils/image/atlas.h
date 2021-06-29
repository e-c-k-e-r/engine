#pragma once

#include <uf/utils/image/image.h>
#include <uf/utils/memory/unordered_map.h>

namespace uf {
	class UF_API Atlas {
	public:
		typedef uf::stl::string hash_t;
		struct Identifier {
			size_t index;
			hash_t hash;
		};
		struct Tile {
			uf::Image image;
			Identifier identifier = {0,""};
			pod::Vector2ui size = {};
			pod::Vector2ui coord = {};
		};
		typedef uf::stl::vector<uf::Image> images_t;
		typedef uf::stl::unordered_map<hash_t, Tile> atlas_t;
	protected:
		uf::Image m_atlas;
		atlas_t m_tiles;
	public:
		hash_t addImage( const uf::Image&, bool = false );
		hash_t addImage( uf::Image&&, bool = false );
		hash_t addImage( const uint8_t*, const pod::Vector2ui&, std::size_t, std::size_t, bool = false, bool = false );
		
		void generate(float padding = 1);
		void generate( const uf::stl::vector<uf::Image>&, float padding = 1);
		void clear(bool = true);
		bool generated() const;
		
		pod::Vector2f mapUv( const pod::Vector2f&, const hash_t& );
		pod::Vector2f mapUv( const pod::Vector2f&, size_t );
		pod::Vector3f mapUv( const pod::Vector3f& );
		
		uf::Image& getAtlas();
		const uf::Image& getAtlas() const;
		
	 	atlas_t& getImages();
		const atlas_t& getImages() const;
	};
#if 0
	class UF_API HashAtlas {
	public:
		typedef uf::stl::string hash_t;
		typedef uf::stl::unordered_map<hash_t, uf::Image> images_t;
		
		struct Identifier {
			hash_t hash;
		};
		typedef Identifier identifier_t;
		typedef BinPack2D::CanvasArray<identifier_t> atlas_t;
	protected:
		uf::Image m_image;
		images_t m_images;
		atlas_t m_atlas;
	public:
		hash_t addImage( const uf::Image&, bool = false );
		hash_t addImage( uf::Image&&, bool = false );
		hash_t addImage( const uint8_t*, const pod::Vector2ui&, std::size_t, std::size_t, bool = false, bool = false );
		
		void generate(float padding = 1);
		void generate( const images_t&, float padding = 1);
		void clear();
		bool has( const uf::Image& ) const;
		bool has( const uf::stl::string& ) const;
		
		pod::Vector2f mapUv( const pod::Vector2f&, const hash_t& );
		
		uf::Image& getAtlas();
	 	images_t& getImages();
		const images_t& getImages() const;
	};
#endif
}