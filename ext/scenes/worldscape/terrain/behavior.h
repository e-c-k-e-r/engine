#pragma once

#include <uf/config.h>
#include <uf/ext/ext.h>
#include <uf/engine/entity/entity.h>
#include <uf/engine/scene/scene.h>

namespace ext {
	namespace TerrainBehavior {
		void attach( uf::Entity& );
		void initialize( uf::Object& );
		void tick( uf::Object& );
		void render( uf::Object& );
		void destroy( uf::Object& );
	}
}