#pragma once

#include <uf/config.h>
#include <uf/ext/ext.h>
#include <uf/engine/entity/entity.h>
#include <uf/engine/scene/scene.h>
#include <uf/utils/math/vector.h>

namespace ext {
	namespace ExtSceneBehavior {
		UF_BEHAVIOR_DEFINE_TYPE();
		EXT_BEHAVIOR_DEFINE_TRAITS();
		EXT_BEHAVIOR_DEFINE_FUNCTIONS();
		UF_BEHAVIOR_DEFINE_METADATA(
			struct {
				uint32_t textures2D = 512;
				uint32_t texturesCube = 128;
				uint32_t textures3D = 128;
				uint32_t lights = 256;
			} max;
			struct {
				bool enabled = true;
				pod::Vector4f ambient = {0,0,0,1};
				pod::Vector4f specular = {1,1,1,1};
				float exposure = 1.0f;
				float gamma = 1.0f;
			} light;
			struct {
				bool enabled = true;
				int samples = 4;
				int max = 8;
				uint32_t update = 4;

				uint32_t experimentalMode = 0;
			} shadow;
			struct {
				uint32_t mode;
				uint32_t scalar;
				pod::Vector4f parameters = {0,0,0,0};
				int8_t time = 3;
				bool invalidated = true;
			} shader;
			struct {
				pod::Vector3f color = {1,1,1};
				float stepScale = 16.0f;
				float absorbtion = 0.85f;
				pod::Vector2f range = {};
				struct {
					pod::Vector4f offset = {};
					float timescale = 0.5f; 
					float threshold = 0.5f; 
					float multiplier = 5.0f;
					float scale = 50.0f;
				} density;
			} fog;
		);

		void bindBuffers( uf::Object&, const uf::stl::string& = "", bool = false );
	}
}