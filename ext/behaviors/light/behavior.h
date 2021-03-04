#pragma once

#include <uf/config.h>
#include <uf/ext/ext.h>
#include <uf/engine/entity/entity.h>
#include <uf/engine/scene/scene.h>
#include <uf/utils/math/vector.h>

namespace ext {
	namespace LightBehavior {
		UF_BEHAVIOR_DEFINE_TYPE;
		void attach( uf::Object& );
		void initialize( uf::Object& );
		void tick( uf::Object& );
		void render( uf::Object& );
		void destroy( uf::Object& );
		struct Metadata {
			pod::Vector3f color = {1,1,1};
			float power = 0.0f;
			float bias = 0.0f;
			bool shadows = false;
			size_t type = 0;
			struct {
				std::string mode = "in-range";
				float limiter = 0.0f;
				float timer = 0.0f;
				bool rendered = false;
				bool external = false;
			} renderer;
		};
	}
}