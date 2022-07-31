#include <uf/ext/ffx/fsr.h>

#if UF_USE_FFX_FSR

#include <ffx_fsr2/ffx_fsr2.h>
#include <ffx_fsr2/vk/ffx_fsr2_vk.h>

#include <uf/utils/renderer/renderer.h>
#include <uf/utils/camera/camera.h>
#include <cfloat>

namespace {
	pod::Matrix4f jitterMatrix = uf::matrix::identity();

	uf::stl::vector<uint8_t> scratchBuffer;
	
	FfxFsr2Context context;
	FfxFsr2ContextDescription contextDescription;

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

	#define FFX_ERROR_CHECK(f) { auto error = f; if ( error != FFX_OK) UF_MSG_ERROR("FFXFSR2 Error {}: {}", #f, FFX_ERROR_TO_STRING(error)); }

	void draw(VkCommandBuffer commandBuffer, size_t swapchainIndex) {

	#if 0
		// reactive mask
		FfxFsr2GenerateReactiveDescription generateReactiveParameters;
		generateReactiveParameters.commandList = ffxGetCommandListVK(pCommandList);
		generateReactiveParameters.colorOpaqueOnly = ffxGetTextureResourceVK(&::context, cameraSetup.opaqueOnlyColorResource->Resource(), cameraSetup.opaqueOnlyColorResourceView, cameraSetup.opaqueOnlyColorResource->GetWidth(), cameraSetup.opaqueOnlyColorResource->GetHeight(), cameraSetup.opaqueOnlyColorResource->GetFormat(), L"FSR2_OpaqueOnlyColorResource");
		generateReactiveParameters.colorPreUpscale = ffxGetTextureResourceVK(&::context, cameraSetup.unresolvedColorResource->Resource(), cameraSetup.unresolvedColorResourceView, cameraSetup.unresolvedColorResource->GetWidth(), cameraSetup.unresolvedColorResource->GetHeight(), cameraSetup.unresolvedColorResource->GetFormat(), L"FSR2_UnresolvedColorResource");
		generateReactiveParameters.outReactive = ffxGetTextureResourceVK(&::context, cameraSetup.reactiveMapResource->Resource(), cameraSetup.reactiveMapResourceView, cameraSetup.reactiveMapResource->GetWidth(), cameraSetup.reactiveMapResource->GetHeight(), cameraSetup.reactiveMapResource->GetFormat(), L"FSR2_InputReactiveMap", FFX_RESOURCE_STATE_GENERIC_READ);

		generateReactiveParameters.renderSize.width = pState->renderWidth;
		generateReactiveParameters.renderSize.height = pState->renderHeight;

		generateReactiveParameters.scale = pState->fFsr2AutoReactiveScale;
		generateReactiveParameters.cutoffThreshold = pState->fFsr2AutoReactiveThreshold;
		generateReactiveParameters.flags = (pState->bFsr2AutoReactiveTonemap ? FFX_FSR2_AUTOREACTIVEFLAGS_APPLY_TONEMAP : 0) |
		(pState->bFsr2AutoReactiveInverseTonemap ? FFX_FSR2_AUTOREACTIVEFLAGS_APPLY_INVERSETONEMAP : 0) |
		(pState->bFsr2AutoReactiveThreshold ? FFX_FSR2_AUTOREACTIVEFLAGS_APPLY_THRESHOLD : 0) |
		(pState->bFsr2AutoReactiveUseMax ? FFX_FSR2_AUTOREACTIVEFLAGS_USE_COMPONENTS_MAX : 0);

		ffxFsr2ContextGenerateReactiveMask(&::context, &generateReactiveParameters);
	#endif
	#if 1
		FfxFsr2DispatchDescription dispatchParameters = {};
		dispatchParameters.commandList = ffxGetCommandListVK(commandBuffer);
		
		if ( uf::renderer::settings::pipelines::rt ) {
			if ( !uf::renderer::hasRenderMode("Compute:RT", true) ) return;

			auto& renderMode = uf::renderer::getRenderMode("Compute:RT", true);
			pod::Vector2ui renderSize = {
				renderMode.width > 0 ? renderMode.width : uf::renderer::settings::width,
				renderMode.height > 0 ? renderMode.height : uf::renderer::settings::height,
			};
			dispatchParameters.renderSize.width = renderSize.x;
			dispatchParameters.renderSize.height = renderSize.y;

			auto& attachmentColor = renderMode.hasAttachment("output") ? renderMode.getAttachment("output") : renderMode.getAttachment("color");
			auto& attachmentDepth = renderMode.getAttachment("depth");
			auto& attachmentMotion = renderMode.getAttachment("motion");

			dispatchParameters.color = ffxGetTextureResourceVK(&::context,
				attachmentColor.image,
				attachmentColor.view,
				renderSize.x,
				renderSize.y,
				attachmentColor.descriptor.format,
				L"FSR2_InputColor"
			);
			dispatchParameters.depth = ffxGetTextureResourceVK(&::context,
				attachmentDepth.image,
				attachmentDepth.view,
				renderSize.x,
				renderSize.y,
				attachmentDepth.descriptor.format,
				L"FSR2_InputDepth"
			);
			dispatchParameters.motionVectors = ffxGetTextureResourceVK(&::context,
				attachmentMotion.image,
				attachmentMotion.view,
				renderSize.x,
				renderSize.y,
				attachmentMotion.descriptor.format,
				L"FSR2_InputMotionVectors"
			);
			dispatchParameters.motionVectorScale.x = renderSize.x;
			dispatchParameters.motionVectorScale.y = renderSize.y;
		} else {
			if ( !uf::renderer::hasRenderMode("", true) ) return;
			
			auto& renderMode = uf::renderer::getRenderMode("", true);
			pod::Vector2ui renderSize = {
				renderMode.width > 0 ? renderMode.width : uf::renderer::settings::width,
				renderMode.height > 0 ? renderMode.height : uf::renderer::settings::height,
			};
			dispatchParameters.renderSize.width = renderSize.x;
			dispatchParameters.renderSize.height = renderSize.y;

			auto& attachmentColor = renderMode.hasAttachment("output") ? renderMode.getAttachment("output") : renderMode.getAttachment("color");
			auto& attachmentDepth = renderMode.getAttachment("depth");
			auto& attachmentMotion = renderMode.getAttachment("motion");

			dispatchParameters.color = ffxGetTextureResourceVK(&::context,
				attachmentColor.image,
				attachmentColor.view,
				renderSize.x,
				renderSize.y,
				attachmentColor.descriptor.format,
				L"FSR2_InputColor"
			);
			dispatchParameters.depth = ffxGetTextureResourceVK(&::context,
				attachmentDepth.image,
				attachmentDepth.view,
				renderSize.x,
				renderSize.y,
				attachmentDepth.descriptor.format,
				L"FSR2_InputDepth"
			);
			dispatchParameters.motionVectors = ffxGetTextureResourceVK(&::context,
				attachmentMotion.image,
				attachmentMotion.view,
				renderSize.x,
				renderSize.y,
				attachmentMotion.descriptor.format,
				L"FSR2_InputMotionVectors"
			);
			dispatchParameters.motionVectorScale.x = renderSize.x;
			dispatchParameters.motionVectorScale.y = renderSize.y;
		}
		{
			if ( !uf::renderer::hasRenderMode("Swapchain", true) ) return;

			auto& swapchainRenderMode = uf::renderer::getRenderMode("Swapchain", true);
			auto& attachmentOutput = swapchainRenderMode.getAttachment("color["+std::to_string(swapchainIndex)+"]");
			pod::Vector2ui displaySize = {
				swapchainRenderMode.width, // uf::renderer::settings::width
				swapchainRenderMode.height, // uf::renderer::settings::height
			};

			dispatchParameters.output = ffxGetTextureResourceVK(&::context,
				attachmentOutput.image,
				attachmentOutput.view,
				displaySize.x,
				displaySize.y,
				attachmentOutput.descriptor.format,
				L"FSR2_OutputUpscaledColor", FFX_RESOURCE_STATE_UNORDERED_ACCESS
			);
		}

		dispatchParameters.exposure = ffxGetTextureResourceVK(&::context,
			nullptr,
			nullptr,
			1,
			1,
			VK_FORMAT_UNDEFINED,
			L"FSR2_InputExposure"
		);
		dispatchParameters.reactive = ffxGetTextureResourceVK(&::context,
			nullptr,
			nullptr,
			1,
			1,
			VK_FORMAT_UNDEFINED,
			L"FSR2_InputReactiveMap"
		);
		dispatchParameters.transparencyAndComposition = ffxGetTextureResourceVK(&::context,
			nullptr,
			nullptr,
			1,
			1,
			VK_FORMAT_UNDEFINED,
			L"FSR2_TransparencyAndCompositionMap"
		);

		auto& scene = uf::scene::getCurrentScene();
		auto& controller = scene.getController();
		auto& camera = controller.getComponent<uf::Camera>();

		dispatchParameters.jitterOffset.x = ext::fsr::jitter.x; // m_JitterX;
		dispatchParameters.jitterOffset.y = ext::fsr::jitter.y; // m_JitterY;

		dispatchParameters.reset = false; // uf::renderer::states::frameAccumulateReset; // pState->bReset;
		dispatchParameters.enableSharpening = ext::fsr::sharpness > 0.0f; // pState->bUseRcas;
		dispatchParameters.sharpness = ext::fsr::sharpness; //1.0f; // pState->sharpening;
		dispatchParameters.frameTimeDelta = uf::time::delta;
		dispatchParameters.preExposure = 1.0f;
		dispatchParameters.cameraFar = FLT_MAX; // pState->camera.GetFarPlane();
		dispatchParameters.cameraNear = 0.01f; // pState->camera.GetNearPlane();
		dispatchParameters.cameraFovAngleVertical = 1.5708f; // pState->camera.GetFovV();

		FFX_ERROR_CHECK(ffxFsr2ContextDispatch(&::context, &dispatchParameters));
	#endif
	}
}

