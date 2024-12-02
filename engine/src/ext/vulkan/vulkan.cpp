#if UF_USE_VULKAN

#include <uf/ext/vulkan/vulkan.h>
#include <uf/ext/vulkan/initializers.h>
#include <uf/ext/vulkan/graphic.h>
#include <uf/ext/vulkan/rendermode.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/engine/graph/graph.h>

#include <uf/ext/openvr/openvr.h>
#include <uf/ext/ffx/fsr.h>

#include <ostream>
#include <fstream>
#include <atomic>

#define VK_REGISTER_HANDLE( handle ) if ( ext::vulkan::settings::validation::checkpoints ) {\
	VK_VALIDATION_MESSAGE("Registered handle: {}: {}", TYPE_NAME(decltype(handle)), (void*) handle);\
	uf::stl::string id = cpptrace::generate_trace().to_string();\
	ext::vulkan::Resource<std::remove_cvref_t<decltype(handle)>>::add( handle, id ); \
}

namespace {
	struct {
		uf::stl::vector<VkFence> graphics;
		uf::stl::vector<VkFence> compute;
	} auxFences;
}

float ext::vulkan::settings::version = 1.3;
uint32_t ext::vulkan::settings::width = 1280;
uint32_t ext::vulkan::settings::height = 720;
uint8_t ext::vulkan::settings::msaa = 1;

bool ext::vulkan::settings::defaultStageBuffers = true;
bool ext::vulkan::settings::defaultDeferBufferDestroy = true;
bool ext::vulkan::settings::defaultCommandBufferImmediate = true;
size_t ext::vulkan::settings::defaultTimeout = UINT64_MAX; // 100000000000;

size_t ext::vulkan::settings::viewCount = 2;
size_t ext::vulkan::settings::gpuID = -1;
size_t ext::vulkan::settings::scratchBufferAlignment = 256;
size_t ext::vulkan::settings::scratchBufferInitialSize = 1024 * 1024;

VkFilter ext::vulkan::settings::swapchainUpscaleFilter = VK_FILTER_LINEAR;

bool ext::vulkan::settings::validation::enabled = false;
bool ext::vulkan::settings::validation::messages = false;
bool ext::vulkan::settings::validation::checkpoints = false;
uf::stl::vector<uf::stl::string> ext::vulkan::settings::validation::filters;

uf::stl::vector<uf::stl::string> ext::vulkan::settings::requested::deviceFeatures;
uf::stl::vector<uf::stl::string> ext::vulkan::settings::requested::deviceExtensions;
uf::stl::vector<uf::stl::string> ext::vulkan::settings::requested::instanceExtensions;
uf::Serializer ext::vulkan::settings::requested::featureChain;

// these go hand in hand for the above
bool ext::vulkan::settings::experimental::dedicatedThread = true;
bool ext::vulkan::settings::experimental::batchQueueSubmissions = true;

bool ext::vulkan::settings::experimental::rebuildOnTickBegin = false;
bool ext::vulkan::settings::experimental::enableMultiGPU = false;
bool ext::vulkan::settings::experimental::memoryBudgetBit = true;
bool ext::vulkan::settings::experimental::registerRenderMode = true;

// not so experimental
bool ext::vulkan::settings::invariant::waitOnRenderEnd = false;
bool ext::vulkan::settings::invariant::individualPipelines = true;
bool ext::vulkan::settings::invariant::multithreadedRecording = true;
bool ext::vulkan::settings::invariant::deviceAddressing = false;

uf::stl::string ext::vulkan::settings::invariant::deferredMode = "";

bool ext::vulkan::settings::invariant::multiview = true;
// pipelines
bool ext::vulkan::settings::pipelines::deferred = true;
bool ext::vulkan::settings::pipelines::vsync = true;
bool ext::vulkan::settings::pipelines::hdr = true;
bool ext::vulkan::settings::pipelines::vxgi = true;
bool ext::vulkan::settings::pipelines::culling = false;
bool ext::vulkan::settings::pipelines::bloom = false;
bool ext::vulkan::settings::pipelines::rt = false;
bool ext::vulkan::settings::pipelines::postProcess = false;
bool ext::vulkan::settings::pipelines::fsr = false;

uf::stl::string ext::vulkan::settings::pipelines::names::deferred = "deferred";
uf::stl::string ext::vulkan::settings::pipelines::names::vsync = "vsync";
uf::stl::string ext::vulkan::settings::pipelines::names::hdr = "hdr";
uf::stl::string ext::vulkan::settings::pipelines::names::vxgi = "vxgi";
uf::stl::string ext::vulkan::settings::pipelines::names::culling = "culling";
uf::stl::string ext::vulkan::settings::pipelines::names::bloom = "bloom";
uf::stl::string ext::vulkan::settings::pipelines::names::rt = "rt";
uf::stl::string ext::vulkan::settings::pipelines::names::postProcess = "postprocess";
uf::stl::string ext::vulkan::settings::pipelines::names::fsr = "fsr";

VkColorSpaceKHR ext::vulkan::settings::formats::colorSpace;
ext::vulkan::enums::Format::type_t ext::vulkan::settings::formats::color = ext::vulkan::enums::Format::R8G8B8A8_UNORM;
ext::vulkan::enums::Format::type_t ext::vulkan::settings::formats::depth = ext::vulkan::enums::Format::D32_SFLOAT;

ext::vulkan::Device ext::vulkan::device;
ext::vulkan::Allocator ext::vulkan::allocator;
ext::vulkan::Swapchain ext::vulkan::swapchain;
std::mutex ext::vulkan::mutex;

bool ext::vulkan::states::initialized = false;
bool ext::vulkan::states::resized = false;
bool ext::vulkan::states::rebuild = false;
uint32_t ext::vulkan::states::currentBuffer = 0;
uint32_t ext::vulkan::states::frameAccumulate = 0;
bool ext::vulkan::states::frameAccumulateReset = false;
uint32_t ext::vulkan::states::frameSkip = 0;

uf::stl::vector<ext::vulkan::Texture> ext::vulkan::gc::textures;

uf::ThreadUnique<ext::vulkan::RenderMode*> ext::vulkan::currentRenderMode;

ext::vulkan::Buffer ext::vulkan::scratchBuffer;

uf::stl::vector<ext::vulkan::RenderMode*> ext::vulkan::renderModes = {
	new ext::vulkan::BaseRenderMode,
};
uf::stl::unordered_map<uf::stl::string, ext::vulkan::RenderMode*> ext::vulkan::renderModesMap;

namespace {
	uf::stl::vector<ext::vulkan::RenderMode*> fetchRenderModes( bool initGraphics = false ) {
		auto& scene = uf::scene::getCurrentScene(); 
		auto/*&*/ graph = scene.getGraph();

		uf::stl::vector<uf::renderer::RenderMode*> renderModes = ext::vulkan::renderModes;
		for ( auto entity : graph ) {
			if ( entity->hasComponent<uf::renderer::RenderTargetRenderMode>() ) {
				auto& renderMode = entity->getComponent<uf::renderer::RenderTargetRenderMode>();
				if ( std::find( renderModes.begin(), renderModes.end(), &renderMode ) == renderModes.end() )
					renderModes.emplace_back(&renderMode);
			}
			if ( entity->hasComponent<uf::renderer::DeferredRenderMode>() ) {
				auto& renderMode = entity->getComponent<uf::renderer::DeferredRenderMode>();
				if ( std::find( renderModes.begin(), renderModes.end(), &renderMode ) == renderModes.end() )
					renderModes.emplace_back(&renderMode);
			}
		}

		// group by category
		uf::stl::unordered_map<uf::stl::string, uf::stl::vector<uf::renderer::RenderMode*>> renderModesMap;
		for ( auto* renderMode : renderModes ) {
			renderModesMap[renderMode->getName()].emplace_back( renderMode );
		}

		// empty
		renderModes = {};

		// order the end rendermodes in a specific way: Gui -> Deferred -> Swapchain
		uf::stl::vector<uf::renderer::RenderMode*> end;

		if ( renderModesMap.count("Gui") > 0 ) {
			end.insert( end.end(), renderModesMap["Gui"].begin(), renderModesMap["Gui"].end() );
			renderModesMap.erase("Gui");
		}
		if ( renderModesMap.count("") > 0 ) {
			end.insert( end.end(), renderModesMap[""].begin(), renderModesMap[""].end() );
			renderModesMap.erase("");
		}
		if ( renderModesMap.count("Swapchain") > 0 ) {
			end.insert( end.end(), renderModesMap["Swapchain"].begin(), renderModesMap["Swapchain"].end() );
			renderModesMap.erase("Swapchain");
		}

		for ( auto& pair : renderModesMap ) {
			renderModes.insert( renderModes.end(), pair.second.begin(), pair.second.end() );
		}
		renderModes.insert( renderModes.end(), end.begin(), end.end() );

		return renderModes;
	}
}

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
	if ( !(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) ) return VK_FALSE;

	uf::stl::string message = pCallbackData->pMessage;
	for ( auto& filter : ext::vulkan::settings::validation::filters ) {
		if ( message.find(::fmt::format("MessageID = {}", filter)) != uf::stl::string::npos ) return VK_FALSE;
	}
	UF_MSG_ERROR("[Validation Layer] {}", message);
	return VK_FALSE;
}

