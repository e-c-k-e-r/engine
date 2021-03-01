#pragma once

#include <uf/config.h>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include <typeinfo>
#include <iostream>
#include <cassert>
#include <mutex>

#define VK_DEBUG_MESSAGE(...) UF_DEBUG_MSG(__VA_ARGS__);
#define VK_VALIDATION_MESSAGE(...) if ( ext::vulkan::settings::validation ) VK_DEBUG_MESSAGE(__VA_ARGS__);

#define VK_CHECK_RESULT(f) {										\
	VkResult res = (f);												\
	if (res != VK_SUCCESS) {										\
		std::string errorString = ext::vulkan::errorString( res ); 	\
		VK_DEBUG_MESSAGE(errorString); 								\
		UF_EXCEPTION(errorString);									\
	}																\
}

#define VK_FLAGS_NONE 0
#define DEFAULT_FENCE_TIMEOUT 100000000000
#define VK_DEFAULT_STAGE_BUFFERS 1