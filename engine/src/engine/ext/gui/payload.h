#pragma once

#include <uf/utils/image/image.h>
#include <uf/utils/mesh/mesh.h>

namespace ext {
	namespace payloads {
		struct GuiInitializationPayload {
			bool free = false;

			uf::Mesh* mesh = NULL;
			uf::Image* image = NULL;
		};
	}
}