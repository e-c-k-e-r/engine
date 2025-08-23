#pragma once

#include <uf/config.h>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include <typeinfo>
#include <iostream>
#include <cassert>
#include <mutex>

#define VK_VALIDATION_MESSAGE(...) if ( ext::vulkan::settings::validation::messages ) UF_MSG("VULKAN", __VA_ARGS__);

#define VK_CHECK_QUEUE_CHECKPOINT(QUEUE, RES) {\
	if ( RES == VK_ERROR_DEVICE_LOST ) UF_MSG_ERROR( "ERROR_DEVICE_LOST detected, retrieving checkpoint:\n{}", ext::vulkan::retrieveCheckpoint(QUEUE) );\
	if ( RES != VK_SUCCESS ) UF_EXCEPTION("{}", ext::vulkan::errorString( RES ));\
}
#define VK_CHECK_RESULT(f) { VkResult res = (f); if ( res != VK_SUCCESS ) UF_EXCEPTION("{}", ext::vulkan::errorString( res )); }

#define VK_FLAGS_NONE 0
#define VK_DEFAULT_FENCE_TIMEOUT ext::vulkan::settings::defaultTimeout
#define VK_DEFAULT_STAGE_BUFFERS ext::vulkan::settings::defaultStageBuffers
#define VK_DEFAULT_DEFER_BUFFER_DESTROY ext::vulkan::settings::defaultDeferBufferDestroy
#define VK_DEFAULT_COMMAND_BUFFER_IMMEDIATE ext::vulkan::settings::defaultCommandBufferImmediate

namespace ext {
	namespace vulkan {
		namespace settings {
			extern UF_API bool defaultStageBuffers;
			extern UF_API bool defaultDeferBufferDestroy;
			extern UF_API bool defaultCommandBufferImmediate;
		}
	}
}