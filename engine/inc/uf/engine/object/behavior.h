#pragma once

#include <uf/config.h>
#include <uf/ext/json/json.h>
#include <uf/engine/behavior/behavior.h>

#include <string>
#include <vector>
#include <unordered_map>

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
				std::string name;
				ext::json::Value payload;
				double timeout;
			};
			struct {
				size_t mtime = 0;
				bool enabled = false;
				std::string source = "";
			} hotReload;
			struct {
				std::unordered_map<std::string, std::vector<size_t>> bound;
				std::vector<Queued> queue;
			} hooks;
			struct {
				bool ignoreGraph = false;
			} system;
		};
	}
}