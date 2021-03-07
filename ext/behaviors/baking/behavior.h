#pragma once

#include <uf/config.h>
#include <uf/ext/ext.h>
#include <uf/engine/entity/entity.h>
#include <uf/engine/scene/scene.h>
#include <uf/utils/math/vector.h>

namespace ext {
	namespace BakingBehavior {
		UF_BEHAVIOR_DEFINE_TYPE;
		void attach( uf::Object& );
		void initialize( uf::Object& );
		void tick( uf::Object& );
		void render( uf::Object& );
		void destroy( uf::Object& );
		struct Metadata {
			pod::Vector2ui size = {};
			struct {
				std::string model = "";
				struct{
					std::string model = "";
					std::string map = "./lightmap.png";
				} output;
				std::string renderMode = "Baker";
			} names;
			struct {
				std::string mode = "key";
				std::string value = "";
			} trigger;
			struct {
				bool renderMode = false;
				bool map = false;
			} initialized;
			bool shadows = false;
		};
	}
}