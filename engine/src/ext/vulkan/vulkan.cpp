#include <uf/ext/glfw/glfw.h>
#include <uf/ext/vulkan/vulkan.h>
#include <uf/ext/vulkan/initializers.h>
#include <uf/ext/vulkan/graphic.h>
#include <uf/ext/vulkan/rendermode.h>
#include <uf/utils/graphic/graphic.h>

#include <ostream>
#include <fstream>

uint32_t ext::vulkan::width = 1280;
uint32_t ext::vulkan::height = 720;

bool ext::vulkan::validation = true;
std::vector<std::string> ext::vulkan::validationFilters;
std::vector<std::string> ext::vulkan::requestedDeviceFeatures;
ext::vulkan::Device ext::vulkan::device;
ext::vulkan::Allocator ext::vulkan::allocator;
ext::vulkan::Swapchain ext::vulkan::swapchain;
std::mutex ext::vulkan::mutex;

bool ext::vulkan::resized = false;
bool ext::vulkan::rebuild = false;
uint32_t ext::vulkan::currentBuffer = 0;
std::vector<std::string> ext::vulkan::passes = { "BASE" };
std::vector<uf::Scene*> ext::vulkan::scenes;
ext::vulkan::RenderMode* ext::vulkan::currentRenderMode = NULL;

std::vector<ext::vulkan::RenderMode*> ext::vulkan::renderModes = {
	new ext::vulkan::BaseRenderMode,
};

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
	if ( messageSeverity <= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT )  return VK_FALSE;
	std::string message = pCallbackData->pMessage;
	for ( auto& filter : ext::vulkan::validationFilters ) {
		if ( message.find(filter) != std::string::npos ) return VK_FALSE;
	}
	uf::iostream << "[Validation Layer] " << message << "\n";
	return VK_FALSE;
}

