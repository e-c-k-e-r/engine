#pragma once

#include <uf/engine/object/object.h>

#define UF_SCENE_USE_GRAPH 1

namespace uf {
	class UF_API Scene : public uf::Object {
	public:
		Scene();

		uf::Entity& getController();
		const uf::Entity& getController() const;

		const std::vector<uf::Entity*>& getGraph();
		std::vector<uf::Entity*> getGraph( bool );
		void invalidateGraph();

		template<typename T> T& getController() {
			return this->getController().as<T>();
		}
		template<typename T> const T& getController() const {
			return this->getController().as<T>();
		}
	};

	namespace scene {
		extern UF_API std::vector<uf::Scene*> scenes;
	//	extern UF_API std::vector<uf::Entity*> graph;
	//	extern UF_API bool queuedInvalidation;
	//	extern UF_API bool useGraph;
	//	void UF_API invalidateGraph();
	//	std::vector<uf::Entity*> UF_API generateGraph( bool = false );
		
		void UF_API invalidateGraphs();

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