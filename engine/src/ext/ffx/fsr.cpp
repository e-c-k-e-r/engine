#if UF_USE_FFX_FSR || UF_USE_FFX_SDK

#include <cfloat>

#include <uf/ext/ffx/fsr.h>
#include <uf/engine/graph/graph.h>

#if UF_USE_FFX_SDK
	#include <FidelityFX/host/backends/vk/ffx_vk.h>
	#define UF_USE_FFX_SDK 3

	#if UF_USE_FFX_SDK == 2
		#include <FidelityFX/host/ffx_fsr2.h>
		#define FfxContext FfxFsr2Context
		#define FfxContextDescription FfxFsr2ContextDescription
		#define FfxGenerateReactiveDescription FfxFsr2GenerateReactiveDescription
		#define FfxDispatchDescription FfxFsr2DispatchDescription

		#define ffxContextGenerateReactiveMask ffxFsr2ContextGenerateReactiveMask
		#define ffxContextDispatch ffxFsr2ContextDispatch
		#define ffxContextCreate ffxFsr2ContextCreate
		#define ffxContextDestroy ffxFsr2ContextDestroy
		#define ffxGetJitterPhaseCount ffxFsr2GetJitterPhaseCount
		#define ffxGetJitterOffset ffxFsr2GetJitterOffset

		#define FFX_FSR_AUTOREACTIVEFLAGS_APPLY_TONEMAP FFX_FSR2_AUTOREACTIVEFLAGS_APPLY_TONEMAP
		#define FFX_FSR_AUTOREACTIVEFLAGS_APPLY_INVERSETONEMAP FFX_FSR2_AUTOREACTIVEFLAGS_APPLY_INVERSETONEMAP
		#define FFX_FSR_AUTOREACTIVEFLAGS_APPLY_THRESHOLD FFX_FSR2_AUTOREACTIVEFLAGS_APPLY_THRESHOLD
		#define FFX_FSR_AUTOREACTIVEFLAGS_USE_COMPONENTS_MAX FFX_FSR2_AUTOREACTIVEFLAGS_USE_COMPONENTS_MAX
		#define FFX_FSR_ENABLE_AUTO_EXPOSURE FFX_FSR2_ENABLE_AUTO_EXPOSURE
		#define FFX_FSR_ENABLE_MOTION_VECTORS_JITTER_CANCELLATION FFX_FSR2_ENABLE_MOTION_VECTORS_JITTER_CANCELLATION
		#define FFX_FSR_ENABLE_HIGH_DYNAMIC_RANGE FFX_FSR2_ENABLE_HIGH_DYNAMIC_RANGE
		#define FFX_FSR_ENABLE_DEPTH_INVERTED FFX_FSR2_ENABLE_DEPTH_INVERTED
		#define FFX_FSR_ENABLE_DEPTH_INFINITE FFX_FSR2_ENABLE_DEPTH_INFINITE

		#define FSR_InputColor L"FSR2_InputColor"
		#define FSR_InputDepth L"FSR2_InputDepth"
		#define FSR_InputMotionVectors L"FSR2_InputMotionVectors"
		#define FSR_OutputUpscaledColor L"FSR2_OutputUpscaledColor"
		#define FSR_OutputUpscaledColor L"FSR2_OutputUpscaledColor"
		#define FSR_InputColor L"FSR2_InputColor"
		#define FSR_InputDepth L"FSR2_InputDepth"
		#define FSR_InputMotionVectors L"FSR2_InputMotionVectors"
		#define FSR_OutputUpscaledColor L"FSR2_OutputUpscaledColor"
		#define FSR_InputExposure L"FSR2_InputExposure"
		#define FSR_InputReactiveMap L"FSR2_InputReactiveMap"
		#define FSR_TransparencyAndCompositionMap L"FSR2_TransparencyAndCompositionMap"
	#elif UF_USE_FFX_SDK == 3
		#include <FidelityFX/host/ffx_fsr3upscaler.h>
		#define FfxContext FfxFsr3UpscalerContext
		#define FfxContextDescription FfxFsr3UpscalerContextDescription
		#define FfxGenerateReactiveDescription FfxFsr3UpscalerGenerateReactiveDescription
		#define FfxDispatchDescription FfxFsr3UpscalerDispatchDescription

		#define ffxContextGenerateReactiveMask ffxFsr3UpscalerContextGenerateReactiveMask
		#define ffxContextDispatch ffxFsr3UpscalerContextDispatch
		#define ffxContextCreate ffxFsr3UpscalerContextCreate
		#define ffxContextDestroy ffxFsr3UpscalerContextDestroy
		#define ffxGetJitterPhaseCount ffxFsr3UpscalerGetJitterPhaseCount
		#define ffxGetJitterOffset ffxFsr3UpscalerGetJitterOffset

		#define FFX_FSR_AUTOREACTIVEFLAGS_APPLY_TONEMAP FFX_FSR3UPSCALER_AUTOREACTIVEFLAGS_APPLY_TONEMAP
		#define FFX_FSR_AUTOREACTIVEFLAGS_APPLY_INVERSETONEMAP FFX_FSR3UPSCALER_AUTOREACTIVEFLAGS_APPLY_INVERSETONEMAP
		#define FFX_FSR_AUTOREACTIVEFLAGS_APPLY_THRESHOLD FFX_FSR3UPSCALER_AUTOREACTIVEFLAGS_APPLY_THRESHOLD
		#define FFX_FSR_AUTOREACTIVEFLAGS_USE_COMPONENTS_MAX FFX_FSR3UPSCALER_AUTOREACTIVEFLAGS_USE_COMPONENTS_MAX
		#define FFX_FSR_ENABLE_AUTO_EXPOSURE FFX_FSR3UPSCALER_ENABLE_AUTO_EXPOSURE
		#define FFX_FSR_ENABLE_MOTION_VECTORS_JITTER_CANCELLATION FFX_FSR3UPSCALER_ENABLE_MOTION_VECTORS_JITTER_CANCELLATION
		#define FFX_FSR_ENABLE_HIGH_DYNAMIC_RANGE FFX_FSR3UPSCALER_ENABLE_HIGH_DYNAMIC_RANGE
		#define FFX_FSR_ENABLE_DEPTH_INVERTED FFX_FSR3UPSCALER_ENABLE_DEPTH_INVERTED
		#define FFX_FSR_ENABLE_DEPTH_INFINITE FFX_FSR3UPSCALER_ENABLE_DEPTH_INFINITE

		#define FSR_InputColor L"FSR3UPSCALER_InputColor"
		#define FSR_InputDepth L"FSR3UPSCALER_InputDepth"
		#define FSR_InputMotionVectors L"FSR3UPSCALER_InputMotionVectors"
		#define FSR_OutputUpscaledColor L"FSR3UPSCALER_OutputUpscaledColor"
		#define FSR_OutputUpscaledColor L"FSR3UPSCALER_OutputUpscaledColor"
		#define FSR_InputColor L"FSR3UPSCALER_InputColor"
		#define FSR_InputDepth L"FSR3UPSCALER_InputDepth"
		#define FSR_InputMotionVectors L"FSR3UPSCALER_InputMotionVectors"
		#define FSR_OutputUpscaledColor L"FSR3UPSCALER_OutputUpscaledColor"
		#define FSR_InputExposure L"FSR3UPSCALER_InputExposure"
		#define FSR_InputReactiveMap L"FSR3UPSCALER_InputReactiveMap"
		#define FSR_TransparencyAndCompositionMap L"FSR3UPSCALER_TransparencyAndCompositionMap"

		#define FSR_DilatedMotionVectors L"FSR3UPSCALER_DilatedMotionVectors"
		#define FSR_DilatedDepth L"FSR3UPSCALER_DilatedDepth"
		#define FSR_ReconstructedPrevNearestDepth L"FSR3UPSCALER_ReconstructedPrevNearestDepth"
	#else
		#error "Invalid FFX-SDK version specified"
	#endif
