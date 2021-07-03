#pragma once

namespace uf {
	namespace SceneBehavior {
		UF_BEHAVIOR_DEFINE_TYPE;
		UF_BEHAVIOR_DEFINE_TRAITS;
		UF_BEHAVIOR_DEFINE_FUNCTIONS;
		UF_BEHAVIOR_DEFINE_METADATA {
			uf::stl::vector<uf::Entity*> graph;
			bool invalidationQueued = false;
			struct {
				uf::Entity* controller = NULL;
				void* renderMode = NULL;
			} cached;
		};
	}
}