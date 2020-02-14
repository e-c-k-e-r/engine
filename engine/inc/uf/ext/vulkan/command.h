#pragma once

#include <uf/ext/vulkan/device.h>

namespace ext {
	namespace vulkan {
		struct UF_API Command {
			// RAII
			void initialize( Device& device );
			void createCommandBuffers();
			void createCommandBuffers( const std::vector<void*>& graphics, const std::vector<uint32_t>& passes );
			void render();
			void destroy();
		};
	}
}