#else
	#include <ffx_fsr2/ffx_fsr2.h>
	#include <ffx_fsr2/vk/ffx_fsr2_vk.h>

	#define FfxContext FfxFsr2Context
	#define FfxContextDescription FfxFsr2ContextDescription
	#define FfxGenerateReactiveDescription FfxFsr2GenerateReactiveDescription
	#define FfxDispatchDescription FfxFsr2DispatchDescription

	#define ffxGetScratchMemorySizeVK ffxFsr2GetScratchMemorySizeVK
	#define ffxGetInterfaceVK ffxFsr2GetInterfaceVK
	#define ffxContextGenerateReactiveMask ffxFsr2ContextGenerateReactiveMask
	#define ffxContextDispatch ffxFsr2ContextDispatch
	#define ffxContextCreate ffxFsr2ContextCreate
	#define ffxContextDestroy ffxFsr2ContextDestroy
	#define ffxGetJitterPhaseCount ffxFsr2GetJitterPhaseCount
	#define ffxGetJitterOffset ffxFsr2GetJitterOffset

	#define FSR_InputColor L"FSR2_InputColor"
	#define FSR_InputDepth L"FSR2_InputDepth"
	#define FSR_InputMotionVectors L"FSR2_InputMotionVectors"
	#define FSR_OutputUpscaledColor L"FSR2_OutputUpscaledColor"
	#define FSR_OutputUpscaledColor L"FSR2_OutputUpscaledColor"
	#define FSR_InputColor L"FSR2_InputColor"
	#define FSR_InputDepth L"FSR2_InputDepth"
	#define FSR_InputMotionVectors L"FSR2_InputMotionVectors"
	#define FSR_OutputUpscaledColor L"FSR2_OutputUpscaledColor"
	#define FSR_InputExposure L"FSR2_InputExposure"
	#define FSR_InputReactiveMap L"FSR2_InputReactiveMap"
	#define FSR_TransparencyAndCompositionMap L"FSR2_TransparencyAndCompositionMap"
