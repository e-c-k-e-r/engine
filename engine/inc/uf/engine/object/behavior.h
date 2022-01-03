#pragma once

#include <uf/config.h>
#include <uf/ext/json/json.h>
#include <uf/utils/math/transform.h>
#include <uf/engine/behavior/behavior.h>

#include <uf/utils/memory/string.h>
#include <uf/utils/memory/vector.h>
#include <uf/utils/memory/unordered_map.h>
#include <uf/utils/hook/hook.h>

namespace uf {
	namespace ObjectBehavior {
		UF_BEHAVIOR_DEFINE_TYPE();
		UF_BEHAVIOR_DEFINE_TRAITS();
		UF_BEHAVIOR_DEFINE_FUNCTIONS();
		UF_BEHAVIOR_DEFINE_METADATA(
			struct Queued {
				uf::stl::string name = "";
				double timeout = 0;
				int_fast8_t type = 0;

				pod::Hook::userdata_t userdata{};
				ext::json::Value json{};
			};
			struct {
				uf::stl::unordered_map<uf::stl::string, uf::stl::vector<size_t>> bound;
				uf::stl::vector<Queued> queue;
			} hooks;
			struct {
				uf::stl::string root = "";
				uf::stl::string filename = "";
				struct {
					size_t mtime = 0;
					bool enabled = false;
				} hotReload;

				bool loaded = false;
				struct {
					bool ignore = false;
					size_t progress{};
					size_t total{};
				} load;
				
				bool ignoreGraph = false;
			} system;
			struct {
				pod::Transform<> initial;
				bool trackParent = false;
			} transform;
		);
	}
}