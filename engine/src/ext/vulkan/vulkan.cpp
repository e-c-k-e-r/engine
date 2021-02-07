#include <uf/ext/glfw/glfw.h>
#include <uf/ext/vulkan/vulkan.h>
#include <uf/ext/vulkan/initializers.h>
#include <uf/ext/vulkan/graphic.h>
#include <uf/ext/vulkan/rendermode.h>
#include <uf/utils/graphic/graphic.h>

#include <uf/ext/openvr/openvr.h>

#include <ostream>
#include <fstream>
#include <atomic>

uint32_t ext::vulkan::settings::width = 1280;
uint32_t ext::vulkan::settings::height = 720;
uint8_t ext::vulkan::settings::msaa = 1;
bool ext::vulkan::settings::validation = true;
// constexpr size_t ext::vulkan::settings::maxViews = 6;
size_t ext::vulkan::settings::viewCount = 2;

std::vector<std::string> ext::vulkan::settings::validationFilters;
std::vector<std::string> ext::vulkan::settings::requestedDeviceFeatures;
std::vector<std::string> ext::vulkan::settings::requestedDeviceExtensions;
std::vector<std::string> ext::vulkan::settings::requestedInstanceExtensions;

VkFilter ext::vulkan::settings::swapchainUpscaleFilter = VK_FILTER_LINEAR;

bool ext::vulkan::settings::experimental::rebuildOnTickBegin = false;
bool ext::vulkan::settings::experimental::waitOnRenderEnd = false;
bool ext::vulkan::settings::experimental::individualPipelines = false;
bool ext::vulkan::settings::experimental::multithreadedCommandRecording = false;
std::string ext::vulkan::settings::experimental::deferredMode = "";
bool ext::vulkan::settings::experimental::deferredReconstructPosition = false;
bool ext::vulkan::settings::experimental::deferredAliasOutputToSwapchain = true;
bool ext::vulkan::settings::experimental::multiview = true;
bool ext::vulkan::settings::experimental::hdr = true;

VkColorSpaceKHR ext::vulkan::settings::formats::colorSpace;
VkFormat ext::vulkan::settings::formats::color = VK_FORMAT_R8G8B8A8_UNORM;
VkFormat ext::vulkan::settings::formats::depth = VK_FORMAT_D32_SFLOAT;
VkFormat ext::vulkan::settings::formats::normal = VK_FORMAT_R16G16B16A16_SFLOAT;
VkFormat ext::vulkan::settings::formats::position = VK_FORMAT_R16G16B16A16_SFLOAT;

ext::vulkan::Device ext::vulkan::device;
ext::vulkan::Allocator ext::vulkan::allocator;
ext::vulkan::Swapchain ext::vulkan::swapchain;
std::mutex ext::vulkan::mutex;

