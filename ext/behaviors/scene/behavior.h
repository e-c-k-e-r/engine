#pragma once

#include <uf/config.h>
#include <uf/ext/ext.h>
#include <uf/engine/entity/entity.h>
#include <uf/engine/scene/scene.h>
#include <uf/utils/math/vector.h>

namespace ext {
	namespace ExtSceneBehavior {
		UF_BEHAVIOR_DEFINE_TYPE;
		void attach( uf::Object& );
		void initialize( uf::Object& );
		void tick( uf::Object& );
		void render( uf::Object& );
		void destroy( uf::Object& );

		struct Metadata {
			struct {
				size_t textures = 256;
				size_t lights = 256;
			} max;
			struct {
				bool enabled = true;
				int shadowSamples = 4;
				int shadowThreshold = 8;
				size_t updateThreshold = 4;
				pod::Vector4f ambient = {0,0,0,1};
				pod::Vector4f specular = {1,1,1,1};
			} light;
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

			std::function<void()> serialize;
			std::function<void()> deserialize;
		};

		void bindBuffers( uf::Object&, const std::string& = "", bool = false );
	}
}