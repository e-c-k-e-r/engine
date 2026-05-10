#if UF_USE_FFX_FSR || UF_USE_FFX_SDK

#include <cfloat>

#include <uf/ext/ffx/fsr.h>
#include <uf/engine/graph/graph.h>

#define FFX_FSR_BLOCK_SIZE 8
#define FFX_FSR_MAX_WIDTH 3840
#define FFX_FSR_MAX_HEIGHT 2160

#if UF_USE_FFX_SDK
	#include <FidelityFX/host/backends/vk/ffx_vk.h>
	
	#define FFX_SDK_2 2
	#define FFX_SDK_3 3
	#define FFX_SDK_3_1 31

	#define UF_USE_FFX_SDK FFX_SDK_3_1
	#define UF_USE_FFX_SDR_FRAME_INTERP 1

	#if UF_USE_FFX_SDK == FFX_SDK_2
		#warning "Using FFX-FSR2"
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
	#elif UF_USE_FFX_SDK == FFX_SDK_3
		#warning "Using FFX-FSR3"
		#include <FidelityFX/host/ffx_fsr3.h>
		#define FfxContext FfxFsr3Context
		#define FfxContextDescription FfxFsr3ContextDescription
		#define FfxGenerateReactiveDescription FfxFsr3GenerateReactiveDescription
		#define FfxDispatchDescription FfxFsr3DispatchUpscaleDescription

		#define ffxContextGenerateReactiveMask ffxFsr3ContextGenerateReactiveMask
		#define ffxContextDispatch ffxFsr3ContextDispatchUpscale
		#define ffxContextCreate ffxFsr3ContextCreate
		#define ffxContextDestroy ffxFsr3ContextDestroy
		#define ffxGetJitterPhaseCount ffxFsr3GetJitterPhaseCount
		#define ffxGetJitterOffset ffxFsr3GetJitterOffset

		#define FFX_FSR_AUTOREACTIVEFLAGS_APPLY_TONEMAP FFX_FSR3_AUTOREACTIVEFLAGS_APPLY_TONEMAP
		#define FFX_FSR_AUTOREACTIVEFLAGS_APPLY_INVERSETONEMAP FFX_FSR3_AUTOREACTIVEFLAGS_APPLY_INVERSETONEMAP
		#define FFX_FSR_AUTOREACTIVEFLAGS_APPLY_THRESHOLD FFX_FSR3_AUTOREACTIVEFLAGS_APPLY_THRESHOLD
		#define FFX_FSR_AUTOREACTIVEFLAGS_USE_COMPONENTS_MAX FFX_FSR3_AUTOREACTIVEFLAGS_USE_COMPONENTS_MAX
		#define FFX_FSR_ENABLE_AUTO_EXPOSURE FFX_FSR3_ENABLE_AUTO_EXPOSURE
		#define FFX_FSR_ENABLE_MOTION_VECTORS_JITTER_CANCELLATION FFX_FSR3_ENABLE_MOTION_VECTORS_JITTER_CANCELLATION
		#define FFX_FSR_ENABLE_HIGH_DYNAMIC_RANGE FFX_FSR3_ENABLE_HIGH_DYNAMIC_RANGE
		#define FFX_FSR_ENABLE_DEPTH_INVERTED FFX_FSR3_ENABLE_DEPTH_INVERTED
		#define FFX_FSR_ENABLE_DEPTH_INFINITE FFX_FSR3_ENABLE_DEPTH_INFINITE

		#define FSR_InputColor L"FSR3_InputColor"
		#define FSR_InputDepth L"FSR3_InputDepth"
		#define FSR_InputMotionVectors L"FSR3_InputMotionVectors"
		#define FSR_OutputUpscaledColor L"FSR3_OutputUpscaledColor"
		#define FSR_OutputUpscaledColor L"FSR3_OutputUpscaledColor"
		#define FSR_InputColor L"FSR3_InputColor"
		#define FSR_InputDepth L"FSR3_InputDepth"
		#define FSR_InputMotionVectors L"FSR3_InputMotionVectors"
		#define FSR_OutputUpscaledColor L"FSR3_OutputUpscaledColor"
		#define FSR_InputExposure L"FSR3_InputExposure"
		#define FSR_InputReactiveMap L"FSR3_InputReactiveMap"
		#define FSR_TransparencyAndCompositionMap L"FSR3_TransparencyAndCompositionMap"

		#define FSR_DilatedMotionVectors L"FSR_DilatedMotionVectors"
		#define FSR_DilatedDepth L"FSR_DilatedDepth"
		#define FSR_ReconstructedPrevNearestDepth L"FSR_ReconstructedPrevNearestDepth"
	#elif UF_USE_FFX_SDK == FFX_SDK_3_1
		#warning "Using FFX-FSR3.1"
		#include <FidelityFX/host/ffx_fsr3upscaler.h>
		#include <FidelityFX/host/ffx_frameinterpolation.h>
		#include <FidelityFX/host/ffx_opticalflow.h>

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

