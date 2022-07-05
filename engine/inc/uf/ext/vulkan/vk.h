#pragma once

#include <uf/config.h>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include <typeinfo>
#include <iostream>
#include <cassert>
#include <mutex>

#define VK_VALIDATION_MESSAGE(...) if ( ext::vulkan::settings::validation ) UF_MSG_DEBUG(__VA_ARGS__);

#define VK_CHECK_RESULT(f) { VkResult res = (f); if ( res != VK_SUCCESS ) UF_EXCEPTION("{}", ext::vulkan::errorString( res )); }

#define VK_FLAGS_NONE 0
#define VK_DEFAULT_FENCE_TIMEOUT 100000000000
#define VK_DEFAULT_STAGE_BUFFERS ext::vulkan::settings::defaultStageBuffers

namespace ext {
	namespace vulkan {
		namespace settings {
			extern UF_API bool defaultStageBuffers;
		}
	}
}