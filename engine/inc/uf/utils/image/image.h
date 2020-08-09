#pragma once

#include <uf/config.h>
#include <uf/utils/math/vector.h>
#include <vector>

#include "pixel.h"

namespace uf {
	class UF_API Image {
	public:
		typedef pod::Vector2ui vec2_t;
		typedef pod::PixelRgba8 pixel_t;
		typedef std::vector<pixel_t::type_t> container_t;
	protected:
		std::string m_filename;
		Image::container_t m_pixels;
		Image::vec2_t m_dimensions;
		std::size_t m_bpp;
		std::size_t m_channels;
	public:
	// 	C-tor
		Image(); 																				// Default
		Image( const Image::vec2_t& size ); 													// Just Size
		Image( Image&& move ); 																	// Move pixels
		Image( const Image& copy ); 															// Copy pixels
		Image( Image::container_t&& move, const Image::vec2_t& size ); 							// Move from vector of pixels
		Image( const Image::container_t& copy, const Image::vec2_t& size ); 					// Copy from vector of pixels

		bool open( const std::string& filename ); 												// from file
		void open( const std::istream& stream ); 												// from stream
		void move( Image::container_t&& move,  const Image::vec2_t& size );						// move from vector of pixels
		void copy( const Image::container_t& copy,  const Image::vec2_t& size );				// copy from vector of pixels
	// 	D-tor
		~Image();
		void clear(); 																			// empties pixel container
	// 	Getters
		void loadFromBuffer( const Image::pixel_t::type_t* pointer, const pod::Vector2ui& size, std::size_t bpp, std::size_t channels );
		void loadFromBuffer( const Image::container_t& container, const pod::Vector2ui& size, std::size_t bpp, std::size_t channels, bool flip = false );
		std::string getFilename() const;
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
		Image::pixel_t at( const Image::vec2_t& at );
	// 	Modifiers
		void padToPowerOfTwo();
		bool save( const std::string& filename, bool flip = false ); 							// to file
		void save( std::ostream& stream ); 														// to stream
		Image overlay(const Image& top, const Image::vec2_t& corner = {} ) const; 	// Merges one image on top of another
		Image replace(const Image::pixel_t& from, const Image::pixel_t& to ) const; 			// Changes all pixel from one color (from), to another (to)
		Image subImage( const Image::vec2_t& start, const Image::vec2_t& end) const; 			// Crops an image
	};
}