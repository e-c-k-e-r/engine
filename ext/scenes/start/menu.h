#pragma once

#include "../base.h"

namespace ext {
	class EXT_API StartMenu : public ext::Scene {
	public:
		virtual void initialize();
		virtual void tick();
		virtual void render();
		
		virtual uf::Entity& getController();
		virtual const uf::Entity& getController() const;
	};
}