#if UF_USE_FFX_SDR_FRAME_INTERP && UF_USE_FFX_SDK == FFX_SDK_3_1
	FfxSwapchain ffxSwapchain = nullptr;
	FfxSwapchainReplacementFunctions ffxSwapchainFuncs = {};

	uf::renderer::Graphic compositor;

	uf::stl::vector<uint8_t> scratchBufferFG;
	uf::stl::vector<uint8_t> scratchBufferOF;

	FfxFrameInterpolationContext contextFG;
	FfxFrameInterpolationContextDescription contextDescriptionFG;

	FfxOpticalflowContext contextOF;
	FfxOpticalflowContextDescription contextDescriptionOF;
#endif
	struct {
		uf::renderer::Texture empty;
		uf::renderer::Texture output;
	#if UF_USE_FFX_SDK == FFX_SDK_3_1
		uf::renderer::Texture outputComposited;
		uf::renderer::Texture dilatedMotionVectors;
		uf::renderer::Texture dilatedDepth;
		uf::renderer::Texture reconstructedPrevNearestDepth;
		uf::renderer::Texture opticalFlowVector;
		uf::renderer::Texture opticalFlowSceneChangeDetection;
	#endif
	} resources;

	void initializeResource( uf::renderer::Texture& resource, uint32_t width = 0, uint32_t height = 0, VkImageUsageFlags usage = 0, VkImageLayout layout = VK_IMAGE_LAYOUT_GENERAL ) {
		if ( width == 0 ) width = uf::renderer::settings::width;
		if ( height == 0 ) height = uf::renderer::settings::height;
		resource.mips = 0;
		resource.destroy();
		resource.fromBuffers(
			NULL,
			0,
			resource.format,
			width, height,
			1,
			1,
			VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | usage,
			layout
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
		if ( texture.layout == VK_IMAGE_LAYOUT_GENERAL ) state = FFX_RESOURCE_STATE_UNORDERED_ACCESS;

		return ffxGetResourceVK(texture.image, desc, (wchar_t*) name, state);
   };
   FfxResource createFfxResource( const uf::renderer::RenderTarget::Attachment& attachment, const wchar_t* name, FfxResourceStates state = FFX_RESOURCE_STATE_COMPUTE_READ ) {
   		uf::renderer::Texture texture;
   		texture.aliasAttachment( attachment );
		return createFfxResource( texture, name, state );
	}
#endif

	void barrier( VkCommandBuffer commandBuffer, VkImage image ) {
		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL; // FFX expects GENERAL
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

		vkCmdPipelineBarrier(
			commandBuffer,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0, 0, nullptr, 0, nullptr, 1, &barrier
		);
	}

	void upscale( VkCommandBuffer commandBuffer ) {
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

		#if UF_USE_FFX_SDK == FFX_SDK_3_1
			dispatchParameters.dilatedMotionVectors = createFfxResource(::resources.dilatedMotionVectors, FSR_DilatedMotionVectors);
			dispatchParameters.dilatedDepth = createFfxResource(::resources.dilatedDepth, FSR_DilatedDepth);
			dispatchParameters.reconstructedPrevNearestDepth = createFfxResource(::resources.reconstructedPrevNearestDepth, FSR_ReconstructedPrevNearestDepth);
		#endif
		
		#if UF_USE_FFX_SDK == FFX_SDK_3
			dispatchParameters.upscaleOutput = createFfxResource(::resources.output, FSR_OutputUpscaledColor );
		#else
			dispatchParameters.output = createFfxResource(::resources.output, FSR_OutputUpscaledColor );
		#endif
	#else
		dispatchParameters.color = ffxGetTextureResourceVK(&::context, attachmentColor.image, attachmentColor.view, renderSize.x, renderSize.y, attachmentColor.descriptor.format, FSR_InputColor );
		dispatchParameters.depth = ffxGetTextureResourceVK(&::context, attachmentDepth.image, attachmentDepth.view, renderSize.x, renderSize.y, attachmentDepth.descriptor.format, FSR_InputDepth );
		dispatchParameters.motionVectors = ffxGetTextureResourceVK(&::context, attachmentMotion.image, attachmentMotion.view, renderSize.x, renderSize.y, attachmentMotion.descriptor.format, FSR_InputMotionVectors );\
		
		dispatchParameters.exposure = ffxGetTextureResourceVK(&::context, nullptr, nullptr, 1, 1, VK_FORMAT_UNDEFINED, FSR_InputExposure );
		dispatchParameters.reactive = ffxGetTextureResourceVK(&::context, nullptr, nullptr, 1, 1, VK_FORMAT_UNDEFINED, FSR_InputReactiveMap );
		dispatchParameters.transparencyAndComposition = ffxGetTextureResourceVK(&::context, nullptr, nullptr, 1, 1, VK_FORMAT_UNDEFINED, FSR_TransparencyAndCompositionMap );
		
		dispatchParameters.output = ffxGetTextureResourceVK(&::context, ::resources.output.image, ::resources.output.view, ::resources.output.width, ::resources.output.height, ::resources.output.format, FSR_OutputUpscaledColor );
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
		barrier(commandBuffer, ::resources.output.image);
	}

	// finish filling out
#if UF_USE_FFX_SDR_FRAME_INTERP
	void framegen( VkCommandBuffer commandBuffer ) {
		if ( !uf::renderer::hasRenderMode("", true) ) return;	
		if ( !uf::renderer::hasRenderMode("Swapchain", true) ) return;
		auto& swapchainRenderMode = uf::renderer::getRenderMode("Swapchain", true);
		auto& renderMode = uf::renderer::getRenderMode("", true);

		pod::Vector2ui renderSize = {
			renderMode.width > 0 ? renderMode.width : (uf::renderer::settings::width * renderMode.scale),
			renderMode.height > 0 ? renderMode.height : (uf::renderer::settings::height * renderMode.scale),
		};
		pod::Vector2ui displaySize = {
			swapchainRenderMode.width > 0 ? swapchainRenderMode.width : uf::renderer::settings::width,
			swapchainRenderMode.height > 0 ? swapchainRenderMode.height : uf::renderer::settings::height,
		};

		auto& scene = uf::scene::getCurrentScene();
		auto& controller = scene.getController();
		auto& camera = controller.getComponent<uf::Camera>();
		auto& projection = camera.getProjection();

		// composite GUI onto output
		{
			::compositor.record( commandBuffer );
			barrier(commandBuffer, ::resources.outputComposited.image);
		}

	#if UF_USE_FFX_SDK == FFX_SDK_3
		FfxFsr3DispatchFrameGenerationPrepareDescription dispatchParameters = {};
		dispatchParameters.commandList = ffxGetCommandListVK(commandBuffer);
	
		// ...		

		FFX_ERROR_CHECK(ffxFsr3ContextDispatchFrameGenerationPrepare(&::context, &dispatchParameters));
	#elif UF_USE_FFX_SDK == FFX_SDK_3_1
		FfxFrameGenerationConfig fgConfig = {};
		fgConfig.swapChain = ffxGetSwapchainVK(uf::renderer::swapchain.swapChain);
		fgConfig.frameGenerationEnabled = true;
		fgConfig.allowAsyncWorkloads = true;

		FFX_ERROR_CHECK(ffxSetFrameGenerationConfigToSwapchainVK(&fgConfig));

		FfxCommandList interpolationCommandList;
		ffxGetFrameinterpolationCommandlistVK(fgConfig.swapChain, interpolationCommandList);

		FfxFrameInterpolationDispatchDescription dispatchParameters = {};
		dispatchParameters.commandList = ffxGetCommandListVK(commandBuffer);

		dispatchParameters.displaySize.width = displaySize.x;
		dispatchParameters.displaySize.height = displaySize.y;
		dispatchParameters.renderSize.width = displaySize.x;
		dispatchParameters.renderSize.height = displaySize.y;

		// use output from rendermode
		if ( !ext::fsr::frameUpscale ) {
			if ( !renderMode.hasAttachment("output") && !renderMode.hasAttachment("color") ) return;
			auto& attachmentColor = renderMode.hasAttachment("output") ? renderMode.getAttachment("output") : renderMode.getAttachment("color");
			dispatchParameters.currentBackBuffer_HUDLess = createFfxResource(attachmentColor, L"FSR3_InterpolationSource_HUDLess");
		} else {
			dispatchParameters.currentBackBuffer_HUDLess = createFfxResource(::resources.output, L"FSR3_InterpolationSource_HUDLess");
		}
		// attach HUD'd image
		dispatchParameters.currentBackBuffer = createFfxResource(::resources.outputComposited, L"FSR3_InterpolationSource");

		dispatchParameters.output = ffxGetFrameinterpolationTextureVK( ffxGetSwapchainVK(uf::renderer::swapchain.swapChain) );

		dispatchParameters.cameraNear = projection(2,3);
		dispatchParameters.cameraFar = FLT_MAX;
		dispatchParameters.cameraFovAngleVertical = 2.0f * std::atan(1.0f / fabs(projection(1,1)));
		dispatchParameters.viewSpaceToMetersFactor = 1.0f;

		dispatchParameters.frameTimeDelta = uf::time::delta * 1000.0f;
		dispatchParameters.reset = uf::renderer::states::frameAccumulateReset;

		static uint64_t frameID = 0;
		dispatchParameters.frameID = frameID++;

		dispatchParameters.backBufferTransferFunction = uf::renderer::settings::pipelines::hdr ? FFX_BACKBUFFER_TRANSFER_FUNCTION_PQ : FFX_BACKBUFFER_TRANSFER_FUNCTION_SRGB;
		dispatchParameters.minMaxLuminance[0] = 0.0f;
		dispatchParameters.minMaxLuminance[1] = 1000.0f;

		dispatchParameters.dilatedDepth = createFfxResource(::resources.dilatedDepth, FSR_DilatedDepth);
		dispatchParameters.dilatedMotionVectors = createFfxResource(::resources.dilatedMotionVectors, FSR_DilatedMotionVectors);
		dispatchParameters.reconstructedPrevDepth = createFfxResource(::resources.reconstructedPrevNearestDepth, FSR_ReconstructedPrevNearestDepth);

		dispatchParameters.opticalFlowVector = createFfxResource(::resources.opticalFlowVector, L"OF_Vector");
		dispatchParameters.opticalFlowSceneChangeDetection = createFfxResource(::resources.opticalFlowSceneChangeDetection, L"OF_SCD");
		dispatchParameters.opticalFlowScale.x = 1.0f;
		dispatchParameters.opticalFlowScale.y = 1.0f;
		dispatchParameters.opticalFlowBlockSize = FFX_FSR_BLOCK_SIZE;
		dispatchParameters.commandList = interpolationCommandList;

		FFX_ERROR_CHECK(ffxFrameInterpolationDispatch(&::contextFG, &dispatchParameters));
	#endif
	}
#endif

#if UF_USE_FFX_SDK == FFX_SDK_3_1
	void computeOpticalFlow( VkCommandBuffer commandBuffer ) {
		FfxOpticalflowDispatchDescription dispatchParameters = {};
		dispatchParameters.commandList = ffxGetCommandListVK(commandBuffer);

		// use output from rendermode
		if ( !ext::fsr::frameUpscale ) {
			if ( !uf::renderer::hasRenderMode("", true) ) return;	
			auto& renderMode = uf::renderer::getRenderMode("", true);
			if ( !renderMode.hasAttachment("output") && !renderMode.hasAttachment("color") ) return;
			auto& attachmentColor = renderMode.hasAttachment("output") ? renderMode.getAttachment("output") : renderMode.getAttachment("color");
			dispatchParameters.color = createFfxResource(attachmentColor, L"OF_InputColor");
		} else {
			dispatchParameters.color = createFfxResource(::resources.output, L"OF_InputColor");
		}
		dispatchParameters.reset = uf::renderer::states::frameAccumulateReset;

		dispatchParameters.backbufferTransferFunction = uf::renderer::settings::pipelines::hdr ? FFX_BACKBUFFER_TRANSFER_FUNCTION_PQ : FFX_BACKBUFFER_TRANSFER_FUNCTION_SRGB;
		dispatchParameters.minMaxLuminance.x = 0.0f;
		dispatchParameters.minMaxLuminance.y = 1000.0f;

		dispatchParameters.opticalFlowVector = createFfxResource(::resources.opticalFlowVector, L"OF_Vector");
		dispatchParameters.opticalFlowSCD = createFfxResource(::resources.opticalFlowSceneChangeDetection, L"OF_SCD");

		FFX_ERROR_CHECK(ffxOpticalflowContextDispatch(&::contextOF, &dispatchParameters));

		barrier(commandBuffer, ::resources.opticalFlowVector.image);
		barrier(commandBuffer, ::resources.opticalFlowSceneChangeDetection.image);
	}
#endif
}

