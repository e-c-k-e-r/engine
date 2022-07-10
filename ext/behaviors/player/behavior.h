#pragma once

#include <uf/config.h>
#include <uf/ext/ext.h>
#include <uf/engine/entity/entity.h>
#include <uf/engine/scene/scene.h>
#include <uf/utils/math/vector.h>

namespace ext {
	namespace PlayerBehavior {
		UF_BEHAVIOR_DEFINE_TYPE();
		EXT_BEHAVIOR_DEFINE_TRAITS();
		EXT_BEHAVIOR_DEFINE_FUNCTIONS();
		UF_BEHAVIOR_DEFINE_METADATA(
			struct {
				bool control = true;
				uf::stl::string menu = "";
				bool crouching = false;
				bool noclipped = false;
			} system;
			struct {
				float crouch = -1.0f;
				float rotate = 1.0f;
				float move = 1.0f;
				float run = 1.0f;
				float walk = 1.0f;
				float friction = 0.8f;
				float air = 1.0f;
				pod::Vector3f jump = {0,8,0};
				struct {
					pod::Vector3f feet = {0,-1.5,0};
					pod::Vector3f floor = {0,-1,0};
					bool print = false;
				} floored;
			} movement;
			struct {
				struct {
					pod::Vector3f current = {NAN, NAN, NAN};
					pod::Vector3f min = {NAN, NAN, NAN};
					pod::Vector3f max = {NAN, NAN, NAN};
				} limit;
				pod::Vector3t<bool> invert;
				pod::Vector2f queued;
			} camera;
			struct {
				pod::Vector2f sensitivity = {1,1};
				pod::Vector2f smoothing = {10,10};
			} mouse;
			struct {
				struct {
					uf::stl::vector<uf::stl::string> list;
					float volume;
				} footstep;
			} audio;
		);
	}
}