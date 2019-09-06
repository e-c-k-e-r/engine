#pragma once

#include <uf/config.h>

#include <vector>

#include <uf/utils/math/vector.h>
#include <uf/ext/freetype/freetype.h>

namespace uf {
	class UF_API Glyph {
	protected:
		pod::Vector2ui m_size = { 0, 0 };
		pod::Vector2i m_bearing = { 0, 0 };
		pod::Vector2i m_advance = { 0, 0 };
		
		pod::Vector2i m_padding = { 0, 0 };
		
		bool m_sdf = false;
		int m_spread = 0;
		uint8_t* m_buffer;
	public:
		~Glyph();

		bool generated();
		uint8_t* generate( const std::string&, unsigned long, uint = 48 );
		uint8_t* generate( ext::freetype::Glyph&, unsigned long, uint = 48 );
		uint8_t* generate( const std::string&, const uf::String&, uint = 48 );
		uint8_t* generate( ext::freetype::Glyph&, const uf::String&, uint = 48 );

		void generateSdf( uint8_t* );
	// 	Get
		const pod::Vector2ui& getSize() const;
		const pod::Vector2i& getBearing() const;
		const pod::Vector2i& getAdvance() const;
		const pod::Vector2i& getPadding() const;
		const uint8_t* getBuffer() const;
		
		bool isSdf() const;
		int getSpread() const;
	//	Set
		void setSize( const pod::Vector2ui& );
		void setBearing( const pod::Vector2i& );
		void setAdvance( const pod::Vector2i& );
		void setPadding( const pod::Vector2i& );

		void useSdf( bool = true );
		void setSpread( int );
	};
}