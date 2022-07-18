#pragma once

namespace uf {
	namespace SceneBehavior {
		UF_BEHAVIOR_DEFINE_TYPE();
		UF_BEHAVIOR_DEFINE_TRAITS();
		UF_BEHAVIOR_DEFINE_FUNCTIONS();
		UF_BEHAVIOR_DEFINE_METADATA(
			uf::stl::vector<uf::Entity*> graph;
			bool invalidationQueued = false;
		#if 0
			struct {
				uf::Entity* controller = NULL;
				void* renderMode = NULL;
			} cached;
		#else
		// 	we could keep a cache of controllers for each rendermode, but we have to invalidate the cache every time the graph regenerates
			uf::stl::unordered_map<uf::stl::string, uf::Entity*> cache;
		#endif

			struct {
				pod::Thread::Tasks serial;
				pod::Thread::Tasks parallel;
			} tasks;
		);
	}
}