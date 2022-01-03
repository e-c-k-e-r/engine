#pragma once

namespace pod {
	namespace payloads {
		struct Entity {
			size_t uid{};
			uf::Object* pointer = NULL;
		};
	}
}