#endif

#include <uf/utils/renderer/renderer.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/io/fmt.h>

namespace {
	pod::Matrix4f jitterMatrix = uf::matrix::identity();

	uf::stl::vector<uint8_t> scratchBuffer;
	
	FfxContext context;
	FfxContextDescription contextDescription;

	struct {
		uf::renderer::Texture empty;
		uf::renderer::Texture output;
	#if UF_USE_FFX_SDK == 3
		uf::renderer::Texture dilatedMotionVectors;
		uf::renderer::Texture dilatedDepth;
		uf::renderer::Texture reconstructedPrevNearestDepth;
	#endif
	} resources;

	void initializeResource( uf::renderer::Texture& resource, uint32_t width = 0, uint32_t height = 0 ) {
		if ( width == 0 ) width = uf::renderer::settings::width;
		if ( height == 0 ) height = uf::renderer::settings::height;
		resource.destroy();
		resource.fromBuffers(
			NULL,
			0,
			resource.format,
			width, height,
			1,
			1,
			VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
			VK_IMAGE_LAYOUT_GENERAL
		);
	}

	uf::stl::string FFX_ERROR_TO_STRING( FfxErrorCode code ) {
		switch ( code ) {
			case FFX_OK: return "OK"; break;
			case FFX_ERROR_INVALID_POINTER: return "ERROR_INVALID_POINTER"; break;
			case FFX_ERROR_INVALID_ALIGNMENT: return "ERROR_INVALID_ALIGNMENT"; break;
			case FFX_ERROR_INVALID_SIZE: return "ERROR_INVALID_SIZE"; break;
			case FFX_EOF: return "EOF"; break;
			case FFX_ERROR_INVALID_PATH: return "ERROR_INVALID_PATH"; break;
			case FFX_ERROR_EOF: return "ERROR_EOF"; break;
			case FFX_ERROR_MALFORMED_DATA: return "ERROR_MALFORMED_DATA"; break;
			case FFX_ERROR_OUT_OF_MEMORY: return "ERROR_OUT_OF_MEMORY"; break;
			case FFX_ERROR_INCOMPLETE_INTERFACE: return "ERROR_INCOMPLETE_INTERFACE"; break;
			case FFX_ERROR_INVALID_ENUM: return "ERROR_INVALID_ENUM"; break;
			case FFX_ERROR_INVALID_ARGUMENT: return "ERROR_INVALID_ARGUMENT"; break;
			case FFX_ERROR_OUT_OF_RANGE: return "ERROR_OUT_OF_RANGE"; break;
			case FFX_ERROR_NULL_DEVICE: return "ERROR_NULL_DEVICE"; break;
			case FFX_ERROR_BACKEND_API_ERROR: return "ERROR_BACKEND_API_ERROR"; break;
			case FFX_ERROR_INSUFFICIENT_MEMORY: return "ERROR_INSUFFICIENT_MEMORY"; break;
		}
		return ::fmt::format("{}", (void*) code);
	}

