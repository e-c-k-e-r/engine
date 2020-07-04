#pragma once

#include <uf/config.h>
#include <uf/ext/ext.h>
#include <uf/engine/entity/entity.h>

#include "../object/object.h"

namespace ext {
	class EXT_API Scene : public ext::Object {
	protected:
		void* m_graphics;
		template<typename T> T& getGraphics() {
			return *((T*) this->m_graphics);
		}
	public:
		static ext::Scene* current;

		virtual void initialize();
		virtual void tick();
		virtual void render();
		virtual void destroy();

		virtual uf::Entity* getController();
		virtual const uf::Entity* getController() const;

		template<typename T> T& getController() {
			return *((T*) this->getController());
		}
		template<typename T> const T& getController() const {
			return *((const T*) this->getController());
		}
	};
}