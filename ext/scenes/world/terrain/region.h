#pragma once

#include <uf/config.h>
#include <uf/ext/ext.h>
#include <uf/engine/entity/entity.h>

#include <uf/utils/math/transform.h>
#include <uf/utils/math/physics.h>

#include <uf/engine/object/object.h>

namespace ext {
	class EXT_API Region : public uf::Object {
	protected:
	public:
		virtual void initialize();
		virtual void load();
		virtual void tick();
		virtual void render();
		virtual void destroy();
	};
}