#pragma once

#include <uf/config.h>
#include <uf/ext/ext.h>
#include <uf/engine/entity/entity.h>
#include <uf/engine/scene/scene.h>

namespace ext {
	namespace SceneCollisionBehavior {
		UF_BEHAVIOR_DEFINE_TYPE;
		void attach( uf::Object& );
		void initialize( uf::Object& );
		void tick( uf::Object& );
		void render( uf::Object& );
		void destroy( uf::Object& );
	}
}