VkShaderModule ext::vulkan::loadShader(const char *filename, VkDevice device) {
	std::ifstream is(filename, std::ios::binary | std::ios::in | std::ios::ate);

	if ( !is.is_open() ) {
		uf::iostream << "Error: Could not open shader file \"" << filename << "\"" << "\n";
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

namespace {
	bool hasRenderMode( const std::string& name, bool isName ) {
		for ( auto& renderMode: ext::vulkan::renderModes ) {
			if ( isName ) {
				if ( renderMode->getName() == name ) return true;
			} else {
				if ( renderMode->getType() == name ) return true;
			}
		}
		return false;
	}
}

ext::vulkan::RenderMode& ext::vulkan::addRenderMode( ext::vulkan::RenderMode* mode, const std::string& name ) {
	mode->name = name;
	renderModes.push_back(mode);
	std::cout << "Adding RenderMode: " << name << ": " << mode->getType() << std::endl;
	// reorder
	return *mode;
}
ext::vulkan::RenderMode& ext::vulkan::getRenderMode( const std::string& name, bool isName ) {
	RenderMode* target = renderModes[renderModes.size()-1];
	for ( auto& renderMode: renderModes ) {
		if ( isName ) {
			if ( renderMode->getName() == "" )  target = renderMode;
			if ( renderMode->getName() == name ) {
				target = renderMode;
				break;
			}
		} else {
			if ( renderMode->getType() == name ) {
				target = renderMode;
				break;
			}
		}
	}
//	std::cout << "Requesting RenderMode `" << name << "`, got `" << target->getName() << "` (" << target->getType() << ")" << std::endl;
	return *target;
}
std::vector<ext::vulkan::RenderMode*> ext::vulkan::getRenderModes( const std::string& name, bool isName ) {
	std::vector<RenderMode*> targets;
	for ( auto& renderMode: renderModes ) {
		if ( ( isName && renderMode->getName() == name ) || renderMode->getType() == name ) {
			targets.push_back(renderMode);
//			std::cout << "Requestings RenderMode `" << name << "`, got `" << renderMode->getName() << "` (" << renderMode->getType() << ")" << std::endl;
		}
	}
	return targets;
}
void ext::vulkan::removeRenderMode( ext::vulkan::RenderMode* mode, bool free ) {
	if ( !mode ) return;
	renderModes.erase( std::remove( renderModes.begin(), renderModes.end(), mode ), renderModes.end() );
	mode->destroy();
	if ( free ) delete mode;
}

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
			{
				std::vector<uint8_t> pixels = { 
					255,   0, 255, 255,      0,   0,   0, 255,
					  0,   0,   0, 255,    255,   0, 255, 255,
				};
				Texture2D::empty.sampler.descriptor.filter.min = VK_FILTER_NEAREST;
				Texture2D::empty.sampler.descriptor.filter.mag = VK_FILTER_NEAREST;
				Texture2D::empty.fromBuffers( (void*) &pixels[0], pixels.size(), VK_FORMAT_R8G8B8A8_UNORM, 2, 2, ext::vulkan::device, ext::vulkan::device.graphicsQueue, VK_IMAGE_USAGE_SAMPLED_BIT );
			}
			for ( auto& renderMode : renderModes ) {
				if ( !renderMode ) continue;
				renderMode->initialize(device);
			}
			for ( auto& renderMode : renderModes ) {
				if ( !renderMode ) continue;
				renderMode->createCommandBuffers();
			}
		} break;
		case 1: {
			std::function<void(uf::Entity*)> filter = [&]( uf::Entity* entity ) {
				if ( !entity->hasComponent<uf::Graphic>() ) return;
				ext::vulkan::Graphic& graphic = entity->getComponent<uf::Graphic>();
				if ( graphic.initialized ) return;

				graphic.initializePipeline();
				ext::vulkan::rebuild = true;
			};
			for ( uf::Scene* scene : ext::vulkan::scenes ) {
				if ( !scene ) continue;
				scene->process(filter);
			}
		}
		case 2: {
			for ( auto& renderMode : renderModes ) {
				if ( !renderMode ) continue;
				renderMode->createCommandBuffers();
			}
		} break;
		default: {
			throw std::runtime_error("invalid stage id");
		} break;
	}
}
void ext::vulkan::tick() {
	ext::vulkan::mutex.lock();
	if ( ext::vulkan::resized ) ext::vulkan::rebuild = true;

	std::function<void(uf::Entity*)> filter = [&]( uf::Entity* entity ) {
		if ( !entity->hasComponent<uf::Graphic>() ) return;
		ext::vulkan::Graphic& graphic = entity->getComponent<uf::Graphic>();
		if ( graphic.initialized ) return;

		if ( !graphic.process ) return;
		if ( graphic.initialized ) return;
		graphic.initializePipeline();
		ext::vulkan::rebuild = true;
	};
	for ( uf::Scene* scene : ext::vulkan::scenes ) {
		if ( !scene ) continue;
		scene->process(filter);
	}
	for ( auto& renderMode : renderModes ) {
		if ( !renderMode ) continue;
		if ( !renderMode->device ) renderMode->initialize(ext::vulkan::device);
		renderMode->tick();
	}
	for ( auto& renderMode : renderModes ) {
		if ( !renderMode ) continue;
		if ( ext::vulkan::rebuild )
			renderMode->createCommandBuffers();
	}
	ext::vulkan::rebuild = false;
	ext::vulkan::resized = false;
	ext::vulkan::mutex.unlock();
}
void ext::vulkan::render() {
	if ( hasRenderMode("", true) ) {
		RenderMode& primary = getRenderMode("", true);
		auto it = std::find( renderModes.begin(), renderModes.end(), &primary );
		if ( it + 1 != renderModes.end() ) std::rotate( it, it + 1, renderModes.end() );
	}

	ext::vulkan::mutex.lock();
	for ( auto& renderMode : renderModes ) {
		if ( !renderMode ) continue;
		ext::vulkan::currentRenderMode = renderMode;
		for ( uf::Scene* scene : ext::vulkan::scenes ) scene->render();
		renderMode->render();
	}

	ext::vulkan::currentRenderMode = NULL;
	ext::vulkan::mutex.unlock();
}
void ext::vulkan::destroy() {
	ext::vulkan::mutex.lock();
	vkDeviceWaitIdle( device );

	Texture2D::empty.destroy();

	std::function<void(uf::Entity*)> filter = [&]( uf::Entity* entity ) {
		if ( !entity->hasComponent<uf::Graphic>() ) return;
		uf::Graphic& graphic = entity->getComponent<uf::Graphic>();
		graphic.destroy();
	};
	for ( uf::Scene* scene : ext::vulkan::scenes ) {
		if ( !scene ) continue;
		scene->process(filter);
	}

	for ( auto& renderMode : renderModes ) {
		if ( !renderMode ) continue;
		renderMode->destroy();
		delete renderMode;
		renderMode = NULL;
	}

	vmaDestroyAllocator( allocator );

	swapchain.destroy();
	device.destroy();
	ext::vulkan::mutex.unlock();
}

std::string ext::vulkan::allocatorStats() {
	char* statsString = nullptr;
	vmaBuildStatsString(ext::vulkan::allocator, &statsString, true);
	std::string result = statsString;
	vmaFreeStatsString(ext::vulkan::allocator, statsString);
	return result;
}