uf::stl::string ext::fsr::preset = "native";
pod::Vector2f ext::fsr::jitter = {};
float ext::fsr::sharpness = 1.0f;
float ext::fsr::jitterScale = 2.0f;
bool ext::fsr::initialized = false;
bool ext::fsr::frameUpscale = true;
bool ext::fsr::frameInterpolation = true;

void ext::fsr::initialize() {
	// setup scratch buffer
	{
		auto scratchSize = ffxGetScratchMemorySizeVK(uf::renderer::device.physicalDevice, MAX(uf::renderer::device.extensions.properties.device.size(), 1) );
		if ( ext::fsr::frameUpscale ) {
			::scratchBuffer.resize(scratchSize);
		}
		if ( ext::fsr::frameInterpolation ) {
		#if UF_USE_FFX_SDR_FRAME_INTERP
			::scratchBufferFG.resize(scratchSize);
		#endif
		#if UF_USE_FFX_SDK == FFX_SDK_3_1
			::scratchBufferOF.resize(scratchSize);
		#endif
		}
	}

	// setup context description
	if ( ext::fsr::frameUpscale ) {
		::contextDescription.maxRenderSize.width = FFX_FSR_MAX_WIDTH;
		::contextDescription.maxRenderSize.height = FFX_FSR_MAX_HEIGHT;
	#if UF_USE_FFX_SDK >= FFX_SDK_3
		::contextDescription.maxUpscaleSize.width = uf::renderer::settings::width;
		::contextDescription.maxUpscaleSize.height = uf::renderer::settings::height;
	#else
		::contextDescription.displaySize.width = uf::renderer::settings::width;
		::contextDescription.displaySize.height = uf::renderer::settings::height;
	#endif
		
		// to-do: validate if motion vectors are rendered with jitter
		//::contextDescription.flags = FFX_FSR_ENABLE_AUTO_EXPOSURE | FFX_FSR_ENABLE_MOTION_VECTORS_JITTER_CANCELLATION;
	#if UF_USE_FFX_SDK == FFX_SDK_3
		if ( !ext::fsr::frameInterpolation ) ::contextDescription.flags |= FFX_FSR3_ENABLE_UPSCALING_ONLY;
		::contextDescription.backBufferFormat = ffxGetSurfaceFormatVK(ext::vulkan::settings::formats::color);
	#endif

		if ( uf::renderer::settings::pipelines::hdr ) ::contextDescription.flags |= FFX_FSR_ENABLE_HIGH_DYNAMIC_RANGE;
		if ( uf::matrix::reverseInfiniteProjection ) ::contextDescription.flags |= FFX_FSR_ENABLE_DEPTH_INVERTED | FFX_FSR_ENABLE_DEPTH_INFINITE;
	}

#if UF_USE_FFX_SDK
	VkDeviceContext deviceContextVK = {};
	deviceContextVK.vkDevice = uf::renderer::device.logicalDevice;
	deviceContextVK.vkPhysicalDevice = uf::renderer::device.physicalDevice;
	deviceContextVK.vkDeviceProcAddr = vkGetDeviceProcAddr;
	FfxDevice ffxDevice = ffxGetDeviceVK(&deviceContextVK);
#endif

	// setup context
	if ( ext::fsr::frameUpscale ) {
		#if UF_USE_FFX_SDK == FFX_SDK_3
			FFX_ERROR_CHECK(ffxGetInterfaceVK( &::contextDescription.backendInterfaceUpscaling, ffxDevice, ::scratchBuffer.data(), ::scratchBuffer.size(), 1 ));
			FFX_ERROR_CHECK(ffxGetInterfaceVK( &::contextDescription.backendInterfaceSharedResources, ffxDevice, ::scratchBuffer.data(), ::scratchBuffer.size(), 1 ));
			FFX_ERROR_CHECK(ffxGetInterfaceVK( &::contextDescription.backendInterfaceFrameInterpolation, ffxDevice, ::scratchBuffer.data(), ::scratchBuffer.size(), 1 ));
		#elif UF_USE_FFX_SDK
			FFX_ERROR_CHECK(ffxGetInterfaceVK( &::contextDescription.backendInterface, ffxDevice, ::scratchBuffer.data(), ::scratchBuffer.size(), 1 ));
		#else
			FFX_ERROR_CHECK(ffxGetInterfaceVK( &::contextDescription.callbacks, ::scratchBuffer.data(), ::scratchBuffer.size(), uf::renderer::device.physicalDevice, &vkGetDeviceProcAddr ));
		#endif
		FFX_ERROR_CHECK(ffxContextCreate( &::context, &::contextDescription ));
	}

	#if UF_USE_FFX_SDR_FRAME_INTERP && UF_USE_FFX_SDK == FFX_SDK_3_1
	// setup frame interpolation context
	if ( ext::fsr::frameInterpolation ) {
		::contextDescriptionFG.maxRenderSize.width = FFX_FSR_MAX_WIDTH;
		::contextDescriptionFG.maxRenderSize.height = FFX_FSR_MAX_HEIGHT;
		::contextDescriptionFG.displaySize.width = uf::renderer::settings::width;
		::contextDescriptionFG.displaySize.height = uf::renderer::settings::height;
		::contextDescriptionFG.previousInterpolationSourceFormat = ffxGetSurfaceFormatVK( uf::renderer::settings::pipelines::hdr ? uf::renderer::enums::Format::HDR : uf::renderer::enums::Format::SDR );
		::contextDescriptionFG.backBufferFormat = ::contextDescriptionFG.previousInterpolationSourceFormat;
	//	::contextDescriptionFG.backBufferFormat = ffxGetSurfaceFormatVK( ext::vulkan::settings::formats::color );

		// to-do: validate if motion vectors are rendered with jitter
		// ::contextDescriptionFG.flags = FFX_FRAMEINTERPOLATION_ENABLE_DISPLAY_RESOLUTION_MOTION_VECTORS;
		if ( uf::renderer::settings::pipelines::hdr ) ::contextDescriptionFG.flags |= FFX_FRAMEINTERPOLATION_ENABLE_HDR_COLOR_INPUT;
		if ( uf::matrix::reverseInfiniteProjection ) ::contextDescriptionFG.flags |= FFX_FRAMEINTERPOLATION_ENABLE_DEPTH_INVERTED | FFX_FRAMEINTERPOLATION_ENABLE_DEPTH_INFINITE;

		FFX_ERROR_CHECK(ffxGetInterfaceVK( &::contextDescriptionFG.backendInterface, ffxDevice, ::scratchBufferFG.data(), ::scratchBufferFG.size(), 1 ));
		FFX_ERROR_CHECK(ffxFrameInterpolationContextCreate( &::contextFG, &::contextDescriptionFG ));

		FFX_ERROR_CHECK(ffxGetSwapchainReplacementFunctionsVK(ffxDevice, &::ffxSwapchainFuncs));
	}

	// setup optical flow context
	if ( ext::fsr::frameInterpolation ) {
		::contextDescriptionOF.resolution.width = uf::renderer::settings::width;
		::contextDescriptionOF.resolution.height = uf::renderer::settings::height;

		FFX_ERROR_CHECK(ffxGetInterfaceVK( &::contextDescriptionOF.backendInterface, ffxDevice, ::scratchBufferOF.data(), ::scratchBufferOF.size(), 1 ));
		FFX_ERROR_CHECK(ffxOpticalflowContextCreate( &::contextOF, &::contextDescriptionOF ));
	}
	#endif

	// setup resources
	{
		::resources.output.format = uf::renderer::settings::pipelines::hdr ? uf::renderer::enums::Format::HDR : uf::renderer::enums::Format::SDR;
		::initializeResource( ::resources.output );
	#if UF_USE_FFX_SDK == FFX_SDK_3_1
		::resources.outputComposited.format = uf::renderer::settings::pipelines::hdr ? uf::renderer::enums::Format::HDR : uf::renderer::enums::Format::SDR;
		::resources.dilatedMotionVectors.format = uf::renderer::enums::Format::R16G16_SFLOAT;
		::resources.dilatedDepth.format = uf::renderer::enums::Format::R32_SFLOAT;
		::resources.reconstructedPrevNearestDepth.format = uf::renderer::enums::Format::R32_UINT;
		::resources.opticalFlowVector.format = uf::renderer::enums::Format::R16G16_SINT;
		::resources.opticalFlowSceneChangeDetection.format = uf::renderer::enums::Format::R32_UINT;

		uint32_t block_size = FFX_FSR_BLOCK_SIZE;
		uint32_t ofWidth = (uf::renderer::settings::width + block_size) / block_size;
		uint32_t ofHeight = (uf::renderer::settings::height + block_size) / block_size;

		::initializeResource( ::resources.outputComposited );
		::initializeResource( ::resources.dilatedMotionVectors );
		::initializeResource( ::resources.dilatedDepth );
		::initializeResource( ::resources.reconstructedPrevNearestDepth );
		::initializeResource( ::resources.opticalFlowVector, ofWidth, ofHeight );
		::initializeResource( ::resources.opticalFlowSceneChangeDetection, ofWidth, ofHeight );
	#endif
	}

	// setup compositor
	{
		uf::Mesh mesh;
		mesh.vertex.count = 3;

		auto& blitter = ::compositor;
		blitter.device = &uf::renderer::device;
		blitter.material.device = &uf::renderer::device;
		blitter.descriptor.renderMode = "Swapchain";
		blitter.descriptor.subpass = -1;

		blitter.initializeMesh( mesh );

		blitter.material.attachShader(uf::io::resolveURI(uf::io::root+"/shaders/display/compositor/comp.spv"), ext::vulkan::enums::Shader::COMPUTE);

		blitter.material.textures.clear();
		blitter.material.textures.emplace_back().aliasTexture(::resources.output);
		blitter.material.textures.emplace_back().aliasTexture(uf::renderer::Texture2D::empty);
		blitter.material.textures.emplace_back().aliasTexture(::resources.outputComposited);

		blitter.descriptor.bind.width = uf::renderer::settings::width;
		blitter.descriptor.bind.height = uf::renderer::settings::height;
		blitter.descriptor.bind.point = VK_PIPELINE_BIND_POINT_COMPUTE;

		blitter.update( blitter.descriptor );
	}

	ext::fsr::initialized = true;
}
void ext::fsr::tick() {
	if ( !ext::fsr::initialized ) return;

	pod::Vector2ui renderSize = {};
	pod::Vector2ui displaySize = {};
	
	if ( !uf::renderer::hasRenderMode("", true) ) return;
	if ( !uf::renderer::hasRenderMode("Swapchain", true) ) return;
	
	auto& renderMode = uf::renderer::getRenderMode("", true);
	auto& swapchainRenderMode = uf::renderer::getRenderMode("Swapchain", true);
	
	// update sizes
	{
		renderSize = {
			renderMode.width > 0 ? renderMode.width : (uf::renderer::settings::width * renderMode.scale),
			renderMode.height > 0 ? renderMode.height : (uf::renderer::settings::height * renderMode.scale),
		};
		displaySize = {
			swapchainRenderMode.width > 0 ? swapchainRenderMode.width : uf::renderer::settings::width,
			swapchainRenderMode.height > 0 ? swapchainRenderMode.height : uf::renderer::settings::height,
		};
	}

	if ( uf::renderer::states::resized || ::resources.output.width != displaySize.x || ::resources.output.height != displaySize.y ) {
		// recreate context
	//	uf::renderer::states::rebuild = true;
		if ( ext::fsr::frameUpscale ) {
		#if UF_USE_FFX_SDK >= FFX_SDK_3
			::contextDescription.maxUpscaleSize.width = displaySize.x;
			::contextDescription.maxUpscaleSize.height = displaySize.y;
		#else
			::contextDescription.displaySize.width = displaySize.x;
			::contextDescription.displaySize.height = displaySize.y;
		#endif
			FFX_ERROR_CHECK(ffxContextDestroy( &::context ));
			FFX_ERROR_CHECK(ffxContextCreate( &::context, &::contextDescription ));
		}

		#if UF_USE_FFX_SDR_FRAME_INTERP && UF_USE_FFX_SDK == FFX_SDK_3_1
		// recreate frame interpolation context
		if ( ext::fsr::frameInterpolation ) {
			::contextDescriptionFG.displaySize.width = displaySize.x;
			::contextDescriptionFG.displaySize.height = displaySize.y;

			FFX_ERROR_CHECK(ffxFrameInterpolationContextDestroy( &::contextFG ));
			FFX_ERROR_CHECK(ffxFrameInterpolationContextCreate( &::contextFG, &::contextDescriptionFG ));
		}
		// recreate optical flow context
		if ( ext::fsr::frameInterpolation ) {
			::contextDescriptionOF.resolution.width = displaySize.x;
			::contextDescriptionOF.resolution.height = displaySize.y;

			FFX_ERROR_CHECK(ffxOpticalflowContextDestroy( &::contextOF ));
			FFX_ERROR_CHECK(ffxOpticalflowContextCreate( &::contextOF, &::contextDescriptionOF ));
		}
		#endif

		// recreate resources
		{
			::initializeResource( ::resources.output, displaySize.x, displaySize.y );
		#if UF_USE_FFX_SDK == FFX_SDK_3_1
			uint32_t block_size = FFX_FSR_BLOCK_SIZE;
			uint32_t ofWidth = (displaySize.x + block_size) / block_size;
			uint32_t ofHeight = (displaySize.y + block_size) / block_size;

			::initializeResource( ::resources.outputComposited, displaySize.x, displaySize.y );
			::initializeResource( ::resources.dilatedMotionVectors, displaySize.x, displaySize.y );
			::initializeResource( ::resources.dilatedDepth, displaySize.x, displaySize.y );
			::initializeResource( ::resources.reconstructedPrevNearestDepth, displaySize.x, displaySize.y );
			::initializeResource( ::resources.opticalFlowVector, ofWidth, ofHeight );
			::initializeResource( ::resources.opticalFlowSceneChangeDetection, ofWidth, ofHeight );
		#endif
		}

		{
			auto& blitter = ::compositor;
			blitter.material.textures.clear();
			blitter.material.textures.emplace_back().aliasTexture(::resources.output);
			if ( uf::renderer::hasRenderMode("Gui", true) ) {
				auto& renderMode = uf::renderer::getRenderMode("Gui", true);
				auto& attachment = renderMode.getAttachment("color");
				blitter.material.textures.emplace_back().aliasAttachment( attachment );
			} else {
				blitter.material.textures.emplace_back().aliasTexture( uf::renderer::Texture2D::empty );
			}
			blitter.material.textures.emplace_back().aliasTexture(::resources.outputComposited);

			blitter.descriptor.bind.width = displaySize.x;
			blitter.descriptor.bind.height = displaySize.y;

			blitter.update( blitter.descriptor );
		}
	}

	// update jitter
	{
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
}
void ext::fsr::render() {
	if ( !ext::fsr::initialized ) return;
	auto commandBuffer = uf::renderer::device.fetchCommandBuffer(uf::renderer::QueueEnum::GRAPHICS, true); // immediately flush
	render( commandBuffer );
	uf::renderer::device.flushCommandBuffer(commandBuffer);
}
void ext::fsr::render( VkCommandBuffer commandBuffer ) {
	if ( !ext::fsr::initialized ) return;
	if ( ext::fsr::frameUpscale ) {
		upscale( commandBuffer );
	}
	if ( ext::fsr::frameInterpolation ) {
	#if UF_USE_FFX_SDK == FFX_SDK_3_1
		computeOpticalFlow( commandBuffer );
	#endif
	#if UF_USE_FFX_SDR_FRAME_INTERP
		framegen( commandBuffer );
	#endif
	}
}
void ext::fsr::terminate() {
	if ( !ext::fsr::initialized ) return;

	// destroy context
	if ( ext::fsr::frameUpscale ) {
		FFX_ERROR_CHECK(ffxContextDestroy( &::context ));
	}
	// destroy framegen context
#if UF_USE_FFX_SDR_FRAME_INTERP && UF_USE_FFX_SDK == FFX_SDK_3_1
	if ( ext::fsr::frameInterpolation ) {
		FFX_ERROR_CHECK(ffxFrameInterpolationContextDestroy( &::contextFG ));
	}
#endif

	// destroy resources
	{
		::resources.output.destroy();
	#if UF_USE_FFX_SDK == FFX_SDK_3_1
		::resources.outputComposited.destroy();
		::resources.dilatedMotionVectors.destroy();
		::resources.dilatedDepth.destroy();
		::resources.reconstructedPrevNearestDepth.destroy();
		::resources.opticalFlowVector.destroy();
		::resources.opticalFlowSceneChangeDetection.destroy();
	#endif
	}

	// destroy compositor
	{
		::compositor.destroy();
	}
}

VkResult ext::fsr::acquireNextImage( uint32_t* imageIndex, VkSemaphore presentCompleteSemaphore, VkFence acquireFence ) {
#if UF_USE_FFX_SDR_FRAME_INTERP && UF_USE_FFX_SDK == FFX_SDK_3_1
	if ( ext::fsr::frameInterpolation && ::ffxSwapchainFuncs.acquireNextImageKHR ) {
		return ::ffxSwapchainFuncs.acquireNextImageKHR( uf::renderer::device, uf::renderer::swapchain.swapChain, VK_DEFAULT_FENCE_TIMEOUT, presentCompleteSemaphore, acquireFence, imageIndex );
	}
#endif
	return vkAcquireNextImageKHR( uf::renderer::device, uf::renderer::swapchain.swapChain, VK_DEFAULT_FENCE_TIMEOUT, presentCompleteSemaphore, acquireFence, imageIndex );
}
VkResult ext::fsr::queuePresent( VkQueue queue, uint32_t imageIndex, VkSemaphore waitSemaphore ) {
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = NULL;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &uf::renderer::swapchain.swapChain;
	presentInfo.pImageIndices = &imageIndex;

	if ( waitSemaphore != VK_NULL_HANDLE ) {
		presentInfo.pWaitSemaphores = &waitSemaphore;
		presentInfo.waitSemaphoreCount = 1;
	}

#if UF_USE_FFX_SDR_FRAME_INTERP && UF_USE_FFX_SDK == FFX_SDK_3_1
	if ( ext::fsr::frameInterpolation && ::ffxSwapchainFuncs.queuePresentKHR ) {
		return ::ffxSwapchainFuncs.queuePresentKHR(queue, &presentInfo);
	}
#endif

	return vkQueuePresentKHR(queue, &presentInfo);
}

VkResult ext::fsr::createSwapchain( VkDevice device, VkSwapchainCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSwapchainKHR* pSwapchain ) {
#if UF_USE_FFX_SDR_FRAME_INTERP && UF_USE_FFX_SDK == FFX_SDK_3_1
	if ( ext::fsr::frameInterpolation && !::ffxSwapchainFuncs.createSwapchainFFX ) {
		VkDeviceContext deviceContextVK = {};
		deviceContextVK.vkDevice = device;
		deviceContextVK.vkPhysicalDevice = uf::renderer::device.physicalDevice;
		deviceContextVK.vkDeviceProcAddr = vkGetDeviceProcAddr;
		FfxDevice ffxDevice = ffxGetDeviceVK(&deviceContextVK);

		FFX_ERROR_CHECK(ffxGetSwapchainReplacementFunctionsVK(ffxDevice, &::ffxSwapchainFuncs));
	}
	if ( ext::fsr::frameInterpolation && ::ffxSwapchainFuncs.createSwapchainFFX ) {
		VkFrameInterpolationInfoFFX fiInfo = {};
		fiInfo.device = device;
		fiInfo.physicalDevice = uf::renderer::device.physicalDevice;
		fiInfo.pAllocator = pAllocator;
		fiInfo.compositionMode = VK_COMPOSITION_MODE_GAME_QUEUE_FFX; // Standard mode

		fiInfo.gameQueue.queue = uf::renderer::device.getQueue( uf::renderer::QueueEnum::GRAPHICS );
		fiInfo.gameQueue.familyIndex = uf::renderer::device.queueFamilyIndices.graphics;
		fiInfo.gameQueue.submitFunc = nullptr;

		fiInfo.presentQueue.queue = uf::renderer::device.getQueue( uf::renderer::QueueEnum::PRESENT );
		fiInfo.presentQueue.familyIndex = uf::renderer::device.queueFamilyIndices.present;
		fiInfo.presentQueue.submitFunc = nullptr;

		fiInfo.asyncComputeQueue.queue = uf::renderer::device.getQueue( uf::renderer::QueueEnum::COMPUTE );
		fiInfo.asyncComputeQueue.familyIndex = uf::renderer::device.queueFamilyIndices.compute;
		fiInfo.asyncComputeQueue.submitFunc = nullptr;

		fiInfo.imageAcquireQueue.queue = uf::renderer::device.getQueue( uf::renderer::QueueEnum::ACQUIRE );
		fiInfo.imageAcquireQueue.familyIndex = uf::renderer::device.queueFamilyIndices.acquire;
		fiInfo.imageAcquireQueue.submitFunc = nullptr;

		return ::ffxSwapchainFuncs.createSwapchainFFX( device, pCreateInfo, pAllocator, pSwapchain, &fiInfo );
	}
#endif
	return vkCreateSwapchainKHR( device, pCreateInfo, pAllocator, pSwapchain );
}
void ext::fsr::destroySwapchain( VkDevice device, VkSwapchainKHR swapchain, const VkAllocationCallbacks* pAllocator ) {
#if UF_USE_FFX_SDR_FRAME_INTERP && UF_USE_FFX_SDK == FFX_SDK_3_1
	if ( ext::fsr::frameInterpolation && ::ffxSwapchainFuncs.destroySwapchainKHR ) {
		::ffxSwapchainFuncs.destroySwapchainKHR(device, swapchain, pAllocator);
		return;
	}
#endif
	vkDestroySwapchainKHR( device, swapchain, pAllocator );
}
VkResult ext::fsr::getSwapchainImages( VkDevice device, VkSwapchainKHR swapchain, uint32_t* pSwapchainImageCount, VkImage* pSwapchainImages ) {
#if UF_USE_FFX_SDR_FRAME_INTERP && UF_USE_FFX_SDK == FFX_SDK_3_1
	if ( ext::fsr::frameInterpolation && ::ffxSwapchainFuncs.getSwapchainImagesKHR ) {
		return ::ffxSwapchainFuncs.getSwapchainImagesKHR(device, swapchain, pSwapchainImageCount, pSwapchainImages);
	}
#endif

	return vkGetSwapchainImagesKHR( device, swapchain, pSwapchainImageCount, pSwapchainImages );
}

pod::Matrix4f ext::fsr::getJitterMatrix() {
	return ::jitterMatrix;
}

uf::renderer::Texture& ext::fsr::getRenderTarget() {
	return ::resources.output;
}

#endif