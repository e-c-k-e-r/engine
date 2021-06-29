#pragma once

#include <uf/config.h>
#include <uf/ext/ext.h>
#include <uf/engine/entity/entity.h>
#include <uf/engine/scene/scene.h>
#include <uf/utils/math/vector.h>

namespace ext {
	namespace GuiBehavior {
		UF_BEHAVIOR_DEFINE_TYPE;
		void attach( uf::Object& );
		void initialize( uf::Object& );
		void tick( uf::Object& );
		void render( uf::Object& );
		void destroy( uf::Object& );
		struct Metadata {
			pod::Vector4f color = {1,1,1,1};
			bool world = false;
			pod::Vector2f size = {0,0};
			size_t mode = 0;
			struct {
				pod::Vector2f min = {  1,  1 };
				pod::Vector2f max = { -1, -1 };
			} box;
			bool clicked = false;
			bool hovered = false;

			pod::Vector4f uv = {0, 0, 1, 1};
			size_t shader = 0;
			float depth = 0;
			float alpha = -1;

			std::function<void()> serialize;
			std::function<void()> deserialize;
		};
		struct GlyphMetadata {
			uf::stl::string font = "";
			uf::stl::string string = "";
			
			float scale = 0;
			pod::Vector2f padding = {0,0};
			
			float spread = 0;
			float size = 0;
			float weight = 0;
			bool sdf = false;
			bool shadowbox = false;
			pod::Vector4f stroke = {0,0,0,0};

			uf::stl::string origin = "";
			uf::stl::string align = "";
			uf::stl::string direction = "";

			std::function<void()> serialize;
			std::function<void()> deserialize;
		};
	}
}