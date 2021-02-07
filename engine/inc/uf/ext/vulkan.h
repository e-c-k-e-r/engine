#pragma once

#include <uf/config.h>
#include <vulkan/vulkan.h>


#include <iostream>
#include <cassert>
#include <mutex>

/*
#include <stdexcept>
#include <functional>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <map>
#include <set>
#include <algorithm>
#include <fstream>
#include <chrono>
*/
#define VK_DEBUG_MESSAGE(...)\
	uf::iostream << "[" << __FUNCTION__ << "@" << __FILE__ ":" << __LINE__ << "] " << __VA_ARGS__ << "\n"

#define VK_VALIDATION_MESSAGE(...)\
	if ( ext::vulkan::settings::validation ) VK_DEBUG_MESSAGE(__VA_ARGS__);

#define VK_CHECK_RESULT(f) {										\
	VkResult res = (f);												\
	if (res != VK_SUCCESS) {										\
		std::string errorString = ext::vulkan::errorString( res ); 	\
		VK_DEBUG_MESSAGE(errorString); 								\
		throw std::runtime_error(errorString);						\
	}																\
}

#define VK_FLAGS_NONE 0
#define DEFAULT_FENCE_TIMEOUT 100000000000
#define VK_DEFAULT_STAGE_BUFFERS 1

#include <vk_mem_alloc.h>