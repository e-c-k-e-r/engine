#pragma once

#include <uf/engine/object/object.h>

namespace uf {
	class UF_API Scene : public uf::Object {
	public:
		virtual uf::Entity& getController();
		virtual const uf::Entity& getController() const;

		template<typename T> T& getController() {
			return *((T*) &this->getController());
		}
		template<typename T> const T& getController() const {
			return *((const T*) &this->getController());
		}

		Scene();
	#if UF_BEHAVIOR_VIRTUAL
		virtual void initialize();
		virtual void tick();
		virtual void render();
		virtual void destroy();
	#endif
	};

	namespace scene {
		extern UF_API std::vector<uf::Scene*> scenes;
		Scene& UF_API getCurrentScene();
		Scene& UF_API loadScene( const std::string& name, const std::string& filename = "" );
		Scene& UF_API loadScene( const std::string& name, const uf::Serializer& );
		void UF_API unloadScene();

		void UF_API tick();
		void UF_API render();
		void UF_API destroy();
	}
}

#include "behavior.h"