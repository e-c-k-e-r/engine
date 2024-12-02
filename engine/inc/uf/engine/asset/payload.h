#pragma once

#include <uf/utils/resolvable/resolvable.h>
#include <uf/utils/serialize/serializer.h>
#include <uf/utils/string/ext.h>
#include <functional>

namespace uf {
	class UF_API Entity;

	namespace asset {
		enum Type {
			UNKNOWN,
			IMAGE,
			AUDIO,
			LUA,
			JSON,
			GRAPH,
		};

		struct UF_API Payload {
			uf::asset::Type type = {};
			uf::stl::string filename = "";
			uf::stl::string mime = "";
			uf::stl::string hash = "";
			uf::stl::string uri = "";

			bool initialize = true;
			bool monoThreaded = false;
			bool asComponent = false;

			uf::Serializer metadata;

			pod::Resolvable<uf::Entity> object;
		};
	}
}