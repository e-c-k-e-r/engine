#include <uf/ext/glfw/glfw.h>
#include <uf/ext/vulkan/vulkan.h>
#include <uf/ext/vulkan/initializers.h>

#include <fstream>

uint32_t ext::vulkan::width = 800;
uint32_t ext::vulkan::height = 600;
uint32_t ext::vulkan::currentBuffer = 600;
ext::vulkan::Device ext::vulkan::device;
ext::vulkan::Swapchain ext::vulkan::swapchain;
ext::vulkan::Allocator ext::vulkan::allocator;
// std::vector<ext::vulkan::Graphic*> ext::vulkan::graphics;
std::vector<ext::vulkan::Graphic*>* ext::vulkan::graphics = NULL;
std::vector<std::string> ext::vulkan::passes = { "BASE" };
std::string ext::vulkan::currentPass = "BASE";
bool ext::vulkan::resizedFramebuffer = false;
bool ext::vulkan::validation = true;
bool ext::vulkan::openvr = false;
std::mutex ext::vulkan::mutex;
ext::vulkan::Command* ext::vulkan::command = NULL;

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

	VK_CHECK_RESULT(vkCreateShaderModule(device, &moduleCreateInfo, NULL, &shaderModule));

	delete[] shaderCode;
	return shaderModule;
}

VkPipelineShaderStageCreateInfo ext::vulkan::loadShader( std::string filename, VkShaderStageFlagBits stage, VkDevice device, std::vector<VkShaderModule>& shaderModules ) {
	VkPipelineShaderStageCreateInfo shaderStage = {};
	shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStage.stage = stage;
	shaderStage.module = loadShader(filename.c_str(), device);
	shaderStage.pName = "main";
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

#include <uf/ext/vulkan/commands/multiview.h>

void ext::vulkan::initialize( uint8_t stage ) {
	switch ( stage ) {
		case 0: {
			device.initialize();
			swapchain.initialize( device );
			{
				VmaAllocatorCreateInfo allocatorInfo = {};
				allocatorInfo.physicalDevice = device.physicalDevice;
				allocatorInfo.device = device.logicalDevice;
				vmaCreateAllocator(&allocatorInfo, &allocator);
			}
			if ( !ext::vulkan::command ) ext::vulkan::command = ext::vulkan::openvr ? new ext::vulkan::MultiviewCommand() : new ext::vulkan::Command();
			ext::vulkan::command->initialize(device);
			ext::vulkan::command->createCommandBuffers();

		} break;
		case 1: if ( ext::vulkan::graphics ) {
			auto& graphics = *ext::vulkan::graphics;
			for ( Graphic* graphic : graphics ) {
				graphic->initialize( device, swapchain );
			}
		} break;
		case 2: {
			ext::vulkan::command->createCommandBuffers();
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
	if ( ext::vulkan::graphics ) {
		auto& graphics = *ext::vulkan::graphics;
		for ( Graphic* graphic : graphics ) {
			if ( !graphic->initialized ) {
				swapchain.rebuild = true;
				graphic->initialize( device, swapchain );
			}
		}
	}
	if ( swapchain.rebuild ) {
		ext::vulkan::command->createCommandBuffers();
		swapchain.rebuild = false;
	}
	// ext::vulkan::mutex.unlock();
}
void ext::vulkan::render() {
	ext::vulkan::command->render();
	
	// Handle resizes
	if ( resizedFramebuffer ) {
		resizedFramebuffer = false;
	}

	if ( !ext::vulkan::graphics ) return;
	auto& graphics = *ext::vulkan::graphics;
	for ( Graphic* graphic : graphics ) {
		if ( !graphic || !graphic->process ) continue;
			graphic->render();
	}
}
void ext::vulkan::destroy() {
	vkDeviceWaitIdle( device );

	ext::vulkan::command->destroy();
	delete ext::vulkan::command;
	ext::vulkan::command = NULL;

	if ( ext::vulkan::graphics ) {
		auto& graphics = *ext::vulkan::graphics;
		for ( Graphic* graphic : graphics ) {
			graphic->destroy();
		}
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