bool ext::vulkan::states::resized = false;
bool ext::vulkan::states::rebuild = false;
uint32_t ext::vulkan::states::currentBuffer = 0;

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
//	if ( messageSeverity <= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT ) return VK_FALSE;
	std::string message = pCallbackData->pMessage;
	for ( auto& filter : ext::vulkan::settings::validationFilters ) {
		if ( message.find(filter) != std::string::npos ) return VK_FALSE;
	}
	uf::iostream << "[Validation Layer] " << message << "\n";
	return VK_FALSE;
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
VkSampleCountFlagBits ext::vulkan::sampleCount( uint8_t count ) {
	VkSampleCountFlags counts = std::min(ext::vulkan::device.properties.limits.framebufferColorSampleCounts, ext::vulkan::device.properties.limits.framebufferDepthSampleCounts);
	if ( count < counts ) counts = count;
	
	if ( counts & VK_SAMPLE_COUNT_64_BIT ) return VK_SAMPLE_COUNT_64_BIT;
	if ( counts & VK_SAMPLE_COUNT_32_BIT ) return VK_SAMPLE_COUNT_32_BIT;
	if ( counts & VK_SAMPLE_COUNT_16_BIT ) return VK_SAMPLE_COUNT_16_BIT;
	if ( counts & VK_SAMPLE_COUNT_8_BIT  ) return VK_SAMPLE_COUNT_8_BIT;
	if ( counts & VK_SAMPLE_COUNT_4_BIT  ) return VK_SAMPLE_COUNT_4_BIT;
	if ( counts & VK_SAMPLE_COUNT_2_BIT  ) return VK_SAMPLE_COUNT_2_BIT;
	return VK_SAMPLE_COUNT_1_BIT;
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

bool ext::vulkan::hasRenderMode( const std::string& name, bool isName ) {
	for ( auto& renderMode: ext::vulkan::renderModes ) {
		if ( isName ) {
			if ( renderMode->getName() == name ) return true;
		} else {
			if ( renderMode->getType() == name ) return true;
		}
	}
	return false;
}

ext::vulkan::RenderMode& ext::vulkan::addRenderMode( ext::vulkan::RenderMode* mode, const std::string& name ) {
	mode->metadata["name"] = name;
	renderModes.push_back(mode);
	if ( ext::vulkan::settings::validation ) uf::iostream << "Adding RenderMode: " << name << ": " << mode->getType() << "\n";
	// reorder
	ext::vulkan::states::rebuild = true;
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
//	if ( ext::vulkan::settings::validation ) uf::iostream << "Requesting RenderMode `" << name << "`, got `" << target->getName() << "` (" << target->getType() << ")" << "\n";
	return *target;
}
std::vector<ext::vulkan::RenderMode*> ext::vulkan::getRenderModes( const std::string& name, bool isName ) {
	return ext::vulkan::getRenderModes({name}, isName);
}
std::vector<ext::vulkan::RenderMode*> ext::vulkan::getRenderModes( const std::vector<std::string>& names, bool isName ) {
	std::vector<RenderMode*> targets;
	for ( auto& renderMode: renderModes ) {
		if ( ( isName && std::find(names.begin(), names.end(), renderMode->getName()) != names.end() ) || std::find(names.begin(), names.end(), renderMode->getType()) != names.end() ) {
			targets.push_back(renderMode);
//			if ( ext::vulkan::settings::validation ) uf::iostream << "Requestings RenderMode `" << name << "`, got `" << renderMode->getName() << "` (" << renderMode->getType() << ")" << "\n";
		}
	}
	return targets;
}
void ext::vulkan::removeRenderMode( ext::vulkan::RenderMode* mode, bool free ) {
	if ( !mode ) return;
	renderModes.erase( std::remove( renderModes.begin(), renderModes.end(), mode ), renderModes.end() );
	mode->destroy();
	if ( free ) delete mode;
	ext::vulkan::states::rebuild = true;
}

void ext::vulkan::initialize( uint8_t stage ) {
	switch ( stage ) {
		case 0: {
			device.initialize();
			swapchain.initialize( device );
			for ( auto& renderMode : renderModes ) {
				if ( !renderMode ) continue;
				renderMode->initialize(device);
			}
			std::vector<std::function<int()>> jobs;
			for ( auto& renderMode : renderModes ) {
				if ( !renderMode ) continue;
				if ( settings::experimental::individualPipelines ) renderMode->bindPipelines();
				if ( settings::experimental::multithreadedCommandRecording ) {
					jobs.emplace_back([&]{
						renderMode->createCommandBuffers();
						return 0;
					});
				} else {
					renderMode->createCommandBuffers();
				}
			}
			if ( !jobs.empty() ) {
				uf::thread::batchWorkers( jobs );
			}
			{
				std::vector<uint8_t> pixels = { 
					255,   0, 255, 255,      0,   0,   0, 255,
					  0,   0,   0, 255,    255,   0, 255, 255,
				};
				Texture2D::empty.sampler.descriptor.filter.min = VK_FILTER_NEAREST;
				Texture2D::empty.sampler.descriptor.filter.mag = VK_FILTER_NEAREST;
				Texture2D::empty.fromBuffers( (void*) &pixels[0], pixels.size(), VK_FORMAT_R8G8B8A8_UNORM, 2, 2, ext::vulkan::device, VK_IMAGE_USAGE_SAMPLED_BIT );
			}
		} break;
		case 1: {
			std::function<void(uf::Entity*)> filter = [&]( uf::Entity* entity ) {
				if ( !entity->hasComponent<uf::Graphic>() ) return;
				ext::vulkan::Graphic& graphic = entity->getComponent<uf::Graphic>();
				if ( graphic.initialized ) return;

				graphic.initializePipeline();
				ext::vulkan::states::rebuild = true;
			};
			for ( uf::Scene* scene : uf::scene::scenes ) {
				if ( !scene ) continue;
				scene->process(filter);
			}
		}
		case 2: {
		/*
			for ( auto& renderMode : renderModes ) {
				if ( !renderMode ) continue;
				renderMode->createCommandBuffers();
			}
		*/
			std::vector<std::function<int()>> jobs;
			for ( auto& renderMode : renderModes ) {
				if ( !renderMode ) continue;
				if ( settings::experimental::individualPipelines ) renderMode->bindPipelines();
				if ( settings::experimental::multithreadedCommandRecording ) {
					jobs.emplace_back([&]{
						renderMode->createCommandBuffers();
						return 0;
					});
				} else {
					renderMode->createCommandBuffers();
				}
			}
			if ( !jobs.empty() ) {
				uf::thread::batchWorkers( jobs );
			}
		} break;
		default: {
			throw std::runtime_error("invalid stage id");
		} break;
	}
}
void ext::vulkan::tick() {
	ext::vulkan::mutex.lock();
	if ( ext::vulkan::states::resized || ext::vulkan::settings::experimental::rebuildOnTickBegin ) {
		ext::vulkan::states::rebuild = true;
	}

	std::function<void(uf::Entity*)> filter = [&]( uf::Entity* entity ) {
		if ( !entity->hasComponent<uf::Graphic>() ) return;
		ext::vulkan::Graphic& graphic = entity->getComponent<uf::Graphic>();
		if ( graphic.initialized || !graphic.process || graphic.initialized ) return;
		graphic.initializePipeline();
		ext::vulkan::states::rebuild = true;
	};
	for ( uf::Scene* scene : uf::scene::scenes ) {
		if ( !scene ) continue;
		scene->process(filter);
	}
	for ( auto& renderMode : renderModes ) {
		if ( !renderMode ) continue;
		if ( !renderMode->device ) {
			renderMode->initialize(ext::vulkan::device);
			ext::vulkan::states::rebuild = true;
		}
		renderMode->tick();
	}

	std::vector<std::function<int()>> jobs;
	for ( auto& renderMode : renderModes ) {
		if ( !renderMode ) continue;
		if ( ext::vulkan::states::rebuild || renderMode->rebuild ) {
			if ( settings::experimental::individualPipelines ) renderMode->bindPipelines();
			if ( settings::experimental::multithreadedCommandRecording ) {
				jobs.emplace_back([&]{
					renderMode->createCommandBuffers();
					return 0;
				});
			} else {
				renderMode->createCommandBuffers();
			}
		}
	}
	if ( !jobs.empty() ) {
		uf::thread::batchWorkers( jobs );
	}
	
	ext::vulkan::states::rebuild = false;
	ext::vulkan::states::resized = false;
	ext::vulkan::mutex.unlock();
}
void ext::vulkan::render() {
	ext::vulkan::mutex.lock();
	if ( hasRenderMode("Gui", true) ) {
		RenderMode& primary = getRenderMode("Gui", true);
		auto it = std::find( renderModes.begin(), renderModes.end(), &primary );
		if ( it + 1 != renderModes.end() ) std::rotate( it, it + 1, renderModes.end() );
	}
	if ( hasRenderMode("", true) ) {
		RenderMode& primary = getRenderMode("", true);
		auto it = std::find( renderModes.begin(), renderModes.end(), &primary );
		if ( it + 1 != renderModes.end() ) std::rotate( it, it + 1, renderModes.end() );
	}

	for ( auto& renderMode : renderModes ) {
		if ( !renderMode ) continue;
		if ( !renderMode->execute ) continue;
		ext::vulkan::currentRenderMode = renderMode;
		for ( uf::Scene* scene : uf::scene::scenes ) scene->render();
		renderMode->render();
	}

	ext::vulkan::currentRenderMode = NULL;
	if ( ext::vulkan::settings::experimental::waitOnRenderEnd ) {
		synchronize();
	}
/*
	if ( ext::openvr::context ) {
		ext::openvr::postSubmit();
	}
*/
	ext::vulkan::mutex.unlock();
}
void ext::vulkan::destroy() {
	ext::vulkan::mutex.lock();
	synchronize();

	Texture2D::empty.destroy();

	std::function<void(uf::Entity*)> filter = [&]( uf::Entity* entity ) {
		if ( !entity->hasComponent<uf::Graphic>() ) return;
		uf::Graphic& graphic = entity->getComponent<uf::Graphic>();
		graphic.destroy();
	};
	for ( uf::Scene* scene : uf::scene::scenes ) {
		if ( !scene ) continue;
		scene->process(filter);
	}

	for ( auto& renderMode : renderModes ) {
		if ( !renderMode ) continue;
		renderMode->destroy();
	//	delete renderMode;
		renderMode = NULL;
	}

	swapchain.destroy();
	device.destroy();
	ext::vulkan::mutex.unlock();
}
void ext::vulkan::synchronize( uint8_t flag ) {
	if ( flag & 0b01 ) {
		for ( auto& renderMode : renderModes ) {
			if ( !renderMode ) continue;
			if ( !renderMode->execute ) continue;
			renderMode->synchronize();
		}
	}
	if ( flag & 0b10 )
		vkDeviceWaitIdle( device );
}

std::string ext::vulkan::allocatorStats() {
	char* statsString = nullptr;
	vmaBuildStatsString(ext::vulkan::allocator, &statsString, true);
	std::string result = statsString;
	vmaFreeStatsString(ext::vulkan::allocator, statsString);
	return result;
}

VkFormat ext::vulkan::formatFromString( const std::string& string ) {
	std::unordered_map<std::string,VkFormat> table = {
		{ "UNDEFINED", VK_FORMAT_UNDEFINED },
		{ "R4G4_UNORM_PACK8", VK_FORMAT_R4G4_UNORM_PACK8 },
		{ "R4G4B4A4_UNORM_PACK16", VK_FORMAT_R4G4B4A4_UNORM_PACK16 },
		{ "B4G4R4A4_UNORM_PACK16", VK_FORMAT_B4G4R4A4_UNORM_PACK16 },
		{ "R5G6B5_UNORM_PACK16", VK_FORMAT_R5G6B5_UNORM_PACK16 },
		{ "B5G6R5_UNORM_PACK16", VK_FORMAT_B5G6R5_UNORM_PACK16 },
		{ "R5G5B5A1_UNORM_PACK16", VK_FORMAT_R5G5B5A1_UNORM_PACK16 },
		{ "B5G5R5A1_UNORM_PACK16", VK_FORMAT_B5G5R5A1_UNORM_PACK16 },
		{ "A1R5G5B5_UNORM_PACK16", VK_FORMAT_A1R5G5B5_UNORM_PACK16 },
		{ "R8_UNORM", VK_FORMAT_R8_UNORM },
		{ "R8_SNORM", VK_FORMAT_R8_SNORM },
		{ "R8_USCALED", VK_FORMAT_R8_USCALED },
		{ "R8_SSCALED", VK_FORMAT_R8_SSCALED },
		{ "R8_UINT", VK_FORMAT_R8_UINT },
		{ "R8_SINT", VK_FORMAT_R8_SINT },
		{ "R8_SRGB", VK_FORMAT_R8_SRGB },
		{ "R8G8_UNORM", VK_FORMAT_R8G8_UNORM },
		{ "R8G8_SNORM", VK_FORMAT_R8G8_SNORM },
		{ "R8G8_USCALED", VK_FORMAT_R8G8_USCALED },
		{ "R8G8_SSCALED", VK_FORMAT_R8G8_SSCALED },
		{ "R8G8_UINT", VK_FORMAT_R8G8_UINT },
		{ "R8G8_SINT", VK_FORMAT_R8G8_SINT },
		{ "R8G8_SRGB", VK_FORMAT_R8G8_SRGB },
		{ "R8G8B8_UNORM", VK_FORMAT_R8G8B8_UNORM },
		{ "R8G8B8_SNORM", VK_FORMAT_R8G8B8_SNORM },
		{ "R8G8B8_USCALED", VK_FORMAT_R8G8B8_USCALED },
		{ "R8G8B8_SSCALED", VK_FORMAT_R8G8B8_SSCALED },
		{ "R8G8B8_UINT", VK_FORMAT_R8G8B8_UINT },
		{ "R8G8B8_SINT", VK_FORMAT_R8G8B8_SINT },
		{ "R8G8B8_SRGB", VK_FORMAT_R8G8B8_SRGB },
		{ "B8G8R8_UNORM", VK_FORMAT_B8G8R8_UNORM },
		{ "B8G8R8_SNORM", VK_FORMAT_B8G8R8_SNORM },
		{ "B8G8R8_USCALED", VK_FORMAT_B8G8R8_USCALED },
		{ "B8G8R8_SSCALED", VK_FORMAT_B8G8R8_SSCALED },
		{ "B8G8R8_UINT", VK_FORMAT_B8G8R8_UINT },
		{ "B8G8R8_SINT", VK_FORMAT_B8G8R8_SINT },
		{ "B8G8R8_SRGB", VK_FORMAT_B8G8R8_SRGB },
		{ "R8G8B8A8_UNORM", VK_FORMAT_R8G8B8A8_UNORM },
		{ "R8G8B8A8_SNORM", VK_FORMAT_R8G8B8A8_SNORM },
		{ "R8G8B8A8_USCALED", VK_FORMAT_R8G8B8A8_USCALED },
		{ "R8G8B8A8_SSCALED", VK_FORMAT_R8G8B8A8_SSCALED },
		{ "R8G8B8A8_UINT", VK_FORMAT_R8G8B8A8_UINT },
		{ "R8G8B8A8_SINT", VK_FORMAT_R8G8B8A8_SINT },
		{ "R8G8B8A8_SRGB", VK_FORMAT_R8G8B8A8_SRGB },
		{ "B8G8R8A8_UNORM", VK_FORMAT_B8G8R8A8_UNORM },
		{ "B8G8R8A8_SNORM", VK_FORMAT_B8G8R8A8_SNORM },
		{ "B8G8R8A8_USCALED", VK_FORMAT_B8G8R8A8_USCALED },
		{ "B8G8R8A8_SSCALED", VK_FORMAT_B8G8R8A8_SSCALED },
		{ "B8G8R8A8_UINT", VK_FORMAT_B8G8R8A8_UINT },
		{ "B8G8R8A8_SINT", VK_FORMAT_B8G8R8A8_SINT },
		{ "B8G8R8A8_SRGB", VK_FORMAT_B8G8R8A8_SRGB },
		{ "A8B8G8R8_UNORM_PACK32", VK_FORMAT_A8B8G8R8_UNORM_PACK32 },
		{ "A8B8G8R8_SNORM_PACK32", VK_FORMAT_A8B8G8R8_SNORM_PACK32 },
		{ "A8B8G8R8_USCALED_PACK32", VK_FORMAT_A8B8G8R8_USCALED_PACK32 },
		{ "A8B8G8R8_SSCALED_PACK32", VK_FORMAT_A8B8G8R8_SSCALED_PACK32 },
		{ "A8B8G8R8_UINT_PACK32", VK_FORMAT_A8B8G8R8_UINT_PACK32 },
		{ "A8B8G8R8_SINT_PACK32", VK_FORMAT_A8B8G8R8_SINT_PACK32 },
		{ "A8B8G8R8_SRGB_PACK32", VK_FORMAT_A8B8G8R8_SRGB_PACK32 },
		{ "A2R10G10B10_UNORM_PACK32", VK_FORMAT_A2R10G10B10_UNORM_PACK32 },
		{ "A2R10G10B10_SNORM_PACK32", VK_FORMAT_A2R10G10B10_SNORM_PACK32 },
		{ "A2R10G10B10_USCALED_PACK32", VK_FORMAT_A2R10G10B10_USCALED_PACK32 },
		{ "A2R10G10B10_SSCALED_PACK32", VK_FORMAT_A2R10G10B10_SSCALED_PACK32 },
		{ "A2R10G10B10_UINT_PACK32", VK_FORMAT_A2R10G10B10_UINT_PACK32 },
		{ "A2R10G10B10_SINT_PACK32", VK_FORMAT_A2R10G10B10_SINT_PACK32 },
		{ "A2B10G10R10_UNORM_PACK32", VK_FORMAT_A2B10G10R10_UNORM_PACK32 },
		{ "A2B10G10R10_SNORM_PACK32", VK_FORMAT_A2B10G10R10_SNORM_PACK32 },
		{ "A2B10G10R10_USCALED_PACK32", VK_FORMAT_A2B10G10R10_USCALED_PACK32 },
		{ "A2B10G10R10_SSCALED_PACK32", VK_FORMAT_A2B10G10R10_SSCALED_PACK32 },
		{ "A2B10G10R10_UINT_PACK32", VK_FORMAT_A2B10G10R10_UINT_PACK32 },
		{ "A2B10G10R10_SINT_PACK32", VK_FORMAT_A2B10G10R10_SINT_PACK32 },
		{ "R16_UNORM", VK_FORMAT_R16_UNORM },
		{ "R16_SNORM", VK_FORMAT_R16_SNORM },
		{ "R16_USCALED", VK_FORMAT_R16_USCALED },
		{ "R16_SSCALED", VK_FORMAT_R16_SSCALED },
		{ "R16_UINT", VK_FORMAT_R16_UINT },
		{ "R16_SINT", VK_FORMAT_R16_SINT },
		{ "R16_SFLOAT", VK_FORMAT_R16_SFLOAT },
		{ "R16G16_UNORM", VK_FORMAT_R16G16_UNORM },
		{ "R16G16_SNORM", VK_FORMAT_R16G16_SNORM },
		{ "R16G16_USCALED", VK_FORMAT_R16G16_USCALED },
		{ "R16G16_SSCALED", VK_FORMAT_R16G16_SSCALED },
		{ "R16G16_UINT", VK_FORMAT_R16G16_UINT },
		{ "R16G16_SINT", VK_FORMAT_R16G16_SINT },
		{ "R16G16_SFLOAT", VK_FORMAT_R16G16_SFLOAT },
		{ "R16G16B16_UNORM", VK_FORMAT_R16G16B16_UNORM },
		{ "R16G16B16_SNORM", VK_FORMAT_R16G16B16_SNORM },
		{ "R16G16B16_USCALED", VK_FORMAT_R16G16B16_USCALED },
		{ "R16G16B16_SSCALED", VK_FORMAT_R16G16B16_SSCALED },
		{ "R16G16B16_UINT", VK_FORMAT_R16G16B16_UINT },
		{ "R16G16B16_SINT", VK_FORMAT_R16G16B16_SINT },
		{ "R16G16B16_SFLOAT", VK_FORMAT_R16G16B16_SFLOAT },
		{ "R16G16B16A16_UNORM", VK_FORMAT_R16G16B16A16_UNORM },
		{ "R16G16B16A16_SNORM", VK_FORMAT_R16G16B16A16_SNORM },
		{ "R16G16B16A16_USCALED", VK_FORMAT_R16G16B16A16_USCALED },
		{ "R16G16B16A16_SSCALED", VK_FORMAT_R16G16B16A16_SSCALED },
		{ "R16G16B16A16_UINT", VK_FORMAT_R16G16B16A16_UINT },
		{ "R16G16B16A16_SINT", VK_FORMAT_R16G16B16A16_SINT },
		{ "R16G16B16A16_SFLOAT", VK_FORMAT_R16G16B16A16_SFLOAT },
		{ "R32_UINT", VK_FORMAT_R32_UINT },
		{ "R32_SINT", VK_FORMAT_R32_SINT },
		{ "R32_SFLOAT", VK_FORMAT_R32_SFLOAT },
		{ "R32G32_UINT", VK_FORMAT_R32G32_UINT },
		{ "R32G32_SINT", VK_FORMAT_R32G32_SINT },
		{ "R32G32_SFLOAT", VK_FORMAT_R32G32_SFLOAT },
		{ "R32G32B32_UINT", VK_FORMAT_R32G32B32_UINT },
		{ "R32G32B32_SINT", VK_FORMAT_R32G32B32_SINT },
		{ "R32G32B32_SFLOAT", VK_FORMAT_R32G32B32_SFLOAT },
		{ "R32G32B32A32_UINT", VK_FORMAT_R32G32B32A32_UINT },
		{ "R32G32B32A32_SINT", VK_FORMAT_R32G32B32A32_SINT },
		{ "R32G32B32A32_SFLOAT", VK_FORMAT_R32G32B32A32_SFLOAT },
		{ "R64_UINT", VK_FORMAT_R64_UINT },
		{ "R64_SINT", VK_FORMAT_R64_SINT },
		{ "R64_SFLOAT", VK_FORMAT_R64_SFLOAT },
		{ "R64G64_UINT", VK_FORMAT_R64G64_UINT },
		{ "R64G64_SINT", VK_FORMAT_R64G64_SINT },
		{ "R64G64_SFLOAT", VK_FORMAT_R64G64_SFLOAT },
		{ "R64G64B64_UINT", VK_FORMAT_R64G64B64_UINT },
		{ "R64G64B64_SINT", VK_FORMAT_R64G64B64_SINT },
		{ "R64G64B64_SFLOAT", VK_FORMAT_R64G64B64_SFLOAT },
		{ "R64G64B64A64_UINT", VK_FORMAT_R64G64B64A64_UINT },
		{ "R64G64B64A64_SINT", VK_FORMAT_R64G64B64A64_SINT },
		{ "R64G64B64A64_SFLOAT", VK_FORMAT_R64G64B64A64_SFLOAT },
		{ "B10G11R11_UFLOAT_PACK32", VK_FORMAT_B10G11R11_UFLOAT_PACK32 },
		{ "E5B9G9R9_UFLOAT_PACK32", VK_FORMAT_E5B9G9R9_UFLOAT_PACK32 },
		{ "D16_UNORM", VK_FORMAT_D16_UNORM },
		{ "X8_D24_UNORM_PACK32", VK_FORMAT_X8_D24_UNORM_PACK32 },
		{ "D32_SFLOAT", VK_FORMAT_D32_SFLOAT },
		{ "S8_UINT", VK_FORMAT_S8_UINT },
		{ "D16_UNORM_S8_UINT", VK_FORMAT_D16_UNORM_S8_UINT },
		{ "D24_UNORM_S8_UINT", VK_FORMAT_D24_UNORM_S8_UINT },
		{ "D32_SFLOAT_S8_UINT", VK_FORMAT_D32_SFLOAT_S8_UINT },
		{ "BC1_RGB_UNORM_BLOCK", VK_FORMAT_BC1_RGB_UNORM_BLOCK },
		{ "BC1_RGB_SRGB_BLOCK", VK_FORMAT_BC1_RGB_SRGB_BLOCK },
		{ "BC1_RGBA_UNORM_BLOCK", VK_FORMAT_BC1_RGBA_UNORM_BLOCK },
		{ "BC1_RGBA_SRGB_BLOCK", VK_FORMAT_BC1_RGBA_SRGB_BLOCK },
		{ "BC2_UNORM_BLOCK", VK_FORMAT_BC2_UNORM_BLOCK },
		{ "BC2_SRGB_BLOCK", VK_FORMAT_BC2_SRGB_BLOCK },
		{ "BC3_UNORM_BLOCK", VK_FORMAT_BC3_UNORM_BLOCK },
		{ "BC3_SRGB_BLOCK", VK_FORMAT_BC3_SRGB_BLOCK },
		{ "BC4_UNORM_BLOCK", VK_FORMAT_BC4_UNORM_BLOCK },
		{ "BC4_SNORM_BLOCK", VK_FORMAT_BC4_SNORM_BLOCK },
		{ "BC5_UNORM_BLOCK", VK_FORMAT_BC5_UNORM_BLOCK },
		{ "BC5_SNORM_BLOCK", VK_FORMAT_BC5_SNORM_BLOCK },
		{ "BC6H_UFLOAT_BLOCK", VK_FORMAT_BC6H_UFLOAT_BLOCK },
		{ "BC6H_SFLOAT_BLOCK", VK_FORMAT_BC6H_SFLOAT_BLOCK },
		{ "BC7_UNORM_BLOCK", VK_FORMAT_BC7_UNORM_BLOCK },
		{ "BC7_SRGB_BLOCK", VK_FORMAT_BC7_SRGB_BLOCK },
		{ "ETC2_R8G8B8_UNORM_BLOCK", VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK },
		{ "ETC2_R8G8B8_SRGB_BLOCK", VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK },
		{ "ETC2_R8G8B8A1_UNORM_BLOCK", VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK },
		{ "ETC2_R8G8B8A1_SRGB_BLOCK", VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK },
		{ "ETC2_R8G8B8A8_UNORM_BLOCK", VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK },
		{ "ETC2_R8G8B8A8_SRGB_BLOCK", VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK },
		{ "EAC_R11_UNORM_BLOCK", VK_FORMAT_EAC_R11_UNORM_BLOCK },
		{ "EAC_R11_SNORM_BLOCK", VK_FORMAT_EAC_R11_SNORM_BLOCK },
		{ "EAC_R11G11_UNORM_BLOCK", VK_FORMAT_EAC_R11G11_UNORM_BLOCK },
		{ "EAC_R11G11_SNORM_BLOCK", VK_FORMAT_EAC_R11G11_SNORM_BLOCK },
		{ "ASTC_4x4_UNORM_BLOCK", VK_FORMAT_ASTC_4x4_UNORM_BLOCK },
		{ "ASTC_4x4_SRGB_BLOCK", VK_FORMAT_ASTC_4x4_SRGB_BLOCK },
		{ "ASTC_5x4_UNORM_BLOCK", VK_FORMAT_ASTC_5x4_UNORM_BLOCK },
		{ "ASTC_5x4_SRGB_BLOCK", VK_FORMAT_ASTC_5x4_SRGB_BLOCK },
		{ "ASTC_5x5_UNORM_BLOCK", VK_FORMAT_ASTC_5x5_UNORM_BLOCK },
		{ "ASTC_5x5_SRGB_BLOCK", VK_FORMAT_ASTC_5x5_SRGB_BLOCK },
		{ "ASTC_6x5_UNORM_BLOCK", VK_FORMAT_ASTC_6x5_UNORM_BLOCK },
		{ "ASTC_6x5_SRGB_BLOCK", VK_FORMAT_ASTC_6x5_SRGB_BLOCK },
		{ "ASTC_6x6_UNORM_BLOCK", VK_FORMAT_ASTC_6x6_UNORM_BLOCK },
		{ "ASTC_6x6_SRGB_BLOCK", VK_FORMAT_ASTC_6x6_SRGB_BLOCK },
		{ "ASTC_8x5_UNORM_BLOCK", VK_FORMAT_ASTC_8x5_UNORM_BLOCK },
		{ "ASTC_8x5_SRGB_BLOCK", VK_FORMAT_ASTC_8x5_SRGB_BLOCK },
		{ "ASTC_8x6_UNORM_BLOCK", VK_FORMAT_ASTC_8x6_UNORM_BLOCK },
		{ "ASTC_8x6_SRGB_BLOCK", VK_FORMAT_ASTC_8x6_SRGB_BLOCK },
		{ "ASTC_8x8_UNORM_BLOCK", VK_FORMAT_ASTC_8x8_UNORM_BLOCK },
		{ "ASTC_8x8_SRGB_BLOCK", VK_FORMAT_ASTC_8x8_SRGB_BLOCK },
		{ "ASTC_10x5_UNORM_BLOCK", VK_FORMAT_ASTC_10x5_UNORM_BLOCK },
		{ "ASTC_10x5_SRGB_BLOCK", VK_FORMAT_ASTC_10x5_SRGB_BLOCK },
		{ "ASTC_10x6_UNORM_BLOCK", VK_FORMAT_ASTC_10x6_UNORM_BLOCK },
		{ "ASTC_10x6_SRGB_BLOCK", VK_FORMAT_ASTC_10x6_SRGB_BLOCK },
		{ "ASTC_10x8_UNORM_BLOCK", VK_FORMAT_ASTC_10x8_UNORM_BLOCK },
		{ "ASTC_10x8_SRGB_BLOCK", VK_FORMAT_ASTC_10x8_SRGB_BLOCK },
		{ "ASTC_10x10_UNORM_BLOCK", VK_FORMAT_ASTC_10x10_UNORM_BLOCK },
		{ "ASTC_10x10_SRGB_BLOCK", VK_FORMAT_ASTC_10x10_SRGB_BLOCK },
		{ "ASTC_12x10_UNORM_BLOCK", VK_FORMAT_ASTC_12x10_UNORM_BLOCK },
		{ "ASTC_12x10_SRGB_BLOCK", VK_FORMAT_ASTC_12x10_SRGB_BLOCK },
		{ "ASTC_12x12_UNORM_BLOCK", VK_FORMAT_ASTC_12x12_UNORM_BLOCK },
		{ "ASTC_12x12_SRGB_BLOCK", VK_FORMAT_ASTC_12x12_SRGB_BLOCK },
		{ "G8B8G8R8_422_UNORM", VK_FORMAT_G8B8G8R8_422_UNORM },
		{ "B8G8R8G8_422_UNORM", VK_FORMAT_B8G8R8G8_422_UNORM },
		{ "G8_B8_R8_3PLANE_420_UNORM", VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM },
		{ "G8_B8R8_2PLANE_420_UNORM", VK_FORMAT_G8_B8R8_2PLANE_420_UNORM },
		{ "G8_B8_R8_3PLANE_422_UNORM", VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM },
		{ "G8_B8R8_2PLANE_422_UNORM", VK_FORMAT_G8_B8R8_2PLANE_422_UNORM },
		{ "G8_B8_R8_3PLANE_444_UNORM", VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM },
		{ "R10X6_UNORM_PACK16", VK_FORMAT_R10X6_UNORM_PACK16 },
		{ "R10X6G10X6_UNORM_2PACK16", VK_FORMAT_R10X6G10X6_UNORM_2PACK16 },
		{ "R10X6G10X6B10X6A10X6_UNORM_4PACK16", VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16 },
		{ "G10X6B10X6G10X6R10X6_422_UNORM_4PACK16", VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16 },
		{ "B10X6G10X6R10X6G10X6_422_UNORM_4PACK16", VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16 },
		{ "G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16", VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16 },
		{ "G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16", VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16 },
		{ "G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16", VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16 },
		{ "G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16", VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16 },
		{ "G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16", VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16 },
		{ "R12X4_UNORM_PACK16", VK_FORMAT_R12X4_UNORM_PACK16 },
		{ "R12X4G12X4_UNORM_2PACK16", VK_FORMAT_R12X4G12X4_UNORM_2PACK16 },
		{ "R12X4G12X4B12X4A12X4_UNORM_4PACK16", VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16 },
		{ "G12X4B12X4G12X4R12X4_422_UNORM_4PACK16", VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16 },
		{ "B12X4G12X4R12X4G12X4_422_UNORM_4PACK16", VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16 },
		{ "G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16", VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16 },
		{ "G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16", VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16 },
		{ "G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16", VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16 },
		{ "G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16", VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16 },
		{ "G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16", VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16 },
		{ "G16B16G16R16_422_UNORM", VK_FORMAT_G16B16G16R16_422_UNORM },
		{ "B16G16R16G16_422_UNORM", VK_FORMAT_B16G16R16G16_422_UNORM },
		{ "G16_B16_R16_3PLANE_420_UNORM", VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM },
		{ "G16_B16R16_2PLANE_420_UNORM", VK_FORMAT_G16_B16R16_2PLANE_420_UNORM },
		{ "G16_B16_R16_3PLANE_422_UNORM", VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM },
		{ "G16_B16R16_2PLANE_422_UNORM", VK_FORMAT_G16_B16R16_2PLANE_422_UNORM },
		{ "G16_B16_R16_3PLANE_444_UNORM", VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM },
		{ "PVRTC1_2BPP_UNORM_BLOCK_IMG", VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG },
		{ "PVRTC1_4BPP_UNORM_BLOCK_IMG", VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG },
		{ "PVRTC2_2BPP_UNORM_BLOCK_IMG", VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG },
		{ "PVRTC2_4BPP_UNORM_BLOCK_IMG", VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG },
		{ "PVRTC1_2BPP_SRGB_BLOCK_IMG", VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG },
		{ "PVRTC1_4BPP_SRGB_BLOCK_IMG", VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG },
		{ "PVRTC2_2BPP_SRGB_BLOCK_IMG", VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG },
		{ "PVRTC2_4BPP_SRGB_BLOCK_IMG", VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG },
		{ "ASTC_4x4_SFLOAT_BLOCK_EXT", VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK_EXT },
		{ "ASTC_5x4_SFLOAT_BLOCK_EXT", VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK_EXT },
		{ "ASTC_5x5_SFLOAT_BLOCK_EXT", VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK_EXT },
		{ "ASTC_6x5_SFLOAT_BLOCK_EXT", VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK_EXT },
		{ "ASTC_6x6_SFLOAT_BLOCK_EXT", VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK_EXT },
		{ "ASTC_8x5_SFLOAT_BLOCK_EXT", VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK_EXT },
		{ "ASTC_8x6_SFLOAT_BLOCK_EXT", VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK_EXT },
		{ "ASTC_8x8_SFLOAT_BLOCK_EXT", VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK_EXT },
		{ "ASTC_10x5_SFLOAT_BLOCK_EXT", VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK_EXT },
		{ "ASTC_10x6_SFLOAT_BLOCK_EXT", VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK_EXT },
		{ "ASTC_10x8_SFLOAT_BLOCK_EXT", VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK_EXT },
		{ "ASTC_10x10_SFLOAT_BLOCK_EXT", VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK_EXT },
		{ "ASTC_12x10_SFLOAT_BLOCK_EXT", VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK_EXT },
		{ "ASTC_12x12_SFLOAT_BLOCK_EXT", VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK_EXT },
		{ "A4R4G4B4_UNORM_PACK16_EXT", VK_FORMAT_A4R4G4B4_UNORM_PACK16_EXT },
		{ "A4B4G4R4_UNORM_PACK16_EXT", VK_FORMAT_A4B4G4R4_UNORM_PACK16_EXT },
		{ "G8B8G8R8_422_UNORM_KHR", VK_FORMAT_G8B8G8R8_422_UNORM_KHR },
		{ "B8G8R8G8_422_UNORM_KHR", VK_FORMAT_B8G8R8G8_422_UNORM_KHR },
		{ "G8_B8_R8_3PLANE_420_UNORM_KHR", VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM_KHR },
		{ "G8_B8R8_2PLANE_420_UNORM_KHR", VK_FORMAT_G8_B8R8_2PLANE_420_UNORM_KHR },
		{ "G8_B8_R8_3PLANE_422_UNORM_KHR", VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM_KHR },
		{ "G8_B8R8_2PLANE_422_UNORM_KHR", VK_FORMAT_G8_B8R8_2PLANE_422_UNORM_KHR },
		{ "G8_B8_R8_3PLANE_444_UNORM_KHR", VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM_KHR },
		{ "R10X6_UNORM_PACK16_KHR", VK_FORMAT_R10X6_UNORM_PACK16_KHR },
		{ "R10X6G10X6_UNORM_2PACK16_KHR", VK_FORMAT_R10X6G10X6_UNORM_2PACK16_KHR },
		{ "R10X6G10X6B10X6A10X6_UNORM_4PACK16_KHR", VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16_KHR },
		{ "G10X6B10X6G10X6R10X6_422_UNORM_4PACK16_KHR", VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16_KHR },
		{ "B10X6G10X6R10X6G10X6_422_UNORM_4PACK16_KHR", VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16_KHR },
		{ "G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16_KHR", VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16_KHR },
		{ "G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16_KHR", VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16_KHR },
		{ "G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16_KHR", VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16_KHR },
		{ "G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16_KHR", VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16_KHR },
		{ "G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16_KHR", VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16_KHR },
		{ "R12X4_UNORM_PACK16_KHR", VK_FORMAT_R12X4_UNORM_PACK16_KHR },
		{ "R12X4G12X4_UNORM_2PACK16_KHR", VK_FORMAT_R12X4G12X4_UNORM_2PACK16_KHR },
		{ "R12X4G12X4B12X4A12X4_UNORM_4PACK16_KHR", VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16_KHR },
		{ "G12X4B12X4G12X4R12X4_422_UNORM_4PACK16_KHR", VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16_KHR },
		{ "B12X4G12X4R12X4G12X4_422_UNORM_4PACK16_KHR", VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16_KHR },
		{ "G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16_KHR", VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16_KHR },
		{ "G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16_KHR", VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16_KHR },
		{ "G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16_KHR", VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16_KHR },
		{ "G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16_KHR", VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16_KHR },
		{ "G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16_KHR", VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16_KHR },
		{ "G16B16G16R16_422_UNORM_KHR", VK_FORMAT_G16B16G16R16_422_UNORM_KHR },
		{ "B16G16R16G16_422_UNORM_KHR", VK_FORMAT_B16G16R16G16_422_UNORM_KHR },
		{ "G16_B16_R16_3PLANE_420_UNORM_KHR", VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM_KHR },
		{ "G16_B16R16_2PLANE_420_UNORM_KHR", VK_FORMAT_G16_B16R16_2PLANE_420_UNORM_KHR },
		{ "G16_B16_R16_3PLANE_422_UNORM_KHR", VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM_KHR },
		{ "G16_B16R16_2PLANE_422_UNORM_KHR", VK_FORMAT_G16_B16R16_2PLANE_422_UNORM_KHR },
		{ "G16_B16_R16_3PLANE_444_UNORM_KHR", VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM_KHR },
	};
	return table[string];
}