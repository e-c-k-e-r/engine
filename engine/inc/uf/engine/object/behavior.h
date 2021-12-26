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
				uf::stl::string name;
				pod::Hook::userdata_t userdata;
				ext::json::Value json;
				double timeout = 0;
				int_fast8_t type = 0;
			};
			struct {
				size_t mtime = 0;
				bool enabled = false;
				uf::stl::string source = "";
			} hotReload;
			struct {
				uf::stl::unordered_map<uf::stl::string, uf::stl::vector<size_t>> bound;
				uf::stl::vector<Queued> queue;
			} hooks;
			struct {
				bool ignoreGraph = false;
			} system;
			struct {
				pod::Transform<> initial;
				bool trackParent = false;
			} transform;
		);
	}
}