uf::stl::string ext::vulkan::retrieveCheckpoint( VkQueue queue ) {
	if ( !ext::vulkan::settings::validation::checkpoints || !vkGetQueueCheckpointDataNV ) return "";
	uint32_t size = 0;
	vkGetQueueCheckpointDataNV(queue, &size, 0);

	std::vector<VkCheckpointDataNV> checkpoints(size, { VK_STRUCTURE_TYPE_CHECKPOINT_DATA_NV });
	vkGetQueueCheckpointDataNV(queue, &size, checkpoints.data());

	uf::stl::vector<uf::stl::string> strings;

	for ( auto& checkpoint : checkpoints ) {
		strings.emplace_back(::fmt::format( "{} | {} | {}:\n{}", ::fmt::ptr(queue), checkpoint.stage, ::fmt::ptr(checkpoint.pCheckpointMarker), uf::checkpoint::traverse( (pod::Checkpoint*) checkpoint.pCheckpointMarker )));
	}

	return uf::string::join( strings, "\n" );
}
uf::stl::string ext::vulkan::errorString( VkResult result ) {
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
		return ::fmt::format("UNKNOWN_ERROR: {}", result);
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
bool ext::vulkan::hasRenderMode( const uf::stl::string& name, bool isName ) {
	if ( isName && ext::vulkan::renderModesMap.count(name) > 0 ) return true;
	auto renderModes = ::fetchRenderModes();
	for ( auto& renderMode: renderModes ) {
		if ( isName && renderMode->getName() == name ) return true;
		else if ( renderMode->getType() == name ) return true;
	}
	return false;
}

ext::vulkan::RenderMode& ext::vulkan::addRenderMode( ext::vulkan::RenderMode* mode, const uf::stl::string& name ) {
	mode->metadata.name = name;
	renderModesMap[name] = renderModes.emplace_back(mode);
	
	VK_VALIDATION_MESSAGE("Adding RenderMode: {} : {}", name, mode->getType());
	
/*
	// reorder
	if ( renderModesMap.count("Gui") > 0 ) {
		RenderMode& primary = *renderModesMap["Gui"]; // getRenderMode("Gui", true);
		auto it = std::find( renderModes.begin(), renderModes.end(), &primary );
		if ( it + 1 != renderModes.end() ) std::rotate( it, it + 1, renderModes.end() );
	}
	if ( renderModesMap.count("") > 0 ) {
		RenderMode& primary = *renderModesMap[""]; // getRenderMode("", true);
		auto it = std::find( renderModes.begin(), renderModes.end(), &primary );
		if ( it + 1 != renderModes.end() ) std::rotate( it, it + 1, renderModes.end() );
	}
	if ( renderModesMap.count("Swapchain") > 0 ) {
		RenderMode& primary = *renderModesMap["Swapchain"]; // getRenderMode("Swapchain", true);
		auto it = std::find( renderModes.begin(), renderModes.end(), &primary );
		if ( it + 1 != renderModes.end() ) std::rotate( it, it + 1, renderModes.end() );
	}
*/

	ext::vulkan::states::rebuild = true;
	return *mode;
}
ext::vulkan::RenderMode& ext::vulkan::getRenderMode( const uf::stl::string& name, bool isName ) {
	if ( isName && renderModesMap.count(name) > 0 ) return *renderModesMap[name];
	auto renderModes = ::fetchRenderModes();
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
//	VK_VALIDATION_MESSAGE("Requesting RenderMode `" << name << "`, got `" << target->getName() << "` (" << target->getType() << ")");
	return *target;
}
uf::stl::vector<ext::vulkan::RenderMode*> ext::vulkan::getRenderModes( const uf::stl::string& name, bool isName ) {
	if ( isName && renderModesMap.count(name) > 0 ) return { renderModesMap[name] };
	return ext::vulkan::getRenderModes(uf::stl::vector<uf::stl::string>{name}, isName);
}
uf::stl::vector<ext::vulkan::RenderMode*> ext::vulkan::getRenderModes( const uf::stl::vector<uf::stl::string>& names, bool isName ) {
	uf::stl::vector<RenderMode*> targets;
	auto renderModes = ::fetchRenderModes();
#if 1
	// this way keeps the render mode ordered as requested
	for ( auto& name : names ) {
		for ( auto& renderMode: renderModes ) {
			if ( (isName && renderMode->getName() == name) || (!isName && renderMode->getType() == name) ) {
				targets.emplace_back( renderMode );
			}
		}
	}
#else
	for ( auto& renderMode: renderModes ) {
		if ( ( isName && std::find(names.begin(), names.end(), renderMode->getName()) != names.end() ) || std::find(names.begin(), names.end(), renderMode->getType()) != names.end() ) {
			targets.push_back(renderMode);
		}
	}
#endif
	return targets;
}
void ext::vulkan::removeRenderMode( ext::vulkan::RenderMode* mode, bool free ) {
	if ( !mode ) return;
	uf::stl::string name = mode->getName();
	renderModes.erase( std::remove( renderModes.begin(), renderModes.end(), mode ), renderModes.end() );
	renderModesMap.erase( name );
	mode->destroy();
	if ( free ) delete mode;
	ext::vulkan::states::rebuild = true;
}
ext::vulkan::RenderMode* ext::vulkan::getCurrentRenderMode() {
	return getCurrentRenderMode( std::this_thread::get_id() );
}
ext::vulkan::RenderMode* ext::vulkan::getCurrentRenderMode( std::thread::id id ) {
//	bool exists = ext::vulkan::currentRenderMode.has(id);
//	auto& currentRenderMode = ext::vulkan::currentRenderMode.get(id);
//	return currentRenderMode;
	return ext::vulkan::currentRenderMode.get(id);
}
void ext::vulkan::setCurrentRenderMode( ext::vulkan::RenderMode* renderMode ) {
	return setCurrentRenderMode( renderMode, std::this_thread::get_id() );
}
void ext::vulkan::setCurrentRenderMode( ext::vulkan::RenderMode* renderMode, std::thread::id id ) {
	ext::vulkan::currentRenderMode.get(id) = renderMode;
}

namespace {
	bool skip = false;
}

void ext::vulkan::initialize( bool soft ) {
	soft = false;
//	ext::vulkan::mutex.lock();
	if ( !soft ) {
		device.initialize();
		swapchain.initialize( device );
	}

	if ( uf::renderer::settings::invariant::deviceAddressing ) {
		ext::vulkan::scratchBuffer.alignment = ext::vulkan::settings::scratchBufferAlignment;
		ext::vulkan::scratchBuffer.initialize( NULL, ext::vulkan::settings::scratchBufferInitialSize,
			uf::renderer::enums::Buffer::ACCELERATION_STRUCTURE | uf::renderer::enums::Buffer::ADDRESS | uf::renderer::enums::Buffer::STORAGE
		);
	}
	
	if ( uf::io::exists(uf::io::root + "/textures/missing.png") ) {
		uf::Image image;
		image.open(uf::io::root + "/textures/missing.png");

		Texture2D::empty.sampler.descriptor.filter.min = uf::renderer::enums::Filter::NEAREST;
		Texture2D::empty.sampler.descriptor.filter.mag = uf::renderer::enums::Filter::NEAREST;
		Texture2D::empty.loadFromImage( image );
	} else {
		// hard coded missing texture if not provided
		uf::stl::vector<uint8_t> pixels = { 
			255,   0, 255, 255,      0,   0,   0, 255,
			  0,   0,   0, 255,    255,   0, 255, 255,
		};
		Texture2D::empty.sampler.descriptor.filter.min = VK_FILTER_NEAREST;
		Texture2D::empty.sampler.descriptor.filter.mag = VK_FILTER_NEAREST;
		Texture2D::empty.fromBuffers( (void*) &pixels[0], pixels.size(), ext::vulkan::enums::Format::R8G8B8A8_UNORM, 2, 2, ext::vulkan::device, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_LAYOUT_GENERAL );
	}
	{
		uf::stl::vector<uint8_t> pixels = { 
			255,   0, 255, 255,      0,   0,   0, 255,
			  0,   0,   0, 255,    255,   0, 255, 255,

			 255,   0, 255, 255,      0,   0,   0, 255,
			  0,   0,   0, 255,    255,   0, 255, 255,
		};
		Texture3D::empty.sampler.descriptor.filter.min = VK_FILTER_NEAREST;
		Texture3D::empty.sampler.descriptor.filter.mag = VK_FILTER_NEAREST;
		Texture3D::empty.fromBuffers( (void*) &pixels[0], pixels.size(), ext::vulkan::enums::Format::R8G8B8A8_UNORM, 2, 2, 2, 1, ext::vulkan::device, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_LAYOUT_GENERAL );
	}
	{
		uf::stl::vector<uint8_t> pixels = { 
			255,   0, 255, 255,      0,   0,   0, 255,
			  0,   0,   0, 255,    255,   0, 255, 255,
			
			255,   0, 255, 255,      0,   0,   0, 255,
			  0,   0,   0, 255,    255,   0, 255, 255,
			
			255,   0, 255, 255,      0,   0,   0, 255,
			  0,   0,   0, 255,    255,   0, 255, 255,
			
			255,   0, 255, 255,      0,   0,   0, 255,
			  0,   0,   0, 255,    255,   0, 255, 255,
			
			255,   0, 255, 255,      0,   0,   0, 255,
			  0,   0,   0, 255,    255,   0, 255, 255,

			255,   0, 255, 255,      0,   0,   0, 255,
			  0,   0,   0, 255,    255,   0, 255, 255,
		};
		TextureCube::empty.sampler.descriptor.filter.min = VK_FILTER_NEAREST;
		TextureCube::empty.sampler.descriptor.filter.mag = VK_FILTER_NEAREST;
		TextureCube::empty.fromBuffers( (void*) &pixels[0], pixels.size(), ext::vulkan::enums::Format::R8G8B8A8_UNORM, 2, 2, 1, 6, ext::vulkan::device, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_LAYOUT_GENERAL );
	}

	{
		::auxFences.graphics.resize( swapchain.buffers );
		::auxFences.compute.resize( swapchain.buffers );
		
		VkFenceCreateInfo fenceCreateInfo = {};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		
		for ( auto& fence : ::auxFences.graphics ) {
			VK_CHECK_RESULT(vkCreateFence(device, &fenceCreateInfo, nullptr, &fence));
			VK_REGISTER_HANDLE( fence );
		}
		for ( auto& fence : ::auxFences.compute ) {
			VK_CHECK_RESULT(vkCreateFence(device, &fenceCreateInfo, nullptr, &fence));
			VK_REGISTER_HANDLE( fence );
		}
	}
	
	// uf::graph::initialize();

	auto renderModes = ::fetchRenderModes();
	for ( auto& renderMode : renderModes ) {
		if ( !renderMode ) continue;
		renderMode->initialize(device);
	}

	auto tasks = uf::thread::schedule( settings::invariant::multithreadedRecording );
	for ( auto& renderMode : renderModes ) { if ( !renderMode ) continue;
		tasks.queue([&]{
			if ( settings::invariant::individualPipelines ) renderMode->bindPipelines();
			renderMode->createCommandBuffers();
		});
	}
	uf::thread::execute( tasks );

#if UF_USE_FFX_FSR
	if ( settings::pipelines::fsr ) {
		ext::fsr::initialize();
	}
#endif

	ext::vulkan::states::initialized = true;
//	ext::vulkan::mutex.unlock();
}
void ext::vulkan::tick() {
//	ext::vulkan::mutex.lock();
	if ( ext::vulkan::states::resized || ext::vulkan::settings::experimental::rebuildOnTickBegin ) {
		ext::vulkan::states::rebuild = true;
		::skip = true;
	}
	
	auto renderModes = ::fetchRenderModes();
	for ( auto& renderMode : renderModes ) {
		if ( !renderMode ) continue;
		if ( renderMode->executed && !renderMode->execute ) continue;
		if ( !renderMode->device ) {
			renderMode->initialize(ext::vulkan::device);
			ext::vulkan::states::rebuild = true;
		}
		renderMode->tick();
	}

	auto& scene = uf::scene::getCurrentScene(); 
	auto/*&*/ graph = scene.getGraph();
	for ( auto entity : graph ) {
		if ( entity->hasComponent<uf::Graphic>() ) {
			ext::vulkan::Graphic& graphic = entity->getComponent<uf::Graphic>();
			if ( graphic.initialized || !graphic.process || graphic.initialized ) continue;
			graphic.initializePipeline();
			ext::vulkan::states::rebuild = true;
		}
	}

	auto tasks = uf::thread::schedule( settings::invariant::multithreadedRecording );
	for ( auto& renderMode : renderModes ) { if ( !renderMode || (renderMode->executed && !renderMode->execute) ) continue;
		if ( ext::vulkan::states::rebuild || renderMode->rebuild ) tasks.queue([&]{
			if ( settings::invariant::individualPipelines ) renderMode->bindPipelines();
			renderMode->createCommandBuffers();
		});
		else if ( renderMode->rerecord ) tasks.queue([&]{
			renderMode->createCommandBuffers();
		});
	} 

	uf::thread::execute( tasks );

/*
	for ( auto& texture : ext::vulkan::gc::textures ) {
		texture.destroy( false );
	}
	ext::vulkan::gc::textures.clear();
*/
	
	ext::vulkan::states::rebuild = false;
	ext::vulkan::states::resized = false;
	
//	ext::vulkan::flushCommandBuffers();
//	ext::vulkan::mutex.unlock();
}
void ext::vulkan::render() {
//	ext::vulkan::mutex.lock();
	ext::vulkan::flushCommandBuffers();

	if ( ::skip ) {
		::skip = false;
		return;
	}

	// return;
/*
	if ( ext::vulkan::states::frameSkip ) {
		--ext::vulkan::states::frameSkip;
		ext::vulkan::states::rebuild = true;
		return;
	}
*/

	ext::vulkan::mutex.lock();
	auto transient = std::move(device.transient);
	ext::vulkan::mutex.unlock();

	auto renderModes = ::fetchRenderModes();
	if ( settings::experimental::batchQueueSubmissions ) {
		uf::stl::vector<VkFence> fences = { ::auxFences.compute[states::currentBuffer], ::auxFences.graphics[states::currentBuffer] };
		uf::stl::vector<RenderMode*> auxRenderModes; auxRenderModes.reserve( renderModes.size() );
		uf::stl::vector<RenderMode*> specialRenderModes; specialRenderModes.reserve( renderModes.size() );

		for ( auto renderMode : renderModes ) {
			if ( !renderMode || !renderMode->execute || !renderMode->metadata.limiter.execute ) continue;
			if ( renderMode->commandBufferCallbacks.count(RenderMode::EXECUTE_BEGIN) > 0 ) renderMode->commandBufferCallbacks[RenderMode::EXECUTE_BEGIN]( VkCommandBuffer{}, 0 );

			if ( renderMode->getName() == "" || renderMode->getName() == "Swapchain" ) specialRenderModes.emplace_back(renderMode);
			else auxRenderModes.emplace_back(renderMode);
		}
		
		uf::stl::vector<VkSubmitInfo> submitsGraphics; submitsGraphics.reserve( auxRenderModes.size() );
		uf::stl::vector<VkSubmitInfo> submitsCompute; submitsCompute.reserve( auxRenderModes.size() );

		// stuff we can batch
//		auto tasks = uf::thread::schedule( settings::invariant::multithreadedRecording );
		for ( auto renderMode : auxRenderModes ) {
			auto submitInfo = renderMode->queue();
			if ( submitInfo.sType != VK_STRUCTURE_TYPE_SUBMIT_INFO ) continue;
			if ( renderMode->metadata.compute ) submitsCompute.emplace_back(submitInfo);
			else submitsGraphics.emplace_back(submitInfo);
			renderMode->executed = true;

//			tasks.queue([&]{
				ext::vulkan::setCurrentRenderMode(renderMode);
				uf::scene::render();
				ext::vulkan::setCurrentRenderMode(NULL);
//			});
		}
//		uf::thread::execute( tasks );

//		ext::vulkan::flushCommandBuffers();
		
		VK_CHECK_RESULT(vkWaitForFences(device, fences.size(), fences.data(), VK_TRUE, VK_DEFAULT_FENCE_TIMEOUT));
		VK_CHECK_RESULT(vkResetFences(device, fences.size(), fences.data()));

		VK_CHECK_RESULT(vkQueueSubmit(device.getQueue( QueueEnum::COMPUTE ), submitsCompute.size(), submitsCompute.data(), ::auxFences.compute[states::currentBuffer]));
		VK_CHECK_RESULT(vkQueueSubmit(device.getQueue( QueueEnum::GRAPHICS ), submitsGraphics.size(), submitsGraphics.data(), ::auxFences.graphics[states::currentBuffer]));


		// stuff we can't batch
		for ( auto renderMode : specialRenderModes ) {
			ext::vulkan::setCurrentRenderMode(renderMode);
			uf::scene::render();
		#if UF_USE_FFX_FSR
			if ( renderMode->getName() == "Swapchain" && settings::pipelines::fsr && ext::fsr::initialized ) {
				ext::fsr::tick();
				ext::fsr::render();
			}
		#endif
			renderMode->render();
			ext::vulkan::setCurrentRenderMode(NULL);
		}


		for ( auto renderMode : renderModes ) {
			if ( renderMode->commandBufferCallbacks.count(RenderMode::EXECUTE_END) > 0 ) renderMode->commandBufferCallbacks[RenderMode::EXECUTE_END]( VkCommandBuffer{}, 0 );
		}
	} else {
		for ( auto& renderMode : renderModes ) {
			if ( !renderMode || !renderMode->execute || !renderMode->metadata.limiter.execute ) continue;

		#if UF_USE_FFX_FSR
			if ( renderMode->getName() == "Swapchain" && settings::pipelines::fsr && ext::fsr::initialized ) {
				ext::fsr::tick();
				ext::fsr::render();
			}
		#endif

			ext::vulkan::setCurrentRenderMode(renderMode);
			uf::graph::render();
			uf::scene::render();
			renderMode->render();
			ext::vulkan::setCurrentRenderMode(NULL);
	
		}
	}

	if ( ext::vulkan::settings::invariant::waitOnRenderEnd ) synchronize();
//	if ( ext::openvr::context ) ext::openvr::postSubmit();

//	ext::vulkan::mutex.unlock();

	// cleanup in-flight buffers
	for ( auto& buffer : transient.buffers ) buffer.destroy(false);
	transient.buffers.clear();
	
	for ( auto& as : transient.ass ) {
		uf::renderer::vkDestroyAccelerationStructureKHR(device, as.handle, nullptr);
		VK_UNREGISTER_HANDLE( as.handle );
	}
	transient.ass.clear();
}
void ext::vulkan::destroy( bool soft ) {
	soft = false;
	ext::vulkan::flushCommandBuffers();

//	ext::vulkan::mutex.lock();
	synchronize();

#if UF_USE_FFX_FSR
	if ( settings::pipelines::fsr ) {
		ext::fsr::terminate();
	}
#endif

	auto renderModes = ::fetchRenderModes();
	for ( auto& renderMode : renderModes ) {
		if ( !renderMode || !renderMode->device ) continue;
		renderMode->destroy();
		delete renderMode;
		renderMode = NULL;
	}

	ext::vulkan::renderModes.clear();
	renderModesMap.clear();

	Texture2D::empty.destroy();
	Texture3D::empty.destroy();
	TextureCube::empty.destroy();
	for ( auto& s : ext::vulkan::Sampler::samplers ) s.destroy();

	for ( auto& fence : ::auxFences.graphics ) {
		vkDestroyFence( device, fence, nullptr);
		VK_UNREGISTER_HANDLE( fence );
	}
	for ( auto& fence : ::auxFences.compute ) {
		vkDestroyFence( device, fence, nullptr);
		VK_UNREGISTER_HANDLE( fence );
	}

	// uf::graph::destroy();

	ext::vulkan::scratchBuffer.destroy();

	if ( soft ) return;

	swapchain.destroy();
	device.destroy();

	ext::vulkan::allocator = {};
	ext::vulkan::swapchain = {};
	ext::vulkan::device = {};
//	ext::vulkan::mutex.unlock();

	// check for any leaked resources
	if ( false ) {
		UF_MSG_DEBUG("Leaked resources:");

		for ( auto& resource : ext::vulkan::Resource<VkBuffer_T*>::handles ) {
			auto name = TYPE_NAME(VkBuffer_T*);
			UF_MSG_DEBUG("Orphaned resource: {}: {} (owned by: {})", name, (void*) resource.handle, resource.id );
		}
		for ( auto& resource : ext::vulkan::Resource<VmaAllocator_T*>::handles ) {
			auto name = TYPE_NAME(VmaAllocator_T*);
			UF_MSG_DEBUG("Orphaned resource: {}: {} (owned by: {})", name, (void*) resource.handle, resource.id );
		}
		for ( auto& resource : ext::vulkan::Resource<VkInstance_T*>::handles ) {
			auto name = TYPE_NAME(VkInstance_T*);
			UF_MSG_DEBUG("Orphaned resource: {}: {} (owned by: {})", name, (void*) resource.handle, resource.id );
		}
		for ( auto& resource : ext::vulkan::Resource<VkSurfaceKHR_T*>::handles ) {
			auto name = TYPE_NAME(VkSurfaceKHR_T*);
			UF_MSG_DEBUG("Orphaned resource: {}: {} (owned by: {})", name, (void*) resource.handle, resource.id );
		}
		for ( auto& resource : ext::vulkan::Resource<VkDevice_T*>::handles ) {
			auto name = TYPE_NAME(VkDevice_T*);
			UF_MSG_DEBUG("Orphaned resource: {}: {} (owned by: {})", name, (void*) resource.handle, resource.id );
		}
		for ( auto& resource : ext::vulkan::Resource<VkPipelineCache_T*>::handles ) {
			auto name = TYPE_NAME(VkPipelineCache_T*);
			UF_MSG_DEBUG("Orphaned resource: {}: {} (owned by: {})", name, (void*) resource.handle, resource.id );
		}
		for ( auto& resource : ext::vulkan::Resource<VkCommandBuffer_T*>::handles ) {
			auto name = TYPE_NAME(VkCommandBuffer_T*);
			UF_MSG_DEBUG("Orphaned resource: {}: {} (owned by: {})", name, (void*) resource.handle, resource.id );
		}
		for ( auto& resource : ext::vulkan::Resource<VkCommandPool_T*>::handles ) {
			auto name = TYPE_NAME(VkCommandPool_T*);
			UF_MSG_DEBUG("Orphaned resource: {}: {} (owned by: {})", name, (void*) resource.handle, resource.id );
		}
		for ( auto& resource : ext::vulkan::Resource<VkFence_T*>::handles ) {
			auto name = TYPE_NAME(VkFence_T*);
			UF_MSG_DEBUG("Orphaned resource: {}: {} (owned by: {})", name, (void*) resource.handle, resource.id );
		}
		for ( auto& resource : ext::vulkan::Resource<VkAccelerationStructureKHR_T*>::handles ) {
			auto name = TYPE_NAME(VkAccelerationStructureKHR_T*);
			UF_MSG_DEBUG("Orphaned resource: {}: {} (owned by: {})", name, (void*) resource.handle, resource.id );
		}
		for ( auto& resource : ext::vulkan::Resource<VkQueryPool_T*>::handles ) {
			auto name = TYPE_NAME(VkQueryPool_T*);
			UF_MSG_DEBUG("Orphaned resource: {}: {} (owned by: {})", name, (void*) resource.handle, resource.id );
		}
		for ( auto& resource : ext::vulkan::Resource<VkDescriptorSetLayout_T*>::handles ) {
			auto name = TYPE_NAME(VkDescriptorSetLayout_T*);
			UF_MSG_DEBUG("Orphaned resource: {}: {} (owned by: {})", name, (void*) resource.handle, resource.id );
		}
		for ( auto& resource : ext::vulkan::Resource<VkPipeline_T*>::handles ) {
			auto name = TYPE_NAME(VkPipeline_T*);
			UF_MSG_DEBUG("Orphaned resource: {}: {} (owned by: {})", name, (void*) resource.handle, resource.id );
		}
		for ( auto& resource : ext::vulkan::Resource<VkPipelineLayout_T*>::handles ) {
			auto name = TYPE_NAME(VkPipelineLayout_T*);
			UF_MSG_DEBUG("Orphaned resource: {}: {} (owned by: {})", name, (void*) resource.handle, resource.id );
		}
		for ( auto& resource : ext::vulkan::Resource<VkDescriptorPool_T*>::handles ) {
			auto name = TYPE_NAME(VkDescriptorPool_T*);
			UF_MSG_DEBUG("Orphaned resource: {}: {} (owned by: {})", name, (void*) resource.handle, resource.id );
		}
		for ( auto& resource : ext::vulkan::Resource<VkSemaphore_T*>::handles ) {
			auto name = TYPE_NAME(VkSemaphore_T*);
			UF_MSG_DEBUG("Orphaned resource: {}: {} (owned by: {})", name, (void*) resource.handle, resource.id );
		}
		for ( auto& resource : ext::vulkan::Resource<VkImage_T*>::handles ) {
			auto name = TYPE_NAME(VkImage_T*);
			UF_MSG_DEBUG("Orphaned resource: {}: {} (owned by: {})", name, (void*) resource.handle, resource.id );
		}
		for ( auto& resource : ext::vulkan::Resource<VkFramebuffer_T*>::handles ) {
			auto name = TYPE_NAME(VkFramebuffer_T*);
			UF_MSG_DEBUG("Orphaned resource: {}: {} (owned by: {})", name, (void*) resource.handle, resource.id );
		}
		for ( auto& resource : ext::vulkan::Resource<VkRenderPass_T*>::handles ) {
			auto name = TYPE_NAME(VkRenderPass_T*);
			UF_MSG_DEBUG("Orphaned resource: {}: {} (owned by: {})", name, (void*) resource.handle, resource.id );
		}
		for ( auto& resource : ext::vulkan::Resource<VkImageView_T*>::handles ) {
			auto name = TYPE_NAME(VkImageView_T*);
			UF_MSG_DEBUG("Orphaned resource: {}: {} (owned by: {})", name, (void*) resource.handle, resource.id );
		}
		for ( auto& resource : ext::vulkan::Resource<VkShaderModule_T*>::handles ) {
			auto name = TYPE_NAME(VkShaderModule_T*);
			UF_MSG_DEBUG("Orphaned resource: {}: {} (owned by: {})", name, (void*) resource.handle, resource.id );
		}
		for ( auto& resource : ext::vulkan::Resource<VkSwapchainKHR_T*>::handles ) {
			auto name = TYPE_NAME(VkSwapchainKHR_T*);
			UF_MSG_DEBUG("Orphaned resource: {}: {} (owned by: {})", name, (void*) resource.handle, resource.id );
		}
		for ( auto& resource : ext::vulkan::Resource<VkSampler_T*>::handles ) {
			auto name = TYPE_NAME(VkSampler_T*);
			UF_MSG_DEBUG("Orphaned resource: {}: {} (owned by: {})", name, (void*) resource.handle, resource.id );
		}
	}
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
void ext::vulkan::flushCommandBuffers() {
	// cleanup in-flight commands
	ext::vulkan::mutex.lock();
	auto transientCommandBuffers = std::move(device.transient.commandBuffers);
	ext::vulkan::mutex.unlock();

	for ( auto& pair : transientCommandBuffers ) {
		auto queueType = pair.first;
		auto& commandBuffers = pair.second;
		
		for ( auto& pair : commandBuffers ) {
			auto threadId = pair.first;
			auto& tuple = pair.second;
		
			auto queue = device.getQueue( queueType, threadId );

			// AMD driver has a limitation on how many fences to wait on at once
			constexpr size_t totalFences = 64;
			if ( tuple.fences.size() < totalFences ) {
				VkResult res = vkWaitForFences( device, tuple.fences.size(), tuple.fences.data(), VK_TRUE, VK_DEFAULT_FENCE_TIMEOUT );
				VK_CHECK_QUEUE_CHECKPOINT( queue, res );
			} else {
				auto it = tuple.fences.begin();
				auto end = tuple.fences.end();
				while ( it != end ) {
					size_t advance = MIN(end - it, totalFences);
					VkResult res = vkWaitForFences( device, advance, &(*it), VK_TRUE, VK_DEFAULT_FENCE_TIMEOUT );
					VK_CHECK_QUEUE_CHECKPOINT( queue, res );
					it += advance;
				}
			}

			for ( auto& commandBuffer : tuple.commandBuffers ) {
				uf::checkpoint::deallocate(device.checkpoints[commandBuffer]);
				device.checkpoints[commandBuffer] = NULL;
				device.checkpoints.erase(commandBuffer);
			}
			for ( auto& fence : tuple.fences ) {
				device.destroyFence( fence );
			}

			auto commandPool = device.getCommandPool( queueType, threadId );
			vkFreeCommandBuffers(device, commandPool, tuple.commandBuffers.size(), tuple.commandBuffers.data());
		}

		commandBuffers.clear();
	}
#if 0
	// process reusable commands
	{
		for ( auto& pair : device.reusable.commandBuffers ) {
			auto queueType = pair.first;
			auto& cb = pair.second;
			auto& commandBuffer = cb.commandBuffer;

			auto queue = device.getQueue( queueType );
			auto commandPool = device.getCommandPool( queueType );

			VK_CHECK_RESULT(vkQueueWaitIdle( queue ));
			vkResetCommandBuffer(device, commandPool, 1, &commandBuffer);
			cb.state = 0;
		}
	}
#endif
}

uf::stl::string ext::vulkan::allocatorStats() {
	char* statsString = nullptr;
	vmaBuildStatsString(ext::vulkan::allocator, &statsString, true);
	uf::stl::string result = statsString;
	vmaFreeStatsString(ext::vulkan::allocator, statsString);
	return result;
}

ext::vulkan::enums::Format::type_t ext::vulkan::formatFromString( const uf::stl::string& string ) {
	uf::stl::unordered_map<uf::stl::string,ext::vulkan::enums::Format::type_t> table = {
		{ "UNDEFINED", ext::vulkan::enums::Format::UNDEFINED },
		{ "R4G4_UNORM_PACK8", ext::vulkan::enums::Format::R4G4_UNORM_PACK8 },
		{ "R4G4B4A4_UNORM_PACK16", ext::vulkan::enums::Format::R4G4B4A4_UNORM_PACK16 },
		{ "B4G4R4A4_UNORM_PACK16", ext::vulkan::enums::Format::B4G4R4A4_UNORM_PACK16 },
		{ "R5G6B5_UNORM_PACK16", ext::vulkan::enums::Format::R5G6B5_UNORM_PACK16 },
		{ "B5G6R5_UNORM_PACK16", ext::vulkan::enums::Format::B5G6R5_UNORM_PACK16 },
		{ "R5G5B5A1_UNORM_PACK16", ext::vulkan::enums::Format::R5G5B5A1_UNORM_PACK16 },
		{ "B5G5R5A1_UNORM_PACK16", ext::vulkan::enums::Format::B5G5R5A1_UNORM_PACK16 },
		{ "A1R5G5B5_UNORM_PACK16", ext::vulkan::enums::Format::A1R5G5B5_UNORM_PACK16 },
		{ "R8_UNORM", ext::vulkan::enums::Format::R8_UNORM },
		{ "R8_SNORM", ext::vulkan::enums::Format::R8_SNORM },
		{ "R8_USCALED", ext::vulkan::enums::Format::R8_USCALED },
		{ "R8_SSCALED", ext::vulkan::enums::Format::R8_SSCALED },
		{ "R8_UINT", ext::vulkan::enums::Format::R8_UINT },
		{ "R8_SINT", ext::vulkan::enums::Format::R8_SINT },
		{ "R8_SRGB", ext::vulkan::enums::Format::R8_SRGB },
		{ "R8G8_UNORM", ext::vulkan::enums::Format::R8G8_UNORM },
		{ "R8G8_SNORM", ext::vulkan::enums::Format::R8G8_SNORM },
		{ "R8G8_USCALED", ext::vulkan::enums::Format::R8G8_USCALED },
		{ "R8G8_SSCALED", ext::vulkan::enums::Format::R8G8_SSCALED },
		{ "R8G8_UINT", ext::vulkan::enums::Format::R8G8_UINT },
		{ "R8G8_SINT", ext::vulkan::enums::Format::R8G8_SINT },
		{ "R8G8_SRGB", ext::vulkan::enums::Format::R8G8_SRGB },
		{ "R8G8B8_UNORM", ext::vulkan::enums::Format::R8G8B8_UNORM },
		{ "R8G8B8_SNORM", ext::vulkan::enums::Format::R8G8B8_SNORM },
		{ "R8G8B8_USCALED", ext::vulkan::enums::Format::R8G8B8_USCALED },
		{ "R8G8B8_SSCALED", ext::vulkan::enums::Format::R8G8B8_SSCALED },
		{ "R8G8B8_UINT", ext::vulkan::enums::Format::R8G8B8_UINT },
		{ "R8G8B8_SINT", ext::vulkan::enums::Format::R8G8B8_SINT },
		{ "R8G8B8_SRGB", ext::vulkan::enums::Format::R8G8B8_SRGB },
		{ "B8G8R8_UNORM", ext::vulkan::enums::Format::B8G8R8_UNORM },
		{ "B8G8R8_SNORM", ext::vulkan::enums::Format::B8G8R8_SNORM },
		{ "B8G8R8_USCALED", ext::vulkan::enums::Format::B8G8R8_USCALED },
		{ "B8G8R8_SSCALED", ext::vulkan::enums::Format::B8G8R8_SSCALED },
		{ "B8G8R8_UINT", ext::vulkan::enums::Format::B8G8R8_UINT },
		{ "B8G8R8_SINT", ext::vulkan::enums::Format::B8G8R8_SINT },
		{ "B8G8R8_SRGB", ext::vulkan::enums::Format::B8G8R8_SRGB },
		{ "R8G8B8A8_UNORM", ext::vulkan::enums::Format::R8G8B8A8_UNORM },
		{ "R8G8B8A8_SNORM", ext::vulkan::enums::Format::R8G8B8A8_SNORM },
		{ "R8G8B8A8_USCALED", ext::vulkan::enums::Format::R8G8B8A8_USCALED },
		{ "R8G8B8A8_SSCALED", ext::vulkan::enums::Format::R8G8B8A8_SSCALED },
		{ "R8G8B8A8_UINT", ext::vulkan::enums::Format::R8G8B8A8_UINT },
		{ "R8G8B8A8_SINT", ext::vulkan::enums::Format::R8G8B8A8_SINT },
		{ "R8G8B8A8_SRGB", ext::vulkan::enums::Format::R8G8B8A8_SRGB },
		{ "B8G8R8A8_UNORM", ext::vulkan::enums::Format::B8G8R8A8_UNORM },
		{ "B8G8R8A8_SNORM", ext::vulkan::enums::Format::B8G8R8A8_SNORM },
		{ "B8G8R8A8_USCALED", ext::vulkan::enums::Format::B8G8R8A8_USCALED },
		{ "B8G8R8A8_SSCALED", ext::vulkan::enums::Format::B8G8R8A8_SSCALED },
		{ "B8G8R8A8_UINT", ext::vulkan::enums::Format::B8G8R8A8_UINT },
		{ "B8G8R8A8_SINT", ext::vulkan::enums::Format::B8G8R8A8_SINT },
		{ "B8G8R8A8_SRGB", ext::vulkan::enums::Format::B8G8R8A8_SRGB },
		{ "A8B8G8R8_UNORM_PACK32", ext::vulkan::enums::Format::A8B8G8R8_UNORM_PACK32 },
		{ "A8B8G8R8_SNORM_PACK32", ext::vulkan::enums::Format::A8B8G8R8_SNORM_PACK32 },
		{ "A8B8G8R8_USCALED_PACK32", ext::vulkan::enums::Format::A8B8G8R8_USCALED_PACK32 },
		{ "A8B8G8R8_SSCALED_PACK32", ext::vulkan::enums::Format::A8B8G8R8_SSCALED_PACK32 },
		{ "A8B8G8R8_UINT_PACK32", ext::vulkan::enums::Format::A8B8G8R8_UINT_PACK32 },
		{ "A8B8G8R8_SINT_PACK32", ext::vulkan::enums::Format::A8B8G8R8_SINT_PACK32 },
		{ "A8B8G8R8_SRGB_PACK32", ext::vulkan::enums::Format::A8B8G8R8_SRGB_PACK32 },
		{ "A2R10G10B10_UNORM_PACK32", ext::vulkan::enums::Format::A2R10G10B10_UNORM_PACK32 },
		{ "A2R10G10B10_SNORM_PACK32", ext::vulkan::enums::Format::A2R10G10B10_SNORM_PACK32 },
		{ "A2R10G10B10_USCALED_PACK32", ext::vulkan::enums::Format::A2R10G10B10_USCALED_PACK32 },
		{ "A2R10G10B10_SSCALED_PACK32", ext::vulkan::enums::Format::A2R10G10B10_SSCALED_PACK32 },
		{ "A2R10G10B10_UINT_PACK32", ext::vulkan::enums::Format::A2R10G10B10_UINT_PACK32 },
		{ "A2R10G10B10_SINT_PACK32", ext::vulkan::enums::Format::A2R10G10B10_SINT_PACK32 },
		{ "A2B10G10R10_UNORM_PACK32", ext::vulkan::enums::Format::A2B10G10R10_UNORM_PACK32 },
		{ "A2B10G10R10_SNORM_PACK32", ext::vulkan::enums::Format::A2B10G10R10_SNORM_PACK32 },
		{ "A2B10G10R10_USCALED_PACK32", ext::vulkan::enums::Format::A2B10G10R10_USCALED_PACK32 },
		{ "A2B10G10R10_SSCALED_PACK32", ext::vulkan::enums::Format::A2B10G10R10_SSCALED_PACK32 },
		{ "A2B10G10R10_UINT_PACK32", ext::vulkan::enums::Format::A2B10G10R10_UINT_PACK32 },
		{ "A2B10G10R10_SINT_PACK32", ext::vulkan::enums::Format::A2B10G10R10_SINT_PACK32 },
		{ "R16_UNORM", ext::vulkan::enums::Format::R16_UNORM },
		{ "R16_SNORM", ext::vulkan::enums::Format::R16_SNORM },
		{ "R16_USCALED", ext::vulkan::enums::Format::R16_USCALED },
		{ "R16_SSCALED", ext::vulkan::enums::Format::R16_SSCALED },
		{ "R16_UINT", ext::vulkan::enums::Format::R16_UINT },
		{ "R16_SINT", ext::vulkan::enums::Format::R16_SINT },
		{ "R16_SFLOAT", ext::vulkan::enums::Format::R16_SFLOAT },
		{ "R16G16_UNORM", ext::vulkan::enums::Format::R16G16_UNORM },
		{ "R16G16_SNORM", ext::vulkan::enums::Format::R16G16_SNORM },
		{ "R16G16_USCALED", ext::vulkan::enums::Format::R16G16_USCALED },
		{ "R16G16_SSCALED", ext::vulkan::enums::Format::R16G16_SSCALED },
		{ "R16G16_UINT", ext::vulkan::enums::Format::R16G16_UINT },
		{ "R16G16_SINT", ext::vulkan::enums::Format::R16G16_SINT },
		{ "R16G16_SFLOAT", ext::vulkan::enums::Format::R16G16_SFLOAT },
		{ "R16G16B16_UNORM", ext::vulkan::enums::Format::R16G16B16_UNORM },
		{ "R16G16B16_SNORM", ext::vulkan::enums::Format::R16G16B16_SNORM },
		{ "R16G16B16_USCALED", ext::vulkan::enums::Format::R16G16B16_USCALED },
		{ "R16G16B16_SSCALED", ext::vulkan::enums::Format::R16G16B16_SSCALED },
		{ "R16G16B16_UINT", ext::vulkan::enums::Format::R16G16B16_UINT },
		{ "R16G16B16_SINT", ext::vulkan::enums::Format::R16G16B16_SINT },
		{ "R16G16B16_SFLOAT", ext::vulkan::enums::Format::R16G16B16_SFLOAT },
		{ "R16G16B16A16_UNORM", ext::vulkan::enums::Format::R16G16B16A16_UNORM },
		{ "R16G16B16A16_SNORM", ext::vulkan::enums::Format::R16G16B16A16_SNORM },
		{ "R16G16B16A16_USCALED", ext::vulkan::enums::Format::R16G16B16A16_USCALED },
		{ "R16G16B16A16_SSCALED", ext::vulkan::enums::Format::R16G16B16A16_SSCALED },
		{ "R16G16B16A16_UINT", ext::vulkan::enums::Format::R16G16B16A16_UINT },
		{ "R16G16B16A16_SINT", ext::vulkan::enums::Format::R16G16B16A16_SINT },
		{ "R16G16B16A16_SFLOAT", ext::vulkan::enums::Format::R16G16B16A16_SFLOAT },
		{ "R32_UINT", ext::vulkan::enums::Format::R32_UINT },
		{ "R32_SINT", ext::vulkan::enums::Format::R32_SINT },
		{ "R32_SFLOAT", ext::vulkan::enums::Format::R32_SFLOAT },
		{ "R32G32_UINT", ext::vulkan::enums::Format::R32G32_UINT },
		{ "R32G32_SINT", ext::vulkan::enums::Format::R32G32_SINT },
		{ "R32G32_SFLOAT", ext::vulkan::enums::Format::R32G32_SFLOAT },
		{ "R32G32B32_UINT", ext::vulkan::enums::Format::R32G32B32_UINT },
		{ "R32G32B32_SINT", ext::vulkan::enums::Format::R32G32B32_SINT },
		{ "R32G32B32_SFLOAT", ext::vulkan::enums::Format::R32G32B32_SFLOAT },
		{ "R32G32B32A32_UINT", ext::vulkan::enums::Format::R32G32B32A32_UINT },
		{ "R32G32B32A32_SINT", ext::vulkan::enums::Format::R32G32B32A32_SINT },
		{ "R32G32B32A32_SFLOAT", ext::vulkan::enums::Format::R32G32B32A32_SFLOAT },
		{ "R64_UINT", ext::vulkan::enums::Format::R64_UINT },
		{ "R64_SINT", ext::vulkan::enums::Format::R64_SINT },
		{ "R64_SFLOAT", ext::vulkan::enums::Format::R64_SFLOAT },
		{ "R64G64_UINT", ext::vulkan::enums::Format::R64G64_UINT },
		{ "R64G64_SINT", ext::vulkan::enums::Format::R64G64_SINT },
		{ "R64G64_SFLOAT", ext::vulkan::enums::Format::R64G64_SFLOAT },
		{ "R64G64B64_UINT", ext::vulkan::enums::Format::R64G64B64_UINT },
		{ "R64G64B64_SINT", ext::vulkan::enums::Format::R64G64B64_SINT },
		{ "R64G64B64_SFLOAT", ext::vulkan::enums::Format::R64G64B64_SFLOAT },
		{ "R64G64B64A64_UINT", ext::vulkan::enums::Format::R64G64B64A64_UINT },
		{ "R64G64B64A64_SINT", ext::vulkan::enums::Format::R64G64B64A64_SINT },
		{ "R64G64B64A64_SFLOAT", ext::vulkan::enums::Format::R64G64B64A64_SFLOAT },
		{ "B10G11R11_UFLOAT_PACK32", ext::vulkan::enums::Format::B10G11R11_UFLOAT_PACK32 },
		{ "E5B9G9R9_UFLOAT_PACK32", ext::vulkan::enums::Format::E5B9G9R9_UFLOAT_PACK32 },
		{ "D16_UNORM", ext::vulkan::enums::Format::D16_UNORM },
		{ "X8_D24_UNORM_PACK32", ext::vulkan::enums::Format::X8_D24_UNORM_PACK32 },
		{ "D32_SFLOAT", ext::vulkan::enums::Format::D32_SFLOAT },
		{ "S8_UINT", ext::vulkan::enums::Format::S8_UINT },
		{ "D16_UNORM_S8_UINT", ext::vulkan::enums::Format::D16_UNORM_S8_UINT },
		{ "D24_UNORM_S8_UINT", ext::vulkan::enums::Format::D24_UNORM_S8_UINT },
		{ "D32_SFLOAT_S8_UINT", ext::vulkan::enums::Format::D32_SFLOAT_S8_UINT },
		{ "BC1_RGB_UNORM_BLOCK", ext::vulkan::enums::Format::BC1_RGB_UNORM_BLOCK },
		{ "BC1_RGB_SRGB_BLOCK", ext::vulkan::enums::Format::BC1_RGB_SRGB_BLOCK },
		{ "BC1_RGBA_UNORM_BLOCK", ext::vulkan::enums::Format::BC1_RGBA_UNORM_BLOCK },
		{ "BC1_RGBA_SRGB_BLOCK", ext::vulkan::enums::Format::BC1_RGBA_SRGB_BLOCK },
		{ "BC2_UNORM_BLOCK", ext::vulkan::enums::Format::BC2_UNORM_BLOCK },
		{ "BC2_SRGB_BLOCK", ext::vulkan::enums::Format::BC2_SRGB_BLOCK },
		{ "BC3_UNORM_BLOCK", ext::vulkan::enums::Format::BC3_UNORM_BLOCK },
		{ "BC3_SRGB_BLOCK", ext::vulkan::enums::Format::BC3_SRGB_BLOCK },
		{ "BC4_UNORM_BLOCK", ext::vulkan::enums::Format::BC4_UNORM_BLOCK },
		{ "BC4_SNORM_BLOCK", ext::vulkan::enums::Format::BC4_SNORM_BLOCK },
		{ "BC5_UNORM_BLOCK", ext::vulkan::enums::Format::BC5_UNORM_BLOCK },
		{ "BC5_SNORM_BLOCK", ext::vulkan::enums::Format::BC5_SNORM_BLOCK },
		{ "BC6H_UFLOAT_BLOCK", ext::vulkan::enums::Format::BC6H_UFLOAT_BLOCK },
		{ "BC6H_SFLOAT_BLOCK", ext::vulkan::enums::Format::BC6H_SFLOAT_BLOCK },
		{ "BC7_UNORM_BLOCK", ext::vulkan::enums::Format::BC7_UNORM_BLOCK },
		{ "BC7_SRGB_BLOCK", ext::vulkan::enums::Format::BC7_SRGB_BLOCK },
		{ "ETC2_R8G8B8_UNORM_BLOCK", ext::vulkan::enums::Format::ETC2_R8G8B8_UNORM_BLOCK },
		{ "ETC2_R8G8B8_SRGB_BLOCK", ext::vulkan::enums::Format::ETC2_R8G8B8_SRGB_BLOCK },
		{ "ETC2_R8G8B8A1_UNORM_BLOCK", ext::vulkan::enums::Format::ETC2_R8G8B8A1_UNORM_BLOCK },
		{ "ETC2_R8G8B8A1_SRGB_BLOCK", ext::vulkan::enums::Format::ETC2_R8G8B8A1_SRGB_BLOCK },
		{ "ETC2_R8G8B8A8_UNORM_BLOCK", ext::vulkan::enums::Format::ETC2_R8G8B8A8_UNORM_BLOCK },
		{ "ETC2_R8G8B8A8_SRGB_BLOCK", ext::vulkan::enums::Format::ETC2_R8G8B8A8_SRGB_BLOCK },
		{ "EAC_R11_UNORM_BLOCK", ext::vulkan::enums::Format::EAC_R11_UNORM_BLOCK },
		{ "EAC_R11_SNORM_BLOCK", ext::vulkan::enums::Format::EAC_R11_SNORM_BLOCK },
		{ "EAC_R11G11_UNORM_BLOCK", ext::vulkan::enums::Format::EAC_R11G11_UNORM_BLOCK },
		{ "EAC_R11G11_SNORM_BLOCK", ext::vulkan::enums::Format::EAC_R11G11_SNORM_BLOCK },
		{ "ASTC_4x4_UNORM_BLOCK", ext::vulkan::enums::Format::ASTC_4x4_UNORM_BLOCK },
		{ "ASTC_4x4_SRGB_BLOCK", ext::vulkan::enums::Format::ASTC_4x4_SRGB_BLOCK },
		{ "ASTC_5x4_UNORM_BLOCK", ext::vulkan::enums::Format::ASTC_5x4_UNORM_BLOCK },
		{ "ASTC_5x4_SRGB_BLOCK", ext::vulkan::enums::Format::ASTC_5x4_SRGB_BLOCK },
		{ "ASTC_5x5_UNORM_BLOCK", ext::vulkan::enums::Format::ASTC_5x5_UNORM_BLOCK },
		{ "ASTC_5x5_SRGB_BLOCK", ext::vulkan::enums::Format::ASTC_5x5_SRGB_BLOCK },
		{ "ASTC_6x5_UNORM_BLOCK", ext::vulkan::enums::Format::ASTC_6x5_UNORM_BLOCK },
		{ "ASTC_6x5_SRGB_BLOCK", ext::vulkan::enums::Format::ASTC_6x5_SRGB_BLOCK },
		{ "ASTC_6x6_UNORM_BLOCK", ext::vulkan::enums::Format::ASTC_6x6_UNORM_BLOCK },
		{ "ASTC_6x6_SRGB_BLOCK", ext::vulkan::enums::Format::ASTC_6x6_SRGB_BLOCK },
		{ "ASTC_8x5_UNORM_BLOCK", ext::vulkan::enums::Format::ASTC_8x5_UNORM_BLOCK },
		{ "ASTC_8x5_SRGB_BLOCK", ext::vulkan::enums::Format::ASTC_8x5_SRGB_BLOCK },
		{ "ASTC_8x6_UNORM_BLOCK", ext::vulkan::enums::Format::ASTC_8x6_UNORM_BLOCK },
		{ "ASTC_8x6_SRGB_BLOCK", ext::vulkan::enums::Format::ASTC_8x6_SRGB_BLOCK },
		{ "ASTC_8x8_UNORM_BLOCK", ext::vulkan::enums::Format::ASTC_8x8_UNORM_BLOCK },
		{ "ASTC_8x8_SRGB_BLOCK", ext::vulkan::enums::Format::ASTC_8x8_SRGB_BLOCK },
		{ "ASTC_10x5_UNORM_BLOCK", ext::vulkan::enums::Format::ASTC_10x5_UNORM_BLOCK },
		{ "ASTC_10x5_SRGB_BLOCK", ext::vulkan::enums::Format::ASTC_10x5_SRGB_BLOCK },
		{ "ASTC_10x6_UNORM_BLOCK", ext::vulkan::enums::Format::ASTC_10x6_UNORM_BLOCK },
		{ "ASTC_10x6_SRGB_BLOCK", ext::vulkan::enums::Format::ASTC_10x6_SRGB_BLOCK },
		{ "ASTC_10x8_UNORM_BLOCK", ext::vulkan::enums::Format::ASTC_10x8_UNORM_BLOCK },
		{ "ASTC_10x8_SRGB_BLOCK", ext::vulkan::enums::Format::ASTC_10x8_SRGB_BLOCK },
		{ "ASTC_10x10_UNORM_BLOCK", ext::vulkan::enums::Format::ASTC_10x10_UNORM_BLOCK },
		{ "ASTC_10x10_SRGB_BLOCK", ext::vulkan::enums::Format::ASTC_10x10_SRGB_BLOCK },
		{ "ASTC_12x10_UNORM_BLOCK", ext::vulkan::enums::Format::ASTC_12x10_UNORM_BLOCK },
		{ "ASTC_12x10_SRGB_BLOCK", ext::vulkan::enums::Format::ASTC_12x10_SRGB_BLOCK },
		{ "ASTC_12x12_UNORM_BLOCK", ext::vulkan::enums::Format::ASTC_12x12_UNORM_BLOCK },
		{ "ASTC_12x12_SRGB_BLOCK", ext::vulkan::enums::Format::ASTC_12x12_SRGB_BLOCK },
/*
		{ "G8B8G8R8_422_UNORM", ext::vulkan::enums::Format::G8B8G8R8_422_UNORM },
		{ "B8G8R8G8_422_UNORM", ext::vulkan::enums::Format::B8G8R8G8_422_UNORM },
		{ "G8_B8_R8_3PLANE_420_UNORM", ext::vulkan::enums::Format::G8_B8_R8_3PLANE_420_UNORM },
		{ "G8_B8R8_2PLANE_420_UNORM", ext::vulkan::enums::Format::G8_B8R8_2PLANE_420_UNORM },
		{ "G8_B8_R8_3PLANE_422_UNORM", ext::vulkan::enums::Format::G8_B8_R8_3PLANE_422_UNORM },
		{ "G8_B8R8_2PLANE_422_UNORM", ext::vulkan::enums::Format::G8_B8R8_2PLANE_422_UNORM },
		{ "G8_B8_R8_3PLANE_444_UNORM", ext::vulkan::enums::Format::G8_B8_R8_3PLANE_444_UNORM },
		{ "R10X6_UNORM_PACK16", ext::vulkan::enums::Format::R10X6_UNORM_PACK16 },
		{ "R10X6G10X6_UNORM_2PACK16", ext::vulkan::enums::Format::R10X6G10X6_UNORM_2PACK16 },
		{ "R10X6G10X6B10X6A10X6_UNORM_4PACK16", ext::vulkan::enums::Format::R10X6G10X6B10X6A10X6_UNORM_4PACK16 },
		{ "G10X6B10X6G10X6R10X6_422_UNORM_4PACK16", ext::vulkan::enums::Format::G10X6B10X6G10X6R10X6_422_UNORM_4PACK16 },
		{ "B10X6G10X6R10X6G10X6_422_UNORM_4PACK16", ext::vulkan::enums::Format::B10X6G10X6R10X6G10X6_422_UNORM_4PACK16 },
		{ "G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16", ext::vulkan::enums::Format::G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16 },
		{ "G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16", ext::vulkan::enums::Format::G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16 },
		{ "G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16", ext::vulkan::enums::Format::G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16 },
		{ "G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16", ext::vulkan::enums::Format::G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16 },
		{ "G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16", ext::vulkan::enums::Format::G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16 },
		{ "R12X4_UNORM_PACK16", ext::vulkan::enums::Format::R12X4_UNORM_PACK16 },
		{ "R12X4G12X4_UNORM_2PACK16", ext::vulkan::enums::Format::R12X4G12X4_UNORM_2PACK16 },
		{ "R12X4G12X4B12X4A12X4_UNORM_4PACK16", ext::vulkan::enums::Format::R12X4G12X4B12X4A12X4_UNORM_4PACK16 },
		{ "G12X4B12X4G12X4R12X4_422_UNORM_4PACK16", ext::vulkan::enums::Format::G12X4B12X4G12X4R12X4_422_UNORM_4PACK16 },
		{ "B12X4G12X4R12X4G12X4_422_UNORM_4PACK16", ext::vulkan::enums::Format::B12X4G12X4R12X4G12X4_422_UNORM_4PACK16 },
		{ "G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16", ext::vulkan::enums::Format::G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16 },
		{ "G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16", ext::vulkan::enums::Format::G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16 },
		{ "G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16", ext::vulkan::enums::Format::G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16 },
		{ "G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16", ext::vulkan::enums::Format::G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16 },
		{ "G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16", ext::vulkan::enums::Format::G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16 },
		{ "G16B16G16R16_422_UNORM", ext::vulkan::enums::Format::G16B16G16R16_422_UNORM },
		{ "B16G16R16G16_422_UNORM", ext::vulkan::enums::Format::B16G16R16G16_422_UNORM },
		{ "G16_B16_R16_3PLANE_420_UNORM", ext::vulkan::enums::Format::G16_B16_R16_3PLANE_420_UNORM },
		{ "G16_B16R16_2PLANE_420_UNORM", ext::vulkan::enums::Format::G16_B16R16_2PLANE_420_UNORM },
		{ "G16_B16_R16_3PLANE_422_UNORM", ext::vulkan::enums::Format::G16_B16_R16_3PLANE_422_UNORM },
		{ "G16_B16R16_2PLANE_422_UNORM", ext::vulkan::enums::Format::G16_B16R16_2PLANE_422_UNORM },
		{ "G16_B16_R16_3PLANE_444_UNORM", ext::vulkan::enums::Format::G16_B16_R16_3PLANE_444_UNORM },
		{ "PVRTC1_2BPP_UNORM_BLOCK_IMG", ext::vulkan::enums::Format::PVRTC1_2BPP_UNORM_BLOCK_IMG },
		{ "PVRTC1_4BPP_UNORM_BLOCK_IMG", ext::vulkan::enums::Format::PVRTC1_4BPP_UNORM_BLOCK_IMG },
		{ "PVRTC2_2BPP_UNORM_BLOCK_IMG", ext::vulkan::enums::Format::PVRTC2_2BPP_UNORM_BLOCK_IMG },
		{ "PVRTC2_4BPP_UNORM_BLOCK_IMG", ext::vulkan::enums::Format::PVRTC2_4BPP_UNORM_BLOCK_IMG },
		{ "PVRTC1_2BPP_SRGB_BLOCK_IMG", ext::vulkan::enums::Format::PVRTC1_2BPP_SRGB_BLOCK_IMG },
		{ "PVRTC1_4BPP_SRGB_BLOCK_IMG", ext::vulkan::enums::Format::PVRTC1_4BPP_SRGB_BLOCK_IMG },
		{ "PVRTC2_2BPP_SRGB_BLOCK_IMG", ext::vulkan::enums::Format::PVRTC2_2BPP_SRGB_BLOCK_IMG },
		{ "PVRTC2_4BPP_SRGB_BLOCK_IMG", ext::vulkan::enums::Format::PVRTC2_4BPP_SRGB_BLOCK_IMG },
		{ "ASTC_4x4_SFLOAT_BLOCK_EXT", ext::vulkan::enums::Format::ASTC_4x4_SFLOAT_BLOCK_EXT },
		{ "ASTC_5x4_SFLOAT_BLOCK_EXT", ext::vulkan::enums::Format::ASTC_5x4_SFLOAT_BLOCK_EXT },
		{ "ASTC_5x5_SFLOAT_BLOCK_EXT", ext::vulkan::enums::Format::ASTC_5x5_SFLOAT_BLOCK_EXT },
		{ "ASTC_6x5_SFLOAT_BLOCK_EXT", ext::vulkan::enums::Format::ASTC_6x5_SFLOAT_BLOCK_EXT },
		{ "ASTC_6x6_SFLOAT_BLOCK_EXT", ext::vulkan::enums::Format::ASTC_6x6_SFLOAT_BLOCK_EXT },
		{ "ASTC_8x5_SFLOAT_BLOCK_EXT", ext::vulkan::enums::Format::ASTC_8x5_SFLOAT_BLOCK_EXT },
		{ "ASTC_8x6_SFLOAT_BLOCK_EXT", ext::vulkan::enums::Format::ASTC_8x6_SFLOAT_BLOCK_EXT },
		{ "ASTC_8x8_SFLOAT_BLOCK_EXT", ext::vulkan::enums::Format::ASTC_8x8_SFLOAT_BLOCK_EXT },
		{ "ASTC_10x5_SFLOAT_BLOCK_EXT", ext::vulkan::enums::Format::ASTC_10x5_SFLOAT_BLOCK_EXT },
		{ "ASTC_10x6_SFLOAT_BLOCK_EXT", ext::vulkan::enums::Format::ASTC_10x6_SFLOAT_BLOCK_EXT },
		{ "ASTC_10x8_SFLOAT_BLOCK_EXT", ext::vulkan::enums::Format::ASTC_10x8_SFLOAT_BLOCK_EXT },
		{ "ASTC_10x10_SFLOAT_BLOCK_EXT", ext::vulkan::enums::Format::ASTC_10x10_SFLOAT_BLOCK_EXT },
		{ "ASTC_12x10_SFLOAT_BLOCK_EXT", ext::vulkan::enums::Format::ASTC_12x10_SFLOAT_BLOCK_EXT },
		{ "ASTC_12x12_SFLOAT_BLOCK_EXT", ext::vulkan::enums::Format::ASTC_12x12_SFLOAT_BLOCK_EXT },
		{ "A4R4G4B4_UNORM_PACK16_EXT", ext::vulkan::enums::Format::A4R4G4B4_UNORM_PACK16_EXT },
		{ "A4B4G4R4_UNORM_PACK16_EXT", ext::vulkan::enums::Format::A4B4G4R4_UNORM_PACK16_EXT },
		{ "G8B8G8R8_422_UNORM_KHR", ext::vulkan::enums::Format::G8B8G8R8_422_UNORM_KHR },
		{ "B8G8R8G8_422_UNORM_KHR", ext::vulkan::enums::Format::B8G8R8G8_422_UNORM_KHR },
		{ "G8_B8_R8_3PLANE_420_UNORM_KHR", ext::vulkan::enums::Format::G8_B8_R8_3PLANE_420_UNORM_KHR },
		{ "G8_B8R8_2PLANE_420_UNORM_KHR", ext::vulkan::enums::Format::G8_B8R8_2PLANE_420_UNORM_KHR },
		{ "G8_B8_R8_3PLANE_422_UNORM_KHR", ext::vulkan::enums::Format::G8_B8_R8_3PLANE_422_UNORM_KHR },
		{ "G8_B8R8_2PLANE_422_UNORM_KHR", ext::vulkan::enums::Format::G8_B8R8_2PLANE_422_UNORM_KHR },
		{ "G8_B8_R8_3PLANE_444_UNORM_KHR", ext::vulkan::enums::Format::G8_B8_R8_3PLANE_444_UNORM_KHR },
		{ "R10X6_UNORM_PACK16_KHR", ext::vulkan::enums::Format::R10X6_UNORM_PACK16_KHR },
		{ "R10X6G10X6_UNORM_2PACK16_KHR", ext::vulkan::enums::Format::R10X6G10X6_UNORM_2PACK16_KHR },
		{ "R10X6G10X6B10X6A10X6_UNORM_4PACK16_KHR", ext::vulkan::enums::Format::R10X6G10X6B10X6A10X6_UNORM_4PACK16_KHR },
		{ "G10X6B10X6G10X6R10X6_422_UNORM_4PACK16_KHR", ext::vulkan::enums::Format::G10X6B10X6G10X6R10X6_422_UNORM_4PACK16_KHR },
		{ "B10X6G10X6R10X6G10X6_422_UNORM_4PACK16_KHR", ext::vulkan::enums::Format::B10X6G10X6R10X6G10X6_422_UNORM_4PACK16_KHR },
		{ "G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16_KHR", ext::vulkan::enums::Format::G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16_KHR },
		{ "G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16_KHR", ext::vulkan::enums::Format::G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16_KHR },
		{ "G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16_KHR", ext::vulkan::enums::Format::G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16_KHR },
		{ "G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16_KHR", ext::vulkan::enums::Format::G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16_KHR },
		{ "G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16_KHR", ext::vulkan::enums::Format::G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16_KHR },
		{ "R12X4_UNORM_PACK16_KHR", ext::vulkan::enums::Format::R12X4_UNORM_PACK16_KHR },
		{ "R12X4G12X4_UNORM_2PACK16_KHR", ext::vulkan::enums::Format::R12X4G12X4_UNORM_2PACK16_KHR },
		{ "R12X4G12X4B12X4A12X4_UNORM_4PACK16_KHR", ext::vulkan::enums::Format::R12X4G12X4B12X4A12X4_UNORM_4PACK16_KHR },
		{ "G12X4B12X4G12X4R12X4_422_UNORM_4PACK16_KHR", ext::vulkan::enums::Format::G12X4B12X4G12X4R12X4_422_UNORM_4PACK16_KHR },
		{ "B12X4G12X4R12X4G12X4_422_UNORM_4PACK16_KHR", ext::vulkan::enums::Format::B12X4G12X4R12X4G12X4_422_UNORM_4PACK16_KHR },
		{ "G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16_KHR", ext::vulkan::enums::Format::G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16_KHR },
		{ "G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16_KHR", ext::vulkan::enums::Format::G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16_KHR },
		{ "G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16_KHR", ext::vulkan::enums::Format::G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16_KHR },
		{ "G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16_KHR", ext::vulkan::enums::Format::G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16_KHR },
		{ "G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16_KHR", ext::vulkan::enums::Format::G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16_KHR },
		{ "G16B16G16R16_422_UNORM_KHR", ext::vulkan::enums::Format::G16B16G16R16_422_UNORM_KHR },
		{ "B16G16R16G16_422_UNORM_KHR", ext::vulkan::enums::Format::B16G16R16G16_422_UNORM_KHR },
		{ "G16_B16_R16_3PLANE_420_UNORM_KHR", ext::vulkan::enums::Format::G16_B16_R16_3PLANE_420_UNORM_KHR },
		{ "G16_B16R16_2PLANE_420_UNORM_KHR", ext::vulkan::enums::Format::G16_B16R16_2PLANE_420_UNORM_KHR },
		{ "G16_B16_R16_3PLANE_422_UNORM_KHR", ext::vulkan::enums::Format::G16_B16_R16_3PLANE_422_UNORM_KHR },
		{ "G16_B16R16_2PLANE_422_UNORM_KHR", ext::vulkan::enums::Format::G16_B16R16_2PLANE_422_UNORM_KHR },
		{ "G16_B16_R16_3PLANE_444_UNORM_KHR", ext::vulkan::enums::Format::G16_B16_R16_3PLANE_444_UNORM_KHR },
*/
	};
	return table[string];
}

#endif