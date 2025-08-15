#pragma once

#include <uf/config.h>
#include <uf/utils/math/vector.h>
#include <uf/utils/memory/vector.h>

#include "pixel.h"

namespace uf {
	class UF_API Image {
	public:
		typedef pod::Vector2ui vec2_t;
		typedef pod::Vector<uint8_t, 4> pixel_t;
		typedef uf::stl::vector<pixel_t::type_t> container_t;
	protected:
		uf::stl::string m_filename;
		Image::container_t m_pixels;
		Image::vec2_t m_dimensions;
		std::size_t m_bpp;
		std::size_t m_channels;
		std::size_t m_format;
	public:
	// 	C-tor
		bool open( const uf::stl::string& filename, bool = true ); 									// from file
		void open( const std::istream& stream ); 												// from stream
		void move( Image::container_t&& move,  const Image::vec2_t& size );						// move from vector of pixels
		void copy( const Image::container_t& copy,  const Image::vec2_t& size );				// copy from vector of pixels
		void copy( const uf::Image& );															// copy from image object
	// 	D-tor
		void clear(); 																			// empties pixel container
	// 	Getters
		void loadFromBuffer( const Image::pixel_t::type_t* pointer, const pod::Vector2ui& size, std::size_t bpp, std::size_t channels, bool flip = false );
		void loadFromBuffer( const Image::container_t& container, const pod::Vector2ui& size, std::size_t bpp, std::size_t channels, bool flip = false );
		
		uf::stl::string getFilename() const;
		void setFilename( const uf::stl::string& );

		Image::container_t& getPixels();
		const Image::container_t& getPixels() const;

		Image::pixel_t::type_t* getPixelsPtr();
		const Image::pixel_t::type_t* getPixelsPtr() const;

		Image::vec2_t& getDimensions();
		const Image::vec2_t& getDimensions() const;

		std::size_t& getBpp();
		std::size_t getBpp() const;

		std::size_t& getChannels();
		std::size_t getChannels() const;

		uf::stl::string getHash() const;
		size_t getFormat() const;

		Image::pixel_t at( const Image::vec2_t& at );
	// 	Modifiers
		void flip();
		void padToPowerOfTwo();
		bool save( const uf::stl::string& filename, bool flip = false ) const; 				// to file
		void save( std::ostream& stream ) const; 										// to stream
		void convert( const uf::stl::string&, const uf::stl::string& = "rgba" );
		Image overlay(const Image& top, const Image::vec2_t& corner = {} ) const; 		// Merges one image on top of another
		Image replace(const Image::pixel_t& from, const Image::pixel_t& to ) const; 	// Changes all pixel from one color (from), to another (to)
		Image subImage( const Image::vec2_t& start, const Image::vec2_t& end) const; 	// Crops an image
	//	Operators
		uf::Image& operator=(const uf::Image&);
	};
}