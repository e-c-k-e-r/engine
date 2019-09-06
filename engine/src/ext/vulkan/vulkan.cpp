#include <uf/ext/glfw/glfw.h>
#include <uf/ext/vulkan/vulkan.h>
#include <uf/ext/vulkan/initializers.h>

#include <fstream>

uint32_t ext::vulkan::width = 800;
uint32_t ext::vulkan::height = 600;
ext::vulkan::Device ext::vulkan::device;
ext::vulkan::Swapchain ext::vulkan::swapchain;
ext::vulkan::Allocator ext::vulkan::allocator;
std::vector<ext::vulkan::Graphic*> ext::vulkan::graphics;
std::vector<uint32_t> ext::vulkan::passes = { 0 };
bool ext::vulkan::resizedFramebuffer = false;
bool ext::vulkan::validation = true;
std::mutex ext::vulkan::mutex;

VkResult ext::vulkan::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if ( func == nullptr ) return VK_ERROR_EXTENSION_NOT_PRESENT;
	return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
}

void ext::vulkan::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) func(instance, debugMessenger, pAllocator);
}

VKAPI_ATTR VkBool32 VKAPI_CALL ext::vulkan::debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData
) {
	std::cerr << "[Validation Layer] " << pCallbackData->pMessage << std::endl;
	return VK_FALSE;
}

VkShaderModule ext::vulkan::loadShader(const char *filename, VkDevice device) {
	std::ifstream is(filename, std::ios::binary | std::ios::in | std::ios::ate);

	if ( !is.is_open() ) {
		std::cerr << "Error: Could not open shader file \"" << filename << "\"" << std::endl;
		return VK_NULL_HANDLE;
	}
	size_t size = is.tellg();
	is.seekg(0, std::ios::beg);
	char* shaderCode = new char[size];
	is.read(shaderCode, size);
	is.close();

	assert(size > 0);

	VkShaderModule shaderModule;
	VkShaderModuleCreateInfo moduleCreateInfo{};
	moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	moduleCreateInfo.codeSize = size;
	moduleCreateInfo.pCode = (uint32_t*)shaderCode;

//	std::cout << filename << ": ";
	VK_CHECK_RESULT(vkCreateShaderModule(device, &moduleCreateInfo, NULL, &shaderModule));
//	std::cout << "Shader OK: " << shaderModule << std::endl;

	delete[] shaderCode;
	return shaderModule;
}

VkPipelineShaderStageCreateInfo ext::vulkan::loadShader( std::string filename, VkShaderStageFlagBits stage, VkDevice device, std::vector<VkShaderModule>& shaderModules ) {
	VkPipelineShaderStageCreateInfo shaderStage = {};
	shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStage.stage = stage;
	shaderStage.module = loadShader(filename.c_str(), device);
	shaderStage.pName = "main"; // todo : make param
	assert(shaderStage.module != VK_NULL_HANDLE);
	shaderModules.push_back(shaderStage.module);
	return shaderStage;
}

std::string ext::vulkan::errorString( VkResult result ) {
	switch ( result ) {
#define STR(r) case VK_ ##r: return #r
		STR(NOT_READY);
		STR(TIMEOUT);
		STR(EVENT_SET);
		STR(EVENT_RESET);
		STR(INCOMPLETE);
		STR(ERROR_OUT_OF_HOST_MEMORY);
		STR(ERROR_OUT_OF_DEVICE_MEMORY);
		STR(ERROR_INITIALIZATION_FAILED);
		STR(ERROR_DEVICE_LOST);
		STR(ERROR_MEMORY_MAP_FAILED);
		STR(ERROR_LAYER_NOT_PRESENT);
		STR(ERROR_EXTENSION_NOT_PRESENT);
		STR(ERROR_FEATURE_NOT_PRESENT);
		STR(ERROR_INCOMPATIBLE_DRIVER);
		STR(ERROR_TOO_MANY_OBJECTS);
		STR(ERROR_FORMAT_NOT_SUPPORTED);
		STR(ERROR_SURFACE_LOST_KHR);
		STR(ERROR_NATIVE_WINDOW_IN_USE_KHR);
		STR(SUBOPTIMAL_KHR);
		STR(ERROR_OUT_OF_DATE_KHR);
		STR(ERROR_INCOMPATIBLE_DISPLAY_KHR);
		STR(ERROR_VALIDATION_FAILED_EXT);
		STR(ERROR_INVALID_SHADER_NV);
#undef STR
	default:
		return "UNKNOWN_ERROR";
	}
}

