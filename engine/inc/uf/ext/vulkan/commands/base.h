#pragma once

#include <uf/ext/vulkan/device.h>

namespace ext {
	namespace vulkan {
		struct UF_API Command {
			uint32_t width, height;
			// RAII
			virtual const std::string& getName() const;
			virtual void initialize( Device& device );
			virtual void createCommandBuffers();
			virtual void createCommandBuffers( const std::vector<void*>& graphics, const std::vector<std::string>& passes );
			virtual void render();
			virtual void destroy();
		};
	}
}