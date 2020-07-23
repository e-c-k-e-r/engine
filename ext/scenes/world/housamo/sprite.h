#pragma once

#include <uf/config.h>
#include <uf/ext/ext.h>
#include "../craeture/craeture.h"

namespace ext {
	class EXT_API HousamoSprite : public ext::Craeture {
	protected:
	public:
		virtual void initialize();
		virtual void tick();
		virtual void render();
		virtual void destroy();
	};
}