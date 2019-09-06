#pragma once

#include <uf/config.h>
#include <uf/ext/ext.h>
#include <uf/engine/entity/entity.h>
/*
#include <uf/gl/texture/texture.h>
#include <uf/gl/camera/camera.h>
#include <uf/gl/gbuffer/gbuffer.h>
*/
#include <uf/utils/math/transform.h>
#include <uf/utils/math/physics.h>

#include "../object/object.h"

namespace ext {
	class EXT_API Light : public ext::Object {
	public:
		void initialize();
		void tick();
		void render();
	};
}