	#define FFX_ERROR_CHECK(f) { auto error = f; if ( error != FFX_OK) UF_MSG_ERROR("FFX-FSR Error {}: {}", #f, FFX_ERROR_TO_STRING(error)); }

#if UF_USE_FFX_SDK
	FfxResource createFfxResource( const uf::renderer::Texture& texture, const wchar_t* name, FfxResourceStates state = FFX_RESOURCE_STATE_COMPUTE_READ ) {
		FfxResourceDescription desc = {};
		desc.type = FFX_RESOURCE_TYPE_TEXTURE2D;
		desc.width = texture.width > 0 ? texture.width : 1;
		desc.height = texture.height > 0 ? texture.height : 1;
		desc.depth = 1;
		desc.mipCount = texture.mips > 0 ? texture.mips : 1;
		desc.format = ffxGetSurfaceFormatVK(texture.format);
		desc.flags = FFX_RESOURCE_FLAGS_NONE;
		if ( texture.usage & VK_IMAGE_USAGE_STORAGE_BIT ) desc.usage = FFX_RESOURCE_USAGE_UAV;

		return ffxGetResourceVK(texture.image, desc, (wchar_t*) name, state);
   };
   FfxResource createFfxResource( const uf::renderer::RenderTarget::Attachment& attachment, const wchar_t* name, FfxResourceStates state = FFX_RESOURCE_STATE_COMPUTE_READ ) {
   		uf::renderer::Texture texture;
   		texture.aliasAttachment( attachment );
		return createFfxResource( texture, name, state );
	}
#endif

