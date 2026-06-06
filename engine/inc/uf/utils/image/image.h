#pragma once

#include <uf/config.h>
#include <uf/utils/math/vector.h>
#include <uf/utils/memory/vector.h>

namespace pod {
	struct UF_API Image {
		typedef pod::Vector4ub pixel_t;
		typedef uf::stl::vector<pixel_t::type_t> container_t;

		uf::stl::string filename;
		pod::Image::container_t pixels;
		pod::Vector2ui size = {};
		size_t bpp = 8 * 4;
		size_t channels = 4;
		size_t format = 0;
	};
}

namespace uf {
	namespace image {
		bool UF_API open( pod::Image&, const uf::stl::string&, bool = true );
		void UF_API clear( pod::Image& );

		void UF_API load( pod::Image&, const pod::Image::pixel_t::type_t* pointer, const pod::Vector2ui& size, size_t bpp, size_t channels, bool flip = false );
		void UF_API load( pod::Image&, const pod::Image::container_t& container, const pod::Vector2ui& size, size_t bpp, size_t channels, bool flip = false );
		
		bool UF_API save( const pod::Image&, const uf::stl::string& filename, bool flip = false );
		void UF_API save( const pod::Image&, std::ostream& stream );

		pod::Image::pixel_t UF_API at( pod::Image&, const pod::Vector2ui& at );
		uf::stl::string UF_API hash( const pod::Image& );

		void UF_API flip( pod::Image& );
		void UF_API padToPowerOfTwo( pod::Image& );
		void UF_API convert( pod::Image&, const uf::stl::string&, const uf::stl::string& = "rgba" );

		pod::Image UF_API overlay( const pod::Image&, const pod::Image& top, const pod::Vector2ui& corner = {} );
		pod::Image UF_API replace( const pod::Image&, const pod::Image::pixel_t& from, const pod::Image::pixel_t& to );
		pod::Image UF_API subImage( const pod::Image&, const pod::Vector2ui& start, const pod::Vector2ui& end );
		pod::Image UF_API scale( const pod::Image&, const pod::Vector2ui& size, const uf::stl::string& = "nearest" );
	}
}

namespace uf {
	class UF_API Image : public pod::Image {
	public:
	// 	C-tor
		Image() = default;
		Image( const pod::Image& image ) : pod::Image(image) {}

		Image( const Image& ) = default;
		Image& operator=( const Image& ) = default;

		Image( Image&& ) noexcept = default;
		Image& operator=( Image&& ) noexcept = default;

		bool open( const uf::stl::string& filename, bool = true ); 								// from file
		void open( const std::istream& stream ); 												// from stream
		void move( Image::container_t&& move,  const pod::Vector2ui& size );					// move from vector of pixels
		void move( uf::Image&& );																// move from image object
		void copy( const Image::container_t& copy,  const pod::Vector2ui& size );				// copy from vector of pixels
		void copy( const uf::Image& );															// copy from image object
	// 	D-tor
		void clear(); 																			// empties pixel container
	// 	Getters
		void loadFromBuffer( const Image::pixel_t::type_t* pointer, const pod::Vector2ui& size, size_t bpp, size_t channels, bool flip = false );
		void loadFromBuffer( const Image::container_t& container, const pod::Vector2ui& size, size_t bpp, size_t channels, bool flip = false );
		
		uf::stl::string getFilename() const;
		void setFilename( const uf::stl::string& );

		Image::container_t& getPixels();
		const Image::container_t& getPixels() const;

		Image::pixel_t::type_t* getPixelsPtr();
		const Image::pixel_t::type_t* getPixelsPtr() const;

		pod::Vector2ui& getDimensions();
		const pod::Vector2ui& getDimensions() const;

		size_t& getBpp();
		size_t getBpp() const;

		size_t& getChannels();
		size_t getChannels() const;

		uf::stl::string getHash() const;
		size_t getFormat() const;

		Image::pixel_t at( const pod::Vector2ui& at );

	// 	Modifiers
		void flip();
		void padToPowerOfTwo();
		bool save( const uf::stl::string& filename, bool flip = false ) const; 			// to file
		void save( std::ostream& stream ) const; 										// to stream
		void convert( const uf::stl::string&, const uf::stl::string& = "rgba" );
		Image overlay(const Image& top, const pod::Vector2ui& corner = {} ) const; 		// Merges one image on top of another
		Image replace(const Image::pixel_t& from, const Image::pixel_t& to ) const; 	// Changes all pixel from one color (from), to another (to)
		Image subImage( const pod::Vector2ui& start, const pod::Vector2ui& end) const; 	// Crops an image
		Image scale( const pod::Vector2ui& size, const uf::stl::string& = "nearest" ) const;
	};
}