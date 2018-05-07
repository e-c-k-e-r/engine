#pragma once

#include <uf/config.h>
#include <uf/ext/ext.h>
#include <uf/engine/entity/entity.h>

#include <uf/gl/shader/shader.h>
#include <uf/gl/mesh/mesh.h>
#include <uf/gl/texture/texture.h>
#include <uf/gl/camera/camera.h>

#include <uf/utils/math/transform.h>
#include <uf/utils/math/physics.h>

#include "../object/object.h"
#include "region.h"

namespace ext {
	class EXT_API Terrain : public ext::Object {
	protected:
	public:
		void initialize();
		void render();
		void tick();

		bool exists( const pod::Vector3i& ) const;
		bool inBounds( const pod::Vector3i& ) const;
		ext::Region* at( const pod::Vector3i& ) const;

		void generate();
		void generate( const pod::Vector3i& );
		void degenerate( const pod::Vector3i& );
	};
}