	void draw(VkCommandBuffer commandBuffer, size_t swapchainIndex) {
		FfxDispatchDescription dispatchParameters = {};
		dispatchParameters.commandList = ffxGetCommandListVK(commandBuffer);
	
		if ( !uf::renderer::hasRenderMode("", true) ) return;	
		auto& renderMode = uf::renderer::getRenderMode("", true);
		if ( !renderMode.hasAttachment("output") && !renderMode.hasAttachment("color") ) return;	

		pod::Vector2ui renderSize = {
			renderMode.width > 0 ? renderMode.width : (uf::renderer::settings::width * renderMode.scale),
			renderMode.height > 0 ? renderMode.height : (uf::renderer::settings::height * renderMode.scale),
		};
		dispatchParameters.renderSize.width = renderSize.x;
		dispatchParameters.renderSize.height = renderSize.y;

		auto& scene = uf::scene::getCurrentScene();
		auto& storage = uf::graph::globalStorage ? uf::graph::storage : scene.getComponent<pod::Graph::Storage>();

		auto& attachmentColor = renderMode.hasAttachment("output") ? renderMode.getAttachment("output") : renderMode.getAttachment("color");
		auto& attachmentDepth = storage.buffers.depthPyramid;
	//	auto& attachmentDepth = renderMode.getAttachment("depth"); // using the depth pyramid since it's already a R32_SFLOAT format
		auto& attachmentMotion = renderMode.getAttachment("motion");

		dispatchParameters.motionVectorScale.x = renderSize.x;
		dispatchParameters.motionVectorScale.y = renderSize.y;

	#if UF_USE_FFX_SDK
		dispatchParameters.color = createFfxResource(attachmentColor, FSR_InputColor );
		dispatchParameters.depth = createFfxResource(attachmentDepth, FSR_InputDepth );
		
		dispatchParameters.motionVectors = createFfxResource(attachmentMotion, FSR_InputMotionVectors );
		dispatchParameters.exposure = createFfxResource(::resources.empty, FSR_InputExposure);
		dispatchParameters.reactive = createFfxResource(::resources.empty, FSR_InputReactiveMap);
		dispatchParameters.transparencyAndComposition = createFfxResource(::resources.empty, FSR_TransparencyAndCompositionMap);

	#if UF_USE_FFX_SDK == 3
		dispatchParameters.dilatedMotionVectors = createFfxResource(::resources.dilatedMotionVectors, FSR_DilatedMotionVectors, FFX_RESOURCE_STATE_UNORDERED_ACCESS);
		dispatchParameters.dilatedDepth = createFfxResource(::resources.dilatedDepth, FSR_DilatedDepth, FFX_RESOURCE_STATE_UNORDERED_ACCESS);
		dispatchParameters.reconstructedPrevNearestDepth = createFfxResource(::resources.reconstructedPrevNearestDepth, FSR_ReconstructedPrevNearestDepth, FFX_RESOURCE_STATE_UNORDERED_ACCESS);
	#endif
		
		dispatchParameters.output = createFfxResource(::resources.output, FSR_OutputUpscaledColor, FFX_RESOURCE_STATE_UNORDERED_ACCESS );
	#else
		dispatchParameters.color = ffxGetTextureResourceVK(&::context, attachmentColor.image, attachmentColor.view, renderSize.x, renderSize.y, attachmentColor.descriptor.format, FSR_InputColor );
		dispatchParameters.depth = ffxGetTextureResourceVK(&::context, attachmentDepth.image, attachmentDepth.view, renderSize.x, renderSize.y, attachmentDepth.descriptor.format, FSR_InputDepth );
		dispatchParameters.motionVectors = ffxGetTextureResourceVK(&::context, attachmentMotion.image, attachmentMotion.view, renderSize.x, renderSize.y, attachmentMotion.descriptor.format, FSR_InputMotionVectors );\
		
		dispatchParameters.exposure = ffxGetTextureResourceVK(&::context, nullptr, nullptr, 1, 1, VK_FORMAT_UNDEFINED, FSR_InputExposure );
		dispatchParameters.reactive = ffxGetTextureResourceVK(&::context, nullptr, nullptr, 1, 1, VK_FORMAT_UNDEFINED, FSR_InputReactiveMap );
		dispatchParameters.transparencyAndComposition = ffxGetTextureResourceVK(&::context, nullptr, nullptr, 1, 1, VK_FORMAT_UNDEFINED, FSR_TransparencyAndCompositionMap );
		
		dispatchParameters.output = ffxGetTextureResourceVK(&::context, ::resources.output.image, ::resources.output.view, ::resources.output.width, ::resources.output.height, ::resources.output.format, FSR_OutputUpscaledColor, FFX_RESOURCE_STATE_UNORDERED_ACCESS );
	#endif

		auto& controller = scene.getController();
		auto& camera = controller.getComponent<uf::Camera>();
		auto& projection = camera.getProjection();

		dispatchParameters.jitterOffset.x = ext::fsr::jitter.x;
		dispatchParameters.jitterOffset.y = ext::fsr::jitter.y;

		dispatchParameters.reset = uf::renderer::states::frameAccumulateReset;
		dispatchParameters.enableSharpening = ext::fsr::sharpness > 0.0f;
		dispatchParameters.sharpness = ext::fsr::sharpness;
		dispatchParameters.frameTimeDelta = uf::time::delta * 1000;
		dispatchParameters.preExposure = 1.0f;
		dispatchParameters.cameraFar = FLT_MAX;
		dispatchParameters.cameraNear = projection(2,3);
		dispatchParameters.cameraFovAngleVertical = 2.0f * std::atan(1.0f / fabs(projection(1,1)));

		FFX_ERROR_CHECK(ffxContextDispatch(&::context, &dispatchParameters));
	}
}

