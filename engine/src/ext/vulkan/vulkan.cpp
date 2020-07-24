#include <uf/ext/glfw/glfw.h>
#include <uf/ext/vulkan/vulkan.h>
#include <uf/ext/vulkan/initializers.h>
#include <uf/ext/vulkan/rendermodes/multiview.h>
#include <uf/ext/vulkan/rendermodes/deferred.h>
#include <uf/utils/mesh/mesh.h>

#include <ostream>
#include <fstream>

uint32_t ext::vulkan::width = 1280;
uint32_t ext::vulkan::height = 720;

bool ext::vulkan::validation = true;
ext::vulkan::Device ext::vulkan::device;
ext::vulkan::Allocator ext::vulkan::allocator;
ext::vulkan::Swapchain ext::vulkan::swapchain;
std::mutex ext::vulkan::mutex;

bool ext::vulkan::rebuild = false;
uint32_t ext::vulkan::currentBuffer = 0;
std::vector<std::string> ext::vulkan::passes = { "BASE" };
std::vector<uf::Scene*> ext::vulkan::scenes;
std::string ext::vulkan::currentPass = "BASE";

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
	uf::iostream << "[Validation Layer] " << pCallbackData->pMessage << "\n";
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
ext::vulkan::RenderMode& ext::vulkan::addRenderMode( ext::vulkan::RenderMode* mode, const std::string& name ) {
	mode->name = name;
	renderModes.push_back(mode);
	return *mode;
}
ext::vulkan::RenderMode& ext::vulkan::getRenderMode( const std::string& name, bool isName ) {
	RenderMode* target = renderModes[0];
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
	return *target;
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
			for ( auto& renderMode : renderModes ) {
				if ( !renderMode ) continue;
				renderMode->initialize(device);
				std::cout << "Initialized render mode `" << renderMode->name << "` (" << renderMode->getType() << ")" << std::endl;
			}
			/* resort */ {
			/*
				for ( auto it = renderModes.begin(); it != renderModes.end(); ++it ) {
					if ( (*it)->getName() == "" ) {
					//	std::rotate( renderModes.begin(), it, renderModes.end() );
						RenderMode* target = *it;
						renderModes.erase(it);
						renderModes.push_back(target);
						break;
					}
				}
				std::cout << "Render order: ";
				for ( auto it = renderModes.begin(); it != renderModes.end(); ++it ) {
					std::cout << "`" << (*it)->getName() << "` -> ";
				}
				std::cout << std::endl;
			*/
			}
			for ( auto& renderMode : renderModes ) {
				if ( !renderMode ) continue;
				renderMode->createCommandBuffers();
			}
		} break;
	/*
		case 1: if ( ext::vulkan::graphics ) {
			auto& graphics = *ext::vulkan::graphics;
			for ( Graphic* graphic : graphics ) {
				if ( !graphic->initialized ) {
					graphic->initialize();
				}
			}
		} break;
	*/
		case 1: {
			std::function<void(uf::Entity*)> filter = [&]( uf::Entity* entity ) {
				if ( !entity->hasComponent<uf::Mesh>() ) return;
				uf::MeshBase& mesh = entity->getComponent<uf::Mesh>();
				ext::vulkan::Graphic& graphic = mesh.graphic;
				if ( !mesh.generated ) return;
				if ( graphic.initialized ) return;
				graphic.initialize();
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
std::ostream& operator<<(std::ostream& os, const ext::vulkan::Graphic& graphic) {
	os << graphic.name() << ": " << &graphic;
	return os;
}
void ext::vulkan::tick() {
	// check for changes in swapchain
	// ext::vulkan::mutex.lock();
/*
	if ( ext::vulkan::graphics ) {
		auto& graphics = *ext::vulkan::graphics;
		for ( Graphic* graphic : graphics ) {
			if ( !graphic->initialized ) {
				ext::vulkan::rebuild = true;
				graphic->initialize();
			}
		}
	}
*/
	std::function<void(uf::Entity*)> filter = [&]( uf::Entity* entity ) {
		if ( !entity->hasComponent<uf::Mesh>() ) return;
		uf::MeshBase& mesh = entity->getComponent<uf::Mesh>();
		ext::vulkan::Graphic& graphic = mesh.graphic;
		// if ( !graphic->process ) return;
		if ( !mesh.generated ) return;
		if ( graphic.initialized ) return;
		ext::vulkan::rebuild = true;
		graphic.initialize();
	};
	for ( uf::Scene* scene : ext::vulkan::scenes ) {
		if ( !scene ) continue;
		scene->process(filter);
	}
	if ( ext::vulkan::rebuild ) {
		for ( auto& renderMode : renderModes ) {
			if ( !renderMode ) continue;
			renderMode->createCommandBuffers();
		}
		ext::vulkan::rebuild = false;
	}
	// ext::vulkan::mutex.unlock();
}
void ext::vulkan::render() {
/*	
	if ( ext::vulkan::graphics ) {
		auto& graphics = *ext::vulkan::graphics;
		for ( Graphic* graphic : graphics ) {
			if ( !graphic || !graphic->process ) continue;
				graphic->render();
		}
	}
*/
/*
	std::function<void(uf::Entity*)> filter = [&]( uf::Entity* entity ) {
		if ( !entity->hasComponent<uf::Mesh>() ) return;
		uf::MeshBase& mesh = entity->getComponent<uf::Mesh>();
		ext::vulkan::Graphic& graphic = mesh.graphic;
		// if ( !graphic.process ) return;
		if ( !graphic.initialized ) return;
		graphic.render();
	};
	for ( uf::Scene* scene : ext::vulkan::scenes ) {
		if ( !scene ) continue;
		scene->process(filter);
	}
*/
	for ( auto& renderMode : renderModes ) {
		if ( !renderMode ) continue;
		renderMode->render();
	}

	// Handle resizes
	if ( ext::vulkan::rebuild ) ext::vulkan::rebuild = false;
}
void ext::vulkan::destroy() {
	vkDeviceWaitIdle( device );

	std::function<void(uf::Entity*)> filter = [&]( uf::Entity* entity ) {
		if ( !entity->hasComponent<uf::Mesh>() ) return;
		uf::MeshBase& mesh = entity->getComponent<uf::Mesh>();
		ext::vulkan::Graphic& graphic = mesh.graphic;
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
}

std::string ext::vulkan::allocatorStats() {
	char* statsString = nullptr;
	vmaBuildStatsString(ext::vulkan::allocator, &statsString, true);
	std::string result = statsString;
	vmaFreeStatsString(ext::vulkan::allocator, statsString);
	return result;
}