#pragma once

#include <uf/config.h>
#include <uf/ext/ext.h>
#include <uf/engine/entity/entity.h>

#include <uf/gl/shader/shader.h>
#include <uf/gl/mesh/mesh.h>
#include <uf/gl/mesh/skeletal/mesh.h>
#include <uf/gl/texture/texture.h>
#include <uf/gl/camera/camera.h>

#include <uf/utils/math/transform.h>
#include <uf/utils/math/physics.h>
#include <uf/utils/math/collision.h>
#include <uf/utils/thread/thread.h>

namespace ext {
	class EXT_API Object : public uf::Entity {
	
	public:
		void initialize();
		void tick();
		void render();

		bool load( const std::string& );
		void load( const uf::Serializer& );
	};
}