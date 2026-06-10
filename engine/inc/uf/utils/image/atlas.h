#pragma once

#include <uf/utils/image/image.h>
#include <uf/utils/memory/unordered_map.h>

namespace pod {
	struct UF_API Atlas {
		typedef uf::stl::string hash_t;
		struct Tile {
			pod::Image image;
			pod::Vector2ui size = {};
			pod::Vector2ui coord = {};
		};
		typedef uf::stl::vector<pod::Image> images_t;
		typedef uf::stl::unordered_map<hash_t, Tile> atlas_t;

		pod::Image image;
		pod::Atlas::atlas_t tiles;
	};
}

namespace uf {
	namespace atlas {
		pod::Atlas::hash_t UF_API add( pod::Atlas& atlas, const pod::Image& image, const pod::Atlas::hash_t& hash );
		pod::Atlas::hash_t UF_API add( pod::Atlas& atlas, const pod::Image& image );

		void UF_API generate( pod::Atlas& atlas, size_t padding = 0 );
		void UF_API generate( pod::Atlas& atlas, const uf::stl::vector<pod::Image>& images, size_t padding = 0 );
		void UF_API clear( pod::Atlas& atlas, bool full = true );
		bool UF_API has( const pod::Atlas& atlas, const pod::Atlas::hash_t& hash );

		pod::Vector2f UF_API mapUv( const pod::Atlas& atlas, const pod::Vector2f& uv, const pod::Atlas::hash_t& hash );
		pod::Image& UF_API get( pod::Atlas& atlas );
		const pod::Image& UF_API get( const pod::Atlas& atlas );
	}
}

namespace uf {
	class UF_API Atlas : public pod::Atlas {
	public:
		Atlas() = default;
		Atlas( const pod::Atlas& atlas ) : pod::Atlas(atlas) {}

		hash_t addImage( const pod::Image&, const hash_t& hash );
		hash_t addImage( const pod::Image& );
		
		void generate(float padding = 1);
		void generate( const uf::stl::vector<pod::Image>&, float padding = 1);
		void clear(bool = true);
		bool generated() const;
		bool has( const hash_t& ) const;
		
		pod::Vector2f mapUv( const pod::Vector2f&, const hash_t& ) const;
		pod::Vector2f mapUv( const pod::Vector2f&, size_t ) const;
		pod::Vector3f mapUv( const pod::Vector3f& ) const;
		
		pod::Image& getAtlas();
		const pod::Image& getAtlas() const;
		
	 	atlas_t& getImages();
		const atlas_t& getImages() const;
	};
#if 0
	class UF_API HashAtlas {
	public:
		typedef uf::stl::string hash_t;
		typedef uf::stl::unordered_map<hash_t, pod::Image> images_t;
		
		struct Identifier {
			hash_t hash;
		};
		typedef Identifier identifier_t;
		typedef BinPack2D::CanvasArray<identifier_t> atlas_t;
	protected:
		pod::Image m_image;
		images_t m_images;
		atlas_t m_atlas;
	public:
		hash_t addImage( const pod::Image&, bool = false );
		hash_t addImage( pod::Image&&, bool = false );
		hash_t addImage( const uint8_t*, const pod::Vector2ui&, std::size_t, std::size_t, bool = false, bool = false );
		
		void generate(float padding = 1);
		void generate( const images_t&, float padding = 1);
		void clear();
		bool has( const pod::Image& ) const;
		bool has( const uf::stl::string& ) const;
		
		pod::Vector2f mapUv( const pod::Vector2f&, const hash_t& );
		
		pod::Image& getAtlas();
	 	images_t& getImages();
		const images_t& getImages() const;
	};
#endif
}