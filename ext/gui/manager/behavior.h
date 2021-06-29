#pragma once

#include <uf/config.h>
#include <uf/ext/ext.h>
#include <uf/engine/entity/entity.h>
#include <uf/engine/scene/scene.h>

namespace ext {
	namespace GuiManagerBehavior {
		UF_BEHAVIOR_DEFINE_TYPE;
		void attach( uf::Object& );
		void initialize( uf::Object& );
		void tick( uf::Object& );
		void render( uf::Object& );
		void destroy( uf::Object& );
		struct Metadata {
			struct {
				struct {
					uf::stl::string type = "";
					pod::Vector4f color{};
					pod::Vector3f position{};
					float radius{};
					bool enabled{};
				} cursor;

				bool enabled{};
				bool floating{};
				pod::Transform<> transform;
			} overlay;

			std::function<void()> serialize;
			std::function<void()> deserialize;
		};
	}
}