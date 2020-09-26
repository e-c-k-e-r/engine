#pragma once

#include <uf/config.h>
#include <uf/ext/ext.h>
#include <uf/engine/object/object.h>
#include <uf/engine/scene/scene.h>

namespace ext {
	class EXT_API Marching : public uf::Object {
	public:
		virtual void initialize();
		virtual void tick();
		virtual void destroy();
		virtual void render();
	};
}