#pragma once

namespace pod {
	struct CachedCamera {
		uf::Camera camera;
		size_t lastFrame = 0;
	};
}

namespace uf {
	namespace SceneBehavior {
		UF_BEHAVIOR_DEFINE_TYPE();
		UF_BEHAVIOR_DEFINE_TRAITS();
		UF_BEHAVIOR_DEFINE_FUNCTIONS();
		UF_BEHAVIOR_DEFINE_METADATA(
			uf::stl::vector<uf::Entity*> graph;
			bool invalidationQueued = false;

		// 	we could keep a cache of controllers for each rendermode, but we have to invalidate the cache every time the graph regenerates
			struct {
				uf::stl::unordered_map<uf::stl::string, uf::Entity*> controllers;
				uf::stl::unordered_map<size_t, pod::CachedCamera> cameras;
			} cache;

			struct {
				pod::Thread::Tasks serial;
				pod::Thread::Tasks parallel;
			} tasks;

			struct {
				size_t hash;
			} mount;
		);
	}
}