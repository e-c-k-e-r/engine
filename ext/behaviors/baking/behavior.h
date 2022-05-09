#pragma once

#include <uf/config.h>
#include <uf/ext/ext.h>
#include <uf/engine/entity/entity.h>
#include <uf/engine/scene/scene.h>
#include <uf/utils/math/vector.h>

namespace ext {
	namespace BakingBehavior {
		UF_BEHAVIOR_DEFINE_TYPE();
		EXT_BEHAVIOR_DEFINE_TRAITS();
		EXT_BEHAVIOR_DEFINE_FUNCTIONS();
		UF_BEHAVIOR_DEFINE_METADATA(
			pod::Vector2ui size = {};

			uf::stl::string output = "./lightmap.png";
			uf::stl::string renderModeName = "Baker";
			
			struct {
				size_t instance = 0;
				size_t material = 0;
				size_t texture = 0;
				size_t light = 0;

				uf::renderer::Texture3D baked;
			} buffers;
			struct {
				size_t textures2D = 1024;
				size_t texturesCube = 512;
				size_t textures3D = 1;
				size_t lights = 1024;
				size_t shadows = 1024;
				size_t layers = 1;
			} max;
			struct {
				size_t lights = 0;
				size_t shadows = 0;
				size_t update = 0;
			} previous;
			struct {
				uf::stl::string mode = "key";
				uf::stl::string value = "";
			} trigger;
			struct {
				bool renderMode = false;
				bool map = false;
			} initialized;
			bool cull = false;
		);
	}
}