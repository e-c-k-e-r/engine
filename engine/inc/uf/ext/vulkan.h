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

#define VK_CHECK_RESULT(f)																				\
{																										\
	VkResult res = (f);																					\
	if (res != VK_SUCCESS) {																									\
		std::cerr << "Fatal : VkResult is \"" << ext::vulkan::errorString( res ) << "\" in " << __FILE__ << " at line " << __LINE__ << std::endl; \
		throw std::runtime_error(ext::vulkan::errorString( res ));										\
	}																									\
}

#define VK_FLAGS_NONE 0
#define DEFAULT_FENCE_TIMEOUT 100000000000

namespace ext {
	namespace vulkan {
		std::string errorString( VkResult result );
		void* alignedAlloc( size_t size, size_t alignment );
		void alignedFree(void* data);
	}
}

#include <vk_mem_alloc.h>