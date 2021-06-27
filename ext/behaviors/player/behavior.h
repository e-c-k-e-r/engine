#pragma once

#include <uf/config.h>
#include <uf/ext/ext.h>
#include <uf/engine/entity/entity.h>
#include <uf/engine/scene/scene.h>
#include <uf/utils/math/vector.h>

namespace ext {
	namespace PlayerBehavior {
		UF_BEHAVIOR_DEFINE_TYPE;
		void attach( uf::Object& );
		void initialize( uf::Object& );
		void tick( uf::Object& );
		void render( uf::Object& );
		void destroy( uf::Object& );
		struct Metadata {
			struct {
				struct {
					bool collision = true;
					bool impulse = true;
					float crouch = -1.0f;
					float rotate = 1.0f;
					float move = 1.0f;
					float run = 1.0f;
					float walk = 1.0f;
					pod::Vector3f jump = {0,8,0};
				} physics;

				bool control = true;
				std::string menu = "";
				bool crouching = false;
				bool noclipped = false;
			} system;
			struct {
				struct {
					pod::Vector3f current = {NAN, NAN, NAN};
					pod::Vector3f min = {NAN, NAN, NAN};
					pod::Vector3f max = {NAN, NAN, NAN};
				} limit;
				pod::Vector3t<bool> invert;
			} camera;
			struct {
				struct {
					std::vector<std::string> list;
					float volume;
				} footstep;
			} audio;

			std::function<void()> serialize;
			std::function<void()> deserialize;
		};
	}
}