#pragma once

namespace pod {
	namespace payloads {
		struct Entity {
			size_t uid{};
			uf::Object* pointer = NULL;
		};

		struct entitySpawn {
			uf::stl::string filename;
			uf::Serializer metadata;

			pod::Transform<> transform;

			pod::payloads::Entity parent;
		};
	}
}