pod::Vector2f ext::fsr::jitter = {};
float ext::fsr::sharpness = 1.0f;
bool ext::fsr::initialized = false;

void UF_API ext::fsr::initialize() {
	ffxFsr2SetInstanceFunctions( uf::renderer::device.instance, vkGetInstanceProcAddr );

	scratchBuffer.resize(ffxFsr2GetScratchMemorySizeVK(uf::renderer::device.physicalDevice, uf::renderer::device.extensionProperties.device.size()));

	::contextDescription.device = ffxGetDeviceVK(uf::renderer::device.logicalDevice);
	::contextDescription.maxRenderSize.width = 3840; // uf::renderer::settings::width;
	::contextDescription.maxRenderSize.height = 2160; // uf::renderer::settings::height;
	::contextDescription.displaySize.width = uf::renderer::settings::width;
	::contextDescription.displaySize.height = uf::renderer::settings::height;
	::contextDescription.flags = FFX_FSR2_ENABLE_AUTO_EXPOSURE | FFX_FSR2_ENABLE_DYNAMIC_RESOLUTION; // | FFX_FSR2_ENABLE_TEXTURE1D_USAGE;
	if ( uf::renderer::settings::pipelines::hdr ) ::contextDescription.flags |= FFX_FSR2_ENABLE_HIGH_DYNAMIC_RANGE;
	if ( uf::matrix::reverseInfiniteProjection ) ::contextDescription.flags |= FFX_FSR2_ENABLE_DEPTH_INVERTED | FFX_FSR2_ENABLE_DEPTH_INFINITE;

	FFX_ERROR_CHECK(ffxFsr2GetInterfaceVK( &::contextDescription.callbacks, ::scratchBuffer.data(), ::scratchBuffer.size(), uf::renderer::device.physicalDevice, &vkGetDeviceProcAddr ));
	FFX_ERROR_CHECK(ffxFsr2ContextCreate( &::context, &::contextDescription ));

	ext::fsr::initialized = true;
//	UF_MSG_DEBUG("{} x {}", ::contextDescription.maxRenderSize.width, ::contextDescription.maxRenderSize.height);
#if 0
	swapchainRenderMode.bindCallback( swapchainRenderMode.CALLBACK_BEGIN, draw);
#endif
}
void UF_API ext::fsr::tick() {
	pod::Vector2ui renderSize = {};
	pod::Vector2ui displaySize = {};

	if ( uf::renderer::settings::pipelines::rt ) {
		if ( !uf::renderer::hasRenderMode("Compute:RT", true) ) return;

		auto& renderMode = uf::renderer::getRenderMode("Compute:RT", true);
		auto& blitter = *renderMode.getBlitter(); if ( !blitter.material.hasShader("fragment") ) return;
		auto& shader = blitter.material.getShader("fragment"); if ( shader.textures.empty() ) return;

		auto& attachmentColor = shader.textures.front();
		renderSize = {
			attachmentColor.width > 0 ? attachmentColor.width : uf::renderer::settings::width,
			attachmentColor.height > 0 ? attachmentColor.height : uf::renderer::settings::height,
		};
	} else {
		if ( !uf::renderer::hasRenderMode("", true) ) return;
		
		auto& renderMode = uf::renderer::getRenderMode("", true);
		renderSize = {
			renderMode.width > 0 ? renderMode.width : uf::renderer::settings::width,
			renderMode.height > 0 ? renderMode.height : uf::renderer::settings::height,
		};
	}

	{
		if ( !uf::renderer::hasRenderMode("Swapchain", true) ) return;

		auto& swapchainRenderMode = uf::renderer::getRenderMode("Swapchain", true);
		displaySize = {
			swapchainRenderMode.width, // uf::renderer::settings::width
			swapchainRenderMode.height, // uf::renderer::settings::height
		};
	}

	const int32_t jitterPhaseCount = ffxFsr2GetJitterPhaseCount(renderSize.x, displaySize.x);
	static uint32_t index = 0;
	if ( ++index > jitterPhaseCount ) index = 0;

	ffxFsr2GetJitterOffset(&ext::fsr::jitter.x, &ext::fsr::jitter.x, index, jitterPhaseCount);

	pod::Vector2f jitter = {};
	jitter.x = 2.0f * jitter.x / (float) renderSize.x;
	jitter.y = 2.0f * jitter.y / (float) renderSize.y;
	::jitterMatrix = uf::matrix::translate( uf::matrix::identity(), pod::Vector3f{ jitter.x, jitter.y, 0 } );
}
void UF_API ext::fsr::render() {
	if ( !ext::fsr::initialized ) return;
	auto commandBuffer = uf::renderer::device.fetchCommandBuffer(uf::renderer::QueueEnum::GRAPHICS, true);
	draw(commandBuffer, uf::renderer::states::currentBuffer);
	uf::renderer::device.flushCommandBuffer(commandBuffer);

}
void UF_API ext::fsr::terminate() {
	if ( !ext::fsr::initialized ) return;
	FFX_ERROR_CHECK(ffxFsr2ContextDestroy( &::context ));
}

pod::Matrix4f UF_API ext::fsr::getJitterMatrix() {
	return ::jitterMatrix;
}

#endif