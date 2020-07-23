#pragma once

#include <uf/config.h>
#include <uf/ext/ext.h>
#include <uf/engine/entity/entity.h>

#include <uf/utils/math/transform.h>
#include <uf/utils/math/physics.h>

#include <uf/engine/object/object.h>

#include "region.h"
#include "maze.h"

namespace ext {
	class EXT_API Terrain : public uf::Object {
	protected:
	public:
		virtual void initialize();
		virtual void render();
		virtual void destroy();
		virtual void tick();

		void relocatePlayer();
		
		bool exists( const pod::Vector3i& ) const;
		bool inBounds( const pod::Vector3i& ) const;
		ext::Region* at( const pod::Vector3i& ) const;

		void generate();
		void generate( const pod::Vector3i& );
		void degenerate( const pod::Vector3i& );
	};
}

namespace std {
	template<> struct hash<pod::Vector3i> {
	    size_t operator()(pod::Vector3i const& vertex) const {
	       std::size_t seed = 3;
	       std::hash<int> hasher;
			for ( size_t i = 0; i < 3; ++i ) seed ^= hasher( vertex[i] ) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			return seed;
	    }
	};
}