uint32_t ext::vulkan::getMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties) {
	// Iterate over all memory types available for the device used in this example
	for (uint32_t i = 0; i < ext::vulkan::device.memoryProperties.memoryTypeCount; i++) {
		if ((typeBits & 1) == 1) {
			if ((ext::vulkan::device.memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {						
				return i;
			}
		}
		typeBits >>= 1;
	}
	throw std::runtime_error("Could not find a suitable memory type!");
}

void* ext::vulkan::alignedAlloc( size_t size, size_t alignment ) {
	void *data = nullptr;
#if defined(_MSC_VER) || defined(__MINGW32__)
	data = _aligned_malloc(size, alignment);
#else 
	int res = posix_memalign(&data, alignment, size);
	if (res != 0)
		data = nullptr;
#endif
	return data;
}

void ext::vulkan::alignedFree(void* data) {
#if	defined(_MSC_VER) || defined(__MINGW32__)
_aligned_free(data);
#else 
free(data);
#endif
}

namespace {
	uint32_t currentBuffer = 0;
	uint32_t currentId = 0;
/*
	ext::vulkan::Compute::Sphere newSphere(glm::vec3 pos, float radius, glm::vec3 diffuse, float specular) {
		ext::vulkan::Compute::Sphere sphere;
		sphere.id = currentId++;
		sphere.position = pos;
		sphere.radius = radius;
		sphere.diffuse = diffuse;
		sphere.specular = specular;
		return sphere;
	}

	ext::vulkan::Compute::Plane newPlane(glm::vec3 normal, float distance, glm::vec3 diffuse, float specular) {
		ext::vulkan::Compute::Plane plane;
		plane.id = currentId++;
		plane.normal = normal;
		plane.distance = distance;
		plane.diffuse = diffuse;
		plane.specular = specular;
		return plane;
	}
*/
}

void ext::vulkan::initialize( uint8_t stage ) {
	switch ( stage ) {
		case 0: {
			device.initialize();
			swapchain.initialize( device );
			swapchain.createCommandBuffers();

			{
				VmaAllocatorCreateInfo allocatorInfo = {};
				allocatorInfo.physicalDevice = device.physicalDevice;
				allocatorInfo.device = device.logicalDevice;
				vmaCreateAllocator(&allocatorInfo, &allocator);
			}
		} break;
		case 1: {
			for ( Graphic* graphic : graphics ) {
				graphic->initialize( device, swapchain );
			}
		} break;
		case 2: {
			swapchain.createCommandBuffers();
		} break;
		default: {
			throw std::runtime_error("invalid stage id");
		} break;
	}
}
#include <ostream>
std::ostream& operator<<(std::ostream& os, const ext::vulkan::Graphic& graphic) {
	os << graphic.name() << ": " << &graphic;
	return os;
}
void ext::vulkan::tick() {
	// check for changes in swapchain
	// ext::vulkan::mutex.lock();
	for ( Graphic* graphic : graphics ) {
		if ( !graphic->initialized ) {
			swapchain.rebuild = true;
			graphic->initialize( device, swapchain );
		}
	}
	if ( swapchain.rebuild ) {
		swapchain.createCommandBuffers();
		swapchain.rebuild = false;
	}
	// ext::vulkan::mutex.unlock();
}
void ext::vulkan::render() {
	// Get next image in the swap chain (back/front buffer)
	VK_CHECK_RESULT(swapchain.acquireNextImage(&currentBuffer));

	// Use a fence to wait until the command buffer has finished execution before using it again
	VK_CHECK_RESULT(vkWaitForFences(device, 1, &swapchain.waitFences[currentBuffer], VK_TRUE, UINT64_MAX));
	VK_CHECK_RESULT(vkResetFences(device, 1, &swapchain.waitFences[currentBuffer]));

	// Pipeline stage at which the queue submission will wait (via pWaitSemaphores)
	VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	// The submit info structure specifices a command buffer queue submission batch
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pWaitDstStageMask = &waitStageMask;									// Pointer to the list of pipeline stages that the semaphore waits will occur at
	submitInfo.pWaitSemaphores = &swapchain.presentCompleteSemaphore;				// Semaphore(s) to wait upon before the submitted command buffer starts executing
	submitInfo.waitSemaphoreCount = 1;												// One wait semaphore																				
	submitInfo.pSignalSemaphores = &swapchain.renderCompleteSemaphore;				// Semaphore(s) to be signaled when command buffers have completed
	submitInfo.signalSemaphoreCount = 1;											// One signal semaphore
	submitInfo.pCommandBuffers = &swapchain.drawCommandBuffers[currentBuffer];		// Command buffers(s) to execute in this batch (submission)
	submitInfo.commandBufferCount = 1;

	// Submit to the graphics queue passing a wait fence
	VK_CHECK_RESULT(vkQueueSubmit(device.graphicsQueue, 1, &submitInfo, swapchain.waitFences[currentBuffer]));
	
	// Present the current buffer to the swap chain
	// Pass the semaphore signaled by the command buffer submission from the submit info as the wait semaphore for swap chain presentation
	// This ensures that the image is not presented to the windowing system until all commands have been submitted
	VK_CHECK_RESULT(swapchain.queuePresent(device.presentQueue, currentBuffer, swapchain.renderCompleteSemaphore));
	VK_CHECK_RESULT(vkQueueWaitIdle(device.presentQueue));
	
	// Handle resizes
	if ( resizedFramebuffer ) {
		resizedFramebuffer = false;
	}

	for ( Graphic* graphic : graphics ) {
		if ( !graphic || !graphic->process ) continue;
			graphic->render();
	}
}
void ext::vulkan::destroy() {
	vkDeviceWaitIdle( device );
	for ( Graphic* graphic : graphics ) {
		graphic->destroy();
	}
	vmaDestroyAllocator( allocator );
	swapchain.destroy();
	device.destroy();
}

std::string ext::vulkan::allocatorStats() {
	char* statsString = nullptr;
	vmaBuildStatsString(ext::vulkan::allocator, &statsString, true);
	std::string result = statsString;
	vmaFreeStatsString(ext::vulkan::allocator, statsString);
	return result;
}