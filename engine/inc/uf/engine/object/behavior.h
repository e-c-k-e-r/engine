#pragma once

#include <uf/config.h>
#include <uf/ext/json/json.h>
#include <uf/utils/math/transform.h>
#include <uf/engine/behavior/behavior.h>

#include <uf/utils/memory/string.h>
#include <uf/utils/memory/vector.h>
#include <uf/utils/memory/unordered_map.h>

namespace uf {
	namespace ObjectBehavior {
		UF_BEHAVIOR_DEFINE_TYPE;
		void UF_API attach( uf::Entity& );
		void UF_API initialize( uf::Object& );
		void UF_API tick( uf::Object& );
		void UF_API render( uf::Object& );
		void UF_API destroy( uf::Object& );
		struct Metadata {
			struct Queued {
				uf::stl::string name;
				ext::json::Value payload;
				double timeout;
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

			std::function<void()> serialize;
			std::function<void()> deserialize;
		};
	}
}