uf::stl::string ext::fsr::preset = "native";
pod::Vector2f ext::fsr::jitter = {};
float ext::fsr::sharpness = 1.0f;
float ext::fsr::jitterScale = 2.0f;
bool ext::fsr::initialized = false;

uf::renderer::Texture& ext::fsr::getRenderTarget() {
	return ::resources.output;
}

void ext::fsr::initialize() {
	auto scratchSize = ffxGetScratchMemorySizeVK(uf::renderer::device.physicalDevice, MAX(uf::renderer::device.extensions.properties.device.size(), 1) );
	::scratchBuffer.resize(scratchSize);

	::contextDescription.maxRenderSize.width = 3840;
	::contextDescription.maxRenderSize.height = 2160;
#if UF_USE_FFX_SDK == 3
	::contextDescription.maxUpscaleSize.width = uf::renderer::settings::width;
	::contextDescription.maxUpscaleSize.height = uf::renderer::settings::height;
#else
	::contextDescription.displaySize.width = uf::renderer::settings::width;
	::contextDescription.displaySize.height = uf::renderer::settings::height;
#endif
	::contextDescription.flags = FFX_FSR_ENABLE_AUTO_EXPOSURE | FFX_FSR_ENABLE_MOTION_VECTORS_JITTER_CANCELLATION;

	if ( uf::renderer::settings::pipelines::hdr ) ::contextDescription.flags |= FFX_FSR_ENABLE_HIGH_DYNAMIC_RANGE;
	if ( uf::matrix::reverseInfiniteProjection ) ::contextDescription.flags |= FFX_FSR_ENABLE_DEPTH_INVERTED | FFX_FSR_ENABLE_DEPTH_INFINITE;

#if UF_USE_FFX_SDK
	VkDeviceContext deviceContextVK = {};
	deviceContextVK.vkDevice = uf::renderer::device.logicalDevice;
	deviceContextVK.vkPhysicalDevice = uf::renderer::device.physicalDevice;
	deviceContextVK.vkDeviceProcAddr = vkGetDeviceProcAddr;
	FfxDevice ffxDevice = ffxGetDeviceVK(&deviceContextVK);
	FFX_ERROR_CHECK(ffxGetInterfaceVK( &::contextDescription.backendInterface, ffxDevice, ::scratchBuffer.data(), ::scratchBuffer.size(), 1 ));
#else
	FFX_ERROR_CHECK(ffxGetInterfaceVK( &::contextDescription.callbacks, ::scratchBuffer.data(), ::scratchBuffer.size(), uf::renderer::device.physicalDevice, &vkGetDeviceProcAddr ));
#endif
	FFX_ERROR_CHECK(ffxContextCreate( &::context, &::contextDescription ));

	::resources.output.format = uf::renderer::settings::pipelines::hdr ? uf::renderer::enums::Format::HDR : uf::renderer::enums::Format::SDR;
	::initializeResource( ::resources.output );
#if UF_USE_FFX_SDK == 3
	::resources.dilatedMotionVectors.format = uf::renderer::enums::Format::R16G16_SFLOAT;
	::resources.dilatedDepth.format = uf::renderer::enums::Format::R32_SFLOAT;
	::resources.reconstructedPrevNearestDepth.format = uf::renderer::enums::Format::R32_UINT;

	::initializeResource( ::resources.dilatedMotionVectors );
	::initializeResource( ::resources.dilatedDepth );
	::initializeResource( ::resources.reconstructedPrevNearestDepth );
#endif

	ext::fsr::initialized = true;
}
void ext::fsr::tick() {
	if ( !ext::fsr::initialized ) return;

	pod::Vector2ui renderSize = {};
	pod::Vector2ui displaySize = {};
	
	if ( !uf::renderer::hasRenderMode("", true) ) return;
	auto& renderMode = uf::renderer::getRenderMode("", true);
	renderSize = {
		renderMode.width > 0 ? renderMode.width : (uf::renderer::settings::width * renderMode.scale),
		renderMode.height > 0 ? renderMode.height : (uf::renderer::settings::height * renderMode.scale),
	};

	{
		if ( !uf::renderer::hasRenderMode("Swapchain", true) ) return;
		auto& swapchainRenderMode = uf::renderer::getRenderMode("Swapchain", true);
		displaySize = {
			swapchainRenderMode.width > 0 ? swapchainRenderMode.width : uf::renderer::settings::width,
			swapchainRenderMode.height > 0 ? swapchainRenderMode.height : uf::renderer::settings::height,
		};
	}

	if ( uf::renderer::states::resized ) {
	#if UF_USE_FFX_SDK == 3
		::contextDescription.maxUpscaleSize.width = uf::renderer::settings::width;
		::contextDescription.maxUpscaleSize.height = uf::renderer::settings::height;
	#else
		::contextDescription.displaySize.width = displaySize.x;
		::contextDescription.displaySize.height = displaySize.y;
	#endif
		FFX_ERROR_CHECK(ffxContextDestroy( &::context ));
		FFX_ERROR_CHECK(ffxContextCreate( &::context, &::contextDescription ));

		::initializeResource( ::resources.output, displaySize.x, displaySize.y );
	#if UF_USE_FFX_SDK == 3
		::initializeResource( ::resources.dilatedMotionVectors, displaySize.x, displaySize.y );
		::initializeResource( ::resources.dilatedDepth, displaySize.x, displaySize.y );
		::initializeResource( ::resources.reconstructedPrevNearestDepth, displaySize.x, displaySize.y );
	#endif
	}

	const int32_t jitterPhaseCount = ffxGetJitterPhaseCount(renderSize.x, displaySize.x);
	static uint32_t index = 0;
	index = (index + 1) % jitterPhaseCount;

	ffxGetJitterOffset(&ext::fsr::jitter.x, &ext::fsr::jitter.y, index, jitterPhaseCount);

	pod::Vector2f jitter = {};

	jitter.x = ext::fsr::jitterScale * ext::fsr::jitter.x / (float) renderSize.x;
	jitter.y = ext::fsr::jitterScale * ext::fsr::jitter.y / (float) renderSize.y;

	ext::fsr::jitter = jitter;
	::jitterMatrix = uf::matrix::translate( uf::matrix::identity(), pod::Vector3f{ jitter.x, jitter.y, 0 } );
}
void ext::fsr::render() {
	if ( !ext::fsr::initialized ) return;
	auto commandBuffer = uf::renderer::device.fetchCommandBuffer(uf::renderer::QueueEnum::GRAPHICS, true); // immediately flush
	draw(commandBuffer, uf::renderer::states::currentBuffer);
	uf::renderer::device.flushCommandBuffer(commandBuffer);
}
void ext::fsr::terminate() {
	if ( !ext::fsr::initialized ) return;
	FFX_ERROR_CHECK(ffxContextDestroy( &::context ));
}

pod::Matrix4f ext::fsr::getJitterMatrix() {
	return ::jitterMatrix;
}

#endif