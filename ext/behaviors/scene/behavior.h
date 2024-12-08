#pragma once

#include <uf/config.h>
#include <uf/ext/ext.h>
#include <uf/engine/entity/entity.h>
#include <uf/engine/scene/scene.h>
#include <uf/utils/math/vector.h>
#include <uf/utils/graphic/graphic.h>

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
			} max;
			struct {
				bool enabled = true;
				uint32_t max = 256;
				pod::Vector4f ambient = {0,0,0,1};
				pod::Vector4f specular = {1,1,1,1};
				float exposure = 1.0f;
				float gamma = 1.0f;
				bool useLightmaps = true;
			} light;
			struct {
				float threshold = 1.0f;
				float smoothness = 1.0f;
				uint32_t size = 1;
				bool outOfDate = true;
			} bloom;
			struct {
				bool enabled = true;
				int samples = 4;
				int max = 8;
				uint32_t update = 4;
				uint32_t typeMap = 1;
			} shadow;
			struct {
				uint32_t mode;
				uint32_t scalar;
				pod::Vector4f parameters = {0,0,0,0};
				int8_t time = 3;
				
				bool invalidated = true;

				uint32_t frameAccumulate = 0;
				uint32_t frameAccumulateLimit = 0;
				bool frameAccumulateReset = false;
				bool init3D = true;
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
			struct {
				struct {
					uf::stl::string filename = "%root%/textures/skybox/%d.png";
				} box;
			} sky;

			struct {
				float scale = 1.0f;
			} framebuffer;
		);

		void bindBuffers( uf::Object&, const uf::stl::string& = "", const uf::stl::string& = "fragment", const uf::stl::string& = "deferred" );
		void bindBuffers( uf::Object&, uf::renderer::Graphic&, const uf::stl::string& = "fragment", const uf::stl::string& = "deferred" );
	}
}