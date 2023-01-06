#if UF_USE_VULKAN

#include <uf/ext/vulkan/vulkan.h>
#include <uf/ext/vulkan/device.h>
#include <uf/ext/vulkan/initializers.h>
#include <uf/utils/window/window.h>
#include <uf/utils/string/ext.h>
#include <uf/ext/openvr/openvr.h>

#include <set>
#include <map>

#include <uf/utils/serialize/serializer.h>

PFN_vkCmdSetCheckpointNV ext::vulkan::vkCmdSetCheckpointNV = NULL; // = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(vkGetDeviceProcAddr(device, "vkCreateRayTracingPipelinesKHR"));
PFN_vkGetQueueCheckpointDataNV ext::vulkan::vkGetQueueCheckpointDataNV = NULL; // = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(vkGetDeviceProcAddr(device, "vkCreateRayTracingPipelinesKHR"));

namespace {
	struct DeviceInfo {
		VkPhysicalDevice handle = VK_NULL_HANDLE;
		VkPhysicalDeviceProperties properties;
		VkPhysicalDeviceFeatures features;
		size_t score;
	};

	::DeviceInfo rate( ext::vulkan::Device& device, VkPhysicalDevice handle ) {
		::DeviceInfo deviceInfo{ .handle = handle };

		auto& physicalDevice = deviceInfo.handle;
		auto& deviceProperties = deviceInfo.properties;
		auto& deviceFeatures = deviceInfo.features;
		auto& score = deviceInfo.score;

		vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
		vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

		// Discrete GPUs have a significant performance advantage
		if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) score += 1000;
		{
			score += deviceProperties.limits.maxImageDimension1D;
			score += deviceProperties.limits.maxImageDimension2D;
			score += deviceProperties.limits.maxImageDimension3D;
			score += deviceProperties.limits.maxImageDimensionCube;
			score += deviceProperties.limits.maxImageArrayLayers;
			score += deviceProperties.limits.maxTexelBufferElements;
			score += deviceProperties.limits.maxUniformBufferRange;
			score += deviceProperties.limits.maxStorageBufferRange;
			score += deviceProperties.limits.maxPushConstantsSize;
			score += deviceProperties.limits.maxMemoryAllocationCount;
			score += deviceProperties.limits.maxSamplerAllocationCount;
			score += deviceProperties.limits.bufferImageGranularity;
			score += deviceProperties.limits.sparseAddressSpaceSize;
			score += deviceProperties.limits.maxBoundDescriptorSets;
			score += deviceProperties.limits.maxPerStageDescriptorSamplers;
			score += deviceProperties.limits.maxPerStageDescriptorUniformBuffers;
			score += deviceProperties.limits.maxPerStageDescriptorStorageBuffers;
			score += deviceProperties.limits.maxPerStageDescriptorSampledImages;
			score += deviceProperties.limits.maxPerStageDescriptorStorageImages;
			score += deviceProperties.limits.maxPerStageDescriptorInputAttachments;
			score += deviceProperties.limits.maxPerStageResources;
			score += deviceProperties.limits.maxDescriptorSetSamplers;
			score += deviceProperties.limits.maxDescriptorSetUniformBuffers;
			score += deviceProperties.limits.maxDescriptorSetUniformBuffersDynamic;
			score += deviceProperties.limits.maxDescriptorSetStorageBuffers;
			score += deviceProperties.limits.maxDescriptorSetStorageBuffersDynamic;
			score += deviceProperties.limits.maxDescriptorSetSampledImages;
			score += deviceProperties.limits.maxDescriptorSetStorageImages;
			score += deviceProperties.limits.maxDescriptorSetInputAttachments;
			score += deviceProperties.limits.maxVertexInputAttributes;
			score += deviceProperties.limits.maxVertexInputBindings;
			score += deviceProperties.limits.maxVertexInputAttributeOffset;
			score += deviceProperties.limits.maxVertexInputBindingStride;
			score += deviceProperties.limits.maxVertexOutputComponents;
			score += deviceProperties.limits.maxTessellationGenerationLevel;
			score += deviceProperties.limits.maxTessellationPatchSize;
			score += deviceProperties.limits.maxTessellationControlPerVertexInputComponents;
			score += deviceProperties.limits.maxTessellationControlPerVertexOutputComponents;
			score += deviceProperties.limits.maxTessellationControlPerPatchOutputComponents;
			score += deviceProperties.limits.maxTessellationControlTotalOutputComponents;
			score += deviceProperties.limits.maxTessellationEvaluationInputComponents;
			score += deviceProperties.limits.maxTessellationEvaluationOutputComponents;
			score += deviceProperties.limits.maxGeometryShaderInvocations;
			score += deviceProperties.limits.maxGeometryInputComponents;
			score += deviceProperties.limits.maxGeometryOutputComponents;
			score += deviceProperties.limits.maxGeometryOutputVertices;
			score += deviceProperties.limits.maxGeometryTotalOutputComponents;
			score += deviceProperties.limits.maxFragmentInputComponents;
			score += deviceProperties.limits.maxFragmentOutputAttachments;
			score += deviceProperties.limits.maxFragmentDualSrcAttachments;
			score += deviceProperties.limits.maxFragmentCombinedOutputResources;
			score += deviceProperties.limits.maxComputeSharedMemorySize;
			score += deviceProperties.limits.maxComputeWorkGroupInvocations;
			score += deviceProperties.limits.subPixelPrecisionBits;
			score += deviceProperties.limits.subTexelPrecisionBits;
			score += deviceProperties.limits.mipmapPrecisionBits;
			score += deviceProperties.limits.maxDrawIndexedIndexValue;
			score += deviceProperties.limits.maxDrawIndirectCount;
			score += deviceProperties.limits.maxSamplerLodBias;
			score += deviceProperties.limits.maxSamplerAnisotropy;
			score += deviceProperties.limits.maxViewports;
			score += deviceProperties.limits.viewportSubPixelBits;
			score += deviceProperties.limits.minMemoryMapAlignment;
			score += deviceProperties.limits.minTexelBufferOffsetAlignment;
			score += deviceProperties.limits.minUniformBufferOffsetAlignment;
			score += deviceProperties.limits.minStorageBufferOffsetAlignment;
			score += deviceProperties.limits.minTexelOffset;
			score += deviceProperties.limits.maxTexelOffset;
			score += deviceProperties.limits.minTexelGatherOffset;
			score += deviceProperties.limits.maxTexelGatherOffset;
			score += deviceProperties.limits.minInterpolationOffset;
			score += deviceProperties.limits.maxInterpolationOffset;
			score += deviceProperties.limits.subPixelInterpolationOffsetBits;
			score += deviceProperties.limits.maxFramebufferWidth;
			score += deviceProperties.limits.maxFramebufferHeight;
			score += deviceProperties.limits.maxFramebufferLayers;
			score += deviceProperties.limits.framebufferColorSampleCounts;
			score += deviceProperties.limits.framebufferDepthSampleCounts;
			score += deviceProperties.limits.framebufferStencilSampleCounts;
			score += deviceProperties.limits.framebufferNoAttachmentsSampleCounts;
			score += deviceProperties.limits.maxColorAttachments;
			score += deviceProperties.limits.sampledImageColorSampleCounts;
			score += deviceProperties.limits.sampledImageIntegerSampleCounts;
			score += deviceProperties.limits.sampledImageDepthSampleCounts;
			score += deviceProperties.limits.sampledImageStencilSampleCounts;
			score += deviceProperties.limits.storageImageSampleCounts;
			score += deviceProperties.limits.maxSampleMaskWords;
			score += deviceProperties.limits.timestampComputeAndGraphics;
			score += deviceProperties.limits.timestampPeriod;
			score += deviceProperties.limits.maxClipDistances;
			score += deviceProperties.limits.maxCullDistances;
			score += deviceProperties.limits.maxCombinedClipAndCullDistances;
			score += deviceProperties.limits.discreteQueuePriorities;
			score += deviceProperties.limits.pointSizeGranularity;
			score += deviceProperties.limits.lineWidthGranularity;
			score += deviceProperties.limits.strictLines;
			score += deviceProperties.limits.standardSampleLocations;
			score += deviceProperties.limits.optimalBufferCopyOffsetAlignment;
			score += deviceProperties.limits.optimalBufferCopyRowPitchAlignment;
			score += deviceProperties.limits.nonCoherentAtomSize;
		}
		// Application can't function without geometry shaders
		if ( !deviceFeatures.geometryShader ) return deviceInfo;
		//
		{
			const uf::stl::vector<uf::stl::string> deviceExtensions = {
				VK_KHR_SWAPCHAIN_EXTENSION_NAME
			};
			uint32_t extensionCount;
			vkEnumerateDeviceExtensionProperties( physicalDevice, nullptr, &extensionCount, nullptr );
			uf::stl::vector<VkExtensionProperties> availableExtensions( extensionCount );
			vkEnumerateDeviceExtensionProperties( physicalDevice, nullptr, &extensionCount, availableExtensions.data() );
			std::set<uf::stl::string> requiredExtensions( deviceExtensions.begin(), deviceExtensions.end() );

			for ( const auto& extension : availableExtensions )
				requiredExtensions.erase( extension.extensionName );

			if ( !requiredExtensions.empty() ) return deviceInfo;
		}
		//
		{
			VkSurfaceCapabilitiesKHR capabilities;
			uf::stl::vector<VkSurfaceFormatKHR> formats;
			uf::stl::vector<VkPresentModeKHR> presentModes;

			vkGetPhysicalDeviceSurfaceCapabilitiesKHR( physicalDevice, device.surface, &capabilities );

			uint32_t formatCount;
			vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, device.surface, &formatCount, nullptr);
			if ( formatCount != 0 ) {
				formats.resize( formatCount );
				vkGetPhysicalDeviceSurfaceFormatsKHR( physicalDevice, device.surface, &formatCount, formats.data() );
			}
			uint32_t presentModeCount;
			vkGetPhysicalDeviceSurfacePresentModesKHR( physicalDevice, device.surface, &presentModeCount, nullptr );
			if ( presentModeCount != 0 ) {
				presentModes.resize(presentModeCount);
				vkGetPhysicalDeviceSurfacePresentModesKHR( physicalDevice, device.surface, &presentModeCount, presentModes.data() );
			}
			if ( formats.empty() || presentModes.empty() ) return deviceInfo;
		}
		return deviceInfo;
	}

#if UF_USE_OPENVR
	void VRInstanceExtensions( uf::stl::vector<uf::stl::string>& requested ) {
		if ( !vr::VRCompositor() ) return;
		int32_t nBufferSize = vr::VRCompositor()->GetVulkanInstanceExtensionsRequired( nullptr, 0 );
		if ( nBufferSize < 0 ) return;
		char pExtensionStr[nBufferSize];
		pExtensionStr[0] = 0;
		vr::VRCompositor()->GetVulkanInstanceExtensionsRequired( pExtensionStr, nBufferSize );
		uf::stl::vector<uf::stl::string> extensions = uf::string::split( pExtensionStr, " " );
		for ( auto& str : extensions ) {
			requested.push_back(str);
		}
		// requested.insert( requested.end(), extensions.begin(), extensions.end() );
	}
	void VRDeviceExtensions( VkPhysicalDevice_T* physicalDevice, uf::stl::vector<uf::stl::string>& requested ) {
		if ( !vr::VRCompositor() ) return;
		int32_t nBufferSize = vr::VRCompositor()->GetVulkanDeviceExtensionsRequired( physicalDevice, nullptr, 0 );
		if ( nBufferSize < 0 ) return;
		char pExtensionStr[nBufferSize];
		pExtensionStr[0] = 0;
		vr::VRCompositor()->GetVulkanDeviceExtensionsRequired( physicalDevice , pExtensionStr, nBufferSize );
		uf::stl::vector<uf::stl::string> extensions = uf::string::split( pExtensionStr, " " );
		for ( auto& str : extensions ) requested.push_back(str);
		// requested.insert( requested.end(), extensions.begin(), extensions.end() );
	}
#endif
	void validateRequestedExtensions( const uf::stl::vector<VkExtensionProperties>& extensionProperties, const uf::stl::vector<uf::stl::string>& requestedExtensions, uf::stl::vector<uf::stl::string>& supportedExtensions ) {
		for ( auto& requestedExtension : requestedExtensions ) {
			bool found = false;
			for ( auto& extensionProperty : extensionProperties ) {
				uf::stl::string extensionName = extensionProperty.extensionName;
				if ( requestedExtension != extensionName ) continue;
				if ( std::find( supportedExtensions.begin(), supportedExtensions.end(), extensionName ) != supportedExtensions.end() ) {
					found = true;
					break;
				}
				if ( found ) break;
				found = true;
				supportedExtensions.push_back( extensionName );
				break;
			}
			if ( !found ) UF_MSG_ERROR("Vulkan missing requested extension: {}", requestedExtension);
		}
	}

	void enableRequestedDeviceFeatures( ext::vulkan::Device& device ) {
		ext::json::Value json;

	#define CHECK_FEATURE( NAME )\
		if ( feature == #NAME ) {\
			if ( device.features.NAME == VK_TRUE ) {\
				device.enabledFeatures.NAME = true;\
				VK_VALIDATION_MESSAGE("Enabled feature: {}", feature);\
			} else VK_VALIDATION_MESSAGE("Failed to enable feature: {}", feature);\
		}

		for ( auto& feature : ext::vulkan::settings::requested::deviceFeatures ) {
			CHECK_FEATURE(robustBufferAccess);
			CHECK_FEATURE(fullDrawIndexUint32);
			CHECK_FEATURE(imageCubeArray);
			CHECK_FEATURE(independentBlend);
			CHECK_FEATURE(geometryShader);
			CHECK_FEATURE(tessellationShader);
			CHECK_FEATURE(sampleRateShading);
			CHECK_FEATURE(dualSrcBlend);
			CHECK_FEATURE(logicOp);
			CHECK_FEATURE(multiDrawIndirect);
			CHECK_FEATURE(drawIndirectFirstInstance);
			CHECK_FEATURE(depthClamp);
			CHECK_FEATURE(depthBiasClamp);
			CHECK_FEATURE(fillModeNonSolid);
			CHECK_FEATURE(depthBounds);
			CHECK_FEATURE(wideLines);
			CHECK_FEATURE(largePoints);
			CHECK_FEATURE(alphaToOne);
			CHECK_FEATURE(multiViewport);
			CHECK_FEATURE(samplerAnisotropy);
			CHECK_FEATURE(textureCompressionETC2);
			CHECK_FEATURE(textureCompressionASTC_LDR);
			CHECK_FEATURE(textureCompressionBC);
			CHECK_FEATURE(occlusionQueryPrecise);
			CHECK_FEATURE(pipelineStatisticsQuery);
			CHECK_FEATURE(vertexPipelineStoresAndAtomics);
			CHECK_FEATURE(fragmentStoresAndAtomics);
			CHECK_FEATURE(shaderTessellationAndGeometryPointSize);
			CHECK_FEATURE(shaderImageGatherExtended);
			CHECK_FEATURE(shaderStorageImageExtendedFormats);
			CHECK_FEATURE(shaderStorageImageMultisample);
			CHECK_FEATURE(shaderStorageImageReadWithoutFormat);
			CHECK_FEATURE(shaderStorageImageWriteWithoutFormat);
			CHECK_FEATURE(shaderUniformBufferArrayDynamicIndexing);
			CHECK_FEATURE(shaderSampledImageArrayDynamicIndexing);
			CHECK_FEATURE(shaderStorageBufferArrayDynamicIndexing);
			CHECK_FEATURE(shaderStorageImageArrayDynamicIndexing);
			CHECK_FEATURE(shaderClipDistance);
			CHECK_FEATURE(shaderCullDistance);
			CHECK_FEATURE(shaderFloat64);
			CHECK_FEATURE(shaderInt64);
			CHECK_FEATURE(shaderInt16);
			CHECK_FEATURE(shaderResourceResidency);
			CHECK_FEATURE(shaderResourceMinLod);
			CHECK_FEATURE(sparseBinding);
			CHECK_FEATURE(sparseResidencyBuffer);
			CHECK_FEATURE(sparseResidencyImage2D);
			CHECK_FEATURE(sparseResidencyImage3D);
			CHECK_FEATURE(sparseResidency2Samples);
			CHECK_FEATURE(sparseResidency4Samples);
			CHECK_FEATURE(sparseResidency8Samples);
			CHECK_FEATURE(sparseResidency16Samples);
			CHECK_FEATURE(sparseResidencyAliased);
			CHECK_FEATURE(variableMultisampleRate);
			CHECK_FEATURE(inheritedQueries);
		}
	#undef CHECK_FEATURE

	#define CHECK_FEATURE2( NAME )\
		if ( feature == #NAME ) {\
			if ( device.features2.NAME == VK_TRUE ) {\
				device.enabledFeatures2.NAME = true;\
				VK_VALIDATION_MESSAGE("Enabled feature: {}", feature);\
			} else VK_VALIDATION_MESSAGE("Failed to enable feature: {}", feature);\
		}
	#undef CHECK_FEATURE2
	}
	ext::json::Value retrieveDeviceFeatures( ext::vulkan::Device& device ) {
		ext::json::Value json;

	#define CHECK_FEATURE( NAME )\
		json[#NAME]["supported"] = device.features.NAME;\
		json[#NAME]["enabled"] = device.enabledFeatures.NAME;

		CHECK_FEATURE(robustBufferAccess);
		CHECK_FEATURE(fullDrawIndexUint32);
		CHECK_FEATURE(imageCubeArray);
		CHECK_FEATURE(independentBlend);
		CHECK_FEATURE(geometryShader);
		CHECK_FEATURE(tessellationShader);
		CHECK_FEATURE(sampleRateShading);
		CHECK_FEATURE(dualSrcBlend);
		CHECK_FEATURE(logicOp);
		CHECK_FEATURE(multiDrawIndirect);
		CHECK_FEATURE(drawIndirectFirstInstance);
		CHECK_FEATURE(depthClamp);
		CHECK_FEATURE(depthBiasClamp);
		CHECK_FEATURE(fillModeNonSolid);
		CHECK_FEATURE(depthBounds);
		CHECK_FEATURE(wideLines);
		CHECK_FEATURE(largePoints);
		CHECK_FEATURE(alphaToOne);
		CHECK_FEATURE(multiViewport);
		CHECK_FEATURE(samplerAnisotropy);
		CHECK_FEATURE(textureCompressionETC2);
		CHECK_FEATURE(textureCompressionASTC_LDR);
		CHECK_FEATURE(textureCompressionBC);
		CHECK_FEATURE(occlusionQueryPrecise);
		CHECK_FEATURE(pipelineStatisticsQuery);
		CHECK_FEATURE(vertexPipelineStoresAndAtomics);
		CHECK_FEATURE(fragmentStoresAndAtomics);
		CHECK_FEATURE(shaderTessellationAndGeometryPointSize);
		CHECK_FEATURE(shaderImageGatherExtended);
		CHECK_FEATURE(shaderStorageImageExtendedFormats);
		CHECK_FEATURE(shaderStorageImageMultisample);
		CHECK_FEATURE(shaderStorageImageReadWithoutFormat);
		CHECK_FEATURE(shaderStorageImageWriteWithoutFormat);
		CHECK_FEATURE(shaderUniformBufferArrayDynamicIndexing);
		CHECK_FEATURE(shaderSampledImageArrayDynamicIndexing);
		CHECK_FEATURE(shaderStorageBufferArrayDynamicIndexing);
		CHECK_FEATURE(shaderStorageImageArrayDynamicIndexing);
		CHECK_FEATURE(shaderClipDistance);
		CHECK_FEATURE(shaderCullDistance);
		CHECK_FEATURE(shaderFloat64);
		CHECK_FEATURE(shaderInt64);
		CHECK_FEATURE(shaderInt16);
		CHECK_FEATURE(shaderResourceResidency);
		CHECK_FEATURE(shaderResourceMinLod);
		CHECK_FEATURE(sparseBinding);
		CHECK_FEATURE(sparseResidencyBuffer);
		CHECK_FEATURE(sparseResidencyImage2D);
		CHECK_FEATURE(sparseResidencyImage3D);
		CHECK_FEATURE(sparseResidency2Samples);
		CHECK_FEATURE(sparseResidency4Samples);
		CHECK_FEATURE(sparseResidency8Samples);
		CHECK_FEATURE(sparseResidency16Samples);
		CHECK_FEATURE(sparseResidencyAliased);
		CHECK_FEATURE(variableMultisampleRate);
		CHECK_FEATURE(inheritedQueries);
	#undef CHECK_FEATURE

	#define CHECK_FEATURE2( NAME )\
		json[#NAME]["supported"] = device.features2.NAME;\
		json[#NAME]["enabled"] = device.enabledFeatures2.NAME;
	#undef CHECK_FEATURE2

		return json;
	}

	void enableRequestedDeviceFeatures12( VkPhysicalDeviceVulkan12Features& features, VkPhysicalDeviceVulkan12Features& enabledFeatures ) {
		ext::json::Value json;

	#define CHECK_FEATURE( NAME )\
		if ( feature == #NAME ) {\
			if ( features.NAME == VK_TRUE ) {\
				enabledFeatures.NAME = true;\
				VK_VALIDATION_MESSAGE("Enabled feature: {}", feature);\
			} else VK_VALIDATION_MESSAGE("Failed to enable feature: {}", feature);\
		}

		for ( auto& feature : ext::vulkan::settings::requested::deviceFeatures ) {
			CHECK_FEATURE(samplerMirrorClampToEdge);
			CHECK_FEATURE(drawIndirectCount);
			CHECK_FEATURE(storageBuffer8BitAccess);
			CHECK_FEATURE(uniformAndStorageBuffer8BitAccess);
			CHECK_FEATURE(storagePushConstant8);
			CHECK_FEATURE(shaderBufferInt64Atomics);
			CHECK_FEATURE(shaderSharedInt64Atomics);
			CHECK_FEATURE(shaderFloat16);
			CHECK_FEATURE(shaderInt8);
			CHECK_FEATURE(descriptorIndexing);
			CHECK_FEATURE(shaderInputAttachmentArrayDynamicIndexing);
			CHECK_FEATURE(shaderUniformTexelBufferArrayDynamicIndexing);
			CHECK_FEATURE(shaderStorageTexelBufferArrayDynamicIndexing);
			CHECK_FEATURE(shaderUniformBufferArrayNonUniformIndexing);
			CHECK_FEATURE(shaderSampledImageArrayNonUniformIndexing);
			CHECK_FEATURE(shaderStorageBufferArrayNonUniformIndexing);
			CHECK_FEATURE(shaderStorageImageArrayNonUniformIndexing);
			CHECK_FEATURE(shaderInputAttachmentArrayNonUniformIndexing);
			CHECK_FEATURE(shaderUniformTexelBufferArrayNonUniformIndexing);
			CHECK_FEATURE(shaderStorageTexelBufferArrayNonUniformIndexing);
			CHECK_FEATURE(descriptorBindingUniformBufferUpdateAfterBind);
			CHECK_FEATURE(descriptorBindingSampledImageUpdateAfterBind);
			CHECK_FEATURE(descriptorBindingStorageImageUpdateAfterBind);
			CHECK_FEATURE(descriptorBindingStorageBufferUpdateAfterBind);
			CHECK_FEATURE(descriptorBindingUniformTexelBufferUpdateAfterBind);
			CHECK_FEATURE(descriptorBindingStorageTexelBufferUpdateAfterBind);
			CHECK_FEATURE(descriptorBindingUpdateUnusedWhilePending);
			CHECK_FEATURE(descriptorBindingPartiallyBound);
			CHECK_FEATURE(descriptorBindingVariableDescriptorCount);
			CHECK_FEATURE(runtimeDescriptorArray);
			CHECK_FEATURE(samplerFilterMinmax);
			CHECK_FEATURE(scalarBlockLayout);
			CHECK_FEATURE(imagelessFramebuffer);
			CHECK_FEATURE(uniformBufferStandardLayout);
			CHECK_FEATURE(shaderSubgroupExtendedTypes);
			CHECK_FEATURE(separateDepthStencilLayouts);
			CHECK_FEATURE(hostQueryReset);
			CHECK_FEATURE(timelineSemaphore);
			CHECK_FEATURE(bufferDeviceAddress);
			CHECK_FEATURE(bufferDeviceAddressCaptureReplay);
			CHECK_FEATURE(bufferDeviceAddressMultiDevice);
			CHECK_FEATURE(vulkanMemoryModel);
			CHECK_FEATURE(vulkanMemoryModelDeviceScope);
			CHECK_FEATURE(vulkanMemoryModelAvailabilityVisibilityChains);
			CHECK_FEATURE(shaderOutputViewportIndex);
			CHECK_FEATURE(shaderOutputLayer);
			CHECK_FEATURE(subgroupBroadcastDynamicId);
		}
	#undef CHECK_FEATURE

	#define CHECK_FEATURE2( NAME )\
		if ( feature == #NAME ) {\
			if ( device.features2.NAME == VK_TRUE ) {\
				device.enabledFeatures2.NAME = true;\
				VK_VALIDATION_MESSAGE("Enabled feature: {}", feature);\
			} else VK_VALIDATION_MESSAGE("Failed to enable feature: {}", feature);\
		}
	#undef CHECK_FEATURE2
	}
	ext::json::Value retrieveDeviceFeatures12( ext::vulkan::Device& device, VkPhysicalDeviceVulkan12Features& features, VkPhysicalDeviceVulkan12Features& enabledFeatures ) {
		ext::json::Value json;

	#define CHECK_FEATURE( NAME )\
		json[#NAME]["supported"] = features.NAME;\
		json[#NAME]["enabled"] = enabledFeatures.NAME;

		CHECK_FEATURE(samplerMirrorClampToEdge);
		CHECK_FEATURE(drawIndirectCount);
		CHECK_FEATURE(storageBuffer8BitAccess);
		CHECK_FEATURE(uniformAndStorageBuffer8BitAccess);
		CHECK_FEATURE(storagePushConstant8);
		CHECK_FEATURE(shaderBufferInt64Atomics);
		CHECK_FEATURE(shaderSharedInt64Atomics);
		CHECK_FEATURE(shaderFloat16);
		CHECK_FEATURE(shaderInt8);
		CHECK_FEATURE(descriptorIndexing);
		CHECK_FEATURE(shaderInputAttachmentArrayDynamicIndexing);
		CHECK_FEATURE(shaderUniformTexelBufferArrayDynamicIndexing);
		CHECK_FEATURE(shaderStorageTexelBufferArrayDynamicIndexing);
		CHECK_FEATURE(shaderUniformBufferArrayNonUniformIndexing);
		CHECK_FEATURE(shaderSampledImageArrayNonUniformIndexing);
		CHECK_FEATURE(shaderStorageBufferArrayNonUniformIndexing);
		CHECK_FEATURE(shaderStorageImageArrayNonUniformIndexing);
		CHECK_FEATURE(shaderInputAttachmentArrayNonUniformIndexing);
		CHECK_FEATURE(shaderUniformTexelBufferArrayNonUniformIndexing);
		CHECK_FEATURE(shaderStorageTexelBufferArrayNonUniformIndexing);
		CHECK_FEATURE(descriptorBindingUniformBufferUpdateAfterBind);
		CHECK_FEATURE(descriptorBindingSampledImageUpdateAfterBind);
		CHECK_FEATURE(descriptorBindingStorageImageUpdateAfterBind);
		CHECK_FEATURE(descriptorBindingStorageBufferUpdateAfterBind);
		CHECK_FEATURE(descriptorBindingUniformTexelBufferUpdateAfterBind);
		CHECK_FEATURE(descriptorBindingStorageTexelBufferUpdateAfterBind);
		CHECK_FEATURE(descriptorBindingUpdateUnusedWhilePending);
		CHECK_FEATURE(descriptorBindingPartiallyBound);
		CHECK_FEATURE(descriptorBindingVariableDescriptorCount);
		CHECK_FEATURE(runtimeDescriptorArray);
		CHECK_FEATURE(samplerFilterMinmax);
		CHECK_FEATURE(scalarBlockLayout);
		CHECK_FEATURE(imagelessFramebuffer);
		CHECK_FEATURE(uniformBufferStandardLayout);
		CHECK_FEATURE(shaderSubgroupExtendedTypes);
		CHECK_FEATURE(separateDepthStencilLayouts);
		CHECK_FEATURE(hostQueryReset);
		CHECK_FEATURE(timelineSemaphore);
		CHECK_FEATURE(bufferDeviceAddress);
		CHECK_FEATURE(bufferDeviceAddressCaptureReplay);
		CHECK_FEATURE(bufferDeviceAddressMultiDevice);
		CHECK_FEATURE(vulkanMemoryModel);
		CHECK_FEATURE(vulkanMemoryModelDeviceScope);
		CHECK_FEATURE(vulkanMemoryModelAvailabilityVisibilityChains);
		CHECK_FEATURE(shaderOutputViewportIndex);
		CHECK_FEATURE(shaderOutputLayer);
		CHECK_FEATURE(subgroupBroadcastDynamicId);
	#undef CHECK_FEATURE

	#define CHECK_FEATURE2( NAME )\
		json[#NAME]["supported"] = device.features2.NAME;\
		json[#NAME]["enabled"] = device.enabledFeatures2.NAME;
	#undef CHECK_FEATURE2

		return json;
	}
}

uint32_t ext::vulkan::Device::getQueueFamilyIndex( VkQueueFlagBits queueFlags ) {
	if ( queueFlags & VK_QUEUE_COMPUTE_BIT ) {
		for ( uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); ++i ) {
			if ( 
				(queueFamilyProperties[i].queueFlags & queueFlags) &&
				((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0)
			)
				return i;
		}
	}
	if ( queueFlags & VK_QUEUE_TRANSFER_BIT ) {
		for ( uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); ++i ) {
			if (
				(queueFamilyProperties[i].queueFlags & queueFlags) && 
				((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) && 
				((queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0)
			)
				return i;
		}
	}
	for ( uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); ++i ) {
		if ( queueFamilyProperties[i].queueFlags & queueFlags )
			return i;
	}
	UF_EXCEPTION("Vulkan error: could not find a matching queue family index");
}
uint32_t ext::vulkan::Device::getMemoryType( uint32_t typeBits, VkMemoryPropertyFlags properties, VkBool32 *memTypeFound ) {
	for ( uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i ) {
		if ( (typeBits & 1) == 1 ) {
			if ( (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties ) {
				if ( memTypeFound ) *memTypeFound = true;
				return i;
			}
		}
		typeBits >>= 1;
	}
	if ( memTypeFound ) {
		*memTypeFound = false;
		return 0;
	}
	UF_EXCEPTION("Vulkan error: could not find a matching memory type");
}

VkCommandBuffer ext::vulkan::Device::createCommandBuffer( VkCommandBufferLevel level, QueueEnum queue, bool begin ){
	VkCommandBufferAllocateInfo cmdBufAllocateInfo = ext::vulkan::initializers::commandBufferAllocateInfo( getCommandPool(queue), level, 1 );

	VkCommandBuffer commandBuffer;
	VK_CHECK_RESULT( vkAllocateCommandBuffers( logicalDevice, &cmdBufAllocateInfo, &commandBuffer ) );
	// If requested, also start recording for the new command buffer
	if ( begin ) {
		VkCommandBufferBeginInfo cmdBufInfo = ext::vulkan::initializers::commandBufferBeginInfo();
		VK_CHECK_RESULT( vkBeginCommandBuffer( commandBuffer, &cmdBufInfo ) );
		UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::BEGIN, "begin" );
	}
	return commandBuffer;
}
void ext::vulkan::Device::flushCommandBuffer( VkCommandBuffer commandBuffer, QueueEnum queueType, bool immediate ) {
	if ( commandBuffer == VK_NULL_HANDLE ) return;

	UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::END, "end" );
	VK_CHECK_RESULT( vkEndCommandBuffer( commandBuffer ) );

	VkFence fence = this->getFence();

	VkSubmitInfo submitInfo = ext::vulkan::initializers::submitInfo();
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	auto queue = getQueue( queueType );
	VK_CHECK_RESULT(vkQueueSubmit( queue, 1, &submitInfo, fence));

	if ( immediate ) {
		VkResult res = vkWaitForFences( this->logicalDevice, 1, &fence, VK_TRUE, VK_DEFAULT_FENCE_TIMEOUT );
		VK_CHECK_QUEUE_CHECKPOINT( queue, res );

		uf::checkpoint::deallocate(checkpoints[commandBuffer]);
		checkpoints[commandBuffer] = NULL;
		checkpoints.erase(commandBuffer);

		this->destroyFence( fence );

		vkFreeCommandBuffers(logicalDevice, getCommandPool( queueType ), 1, &commandBuffer);
	} else {
		ext::vulkan::mutex.lock();
		auto& transient = this->transient.commandBuffers[queueType][std::this_thread::get_id()];
		transient.commandBuffers.emplace_back(commandBuffer);
		transient.fences.emplace_back(fence);
		ext::vulkan::mutex.unlock();
	}
}
pod::Checkpoint* ext::vulkan::Device::markCommandBuffer( VkCommandBuffer commandBuffer, pod::Checkpoint::Type type, const uf::stl::string& name, const uf::stl::string& info ) {
	if ( !ext::vulkan::settings::validation::checkpoints ) return NULL;

	pod::Checkpoint* previous = checkpoints[commandBuffer];
	pod::Checkpoint* pointer = uf::checkpoint::allocate( type, name, info, previous );
	if ( ext::vulkan::vkCmdSetCheckpointNV ) ext::vulkan::vkCmdSetCheckpointNV(commandBuffer, pointer);
	checkpoints[commandBuffer] = pointer;
	return pointer;	
}

ext::vulkan::CommandBuffer ext::vulkan::Device::fetchCommandBuffer( ext::vulkan::QueueEnum queueType, bool immediate ){
	return {
		.immediate = immediate,
		.queueType = queueType,
		.handle = this->createCommandBuffer( VK_COMMAND_BUFFER_LEVEL_PRIMARY, queueType, true ),
		.threadId = std::this_thread::get_id(),
	};
}
void ext::vulkan::Device::flushCommandBuffer( ext::vulkan::CommandBuffer& commandBuffer ) {
//	return this->flushCommandBuffer( commandBuffer.handle, commandBuffer.queueType, commandBuffer.immediate );
	if ( commandBuffer.handle == VK_NULL_HANDLE ) return;

	UF_CHECKPOINT_MARK( commandBuffer.handle, pod::Checkpoint::END, "end" );
	VK_CHECK_RESULT( vkEndCommandBuffer( commandBuffer.handle ) );

	VkFence fence = this->getFence();

	VkSubmitInfo submitInfo = ext::vulkan::initializers::submitInfo();
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer.handle;

	auto queue = getQueue( commandBuffer.queueType, commandBuffer.threadId );
	VK_CHECK_RESULT(vkQueueSubmit( queue, 1, &submitInfo, fence));

	if ( commandBuffer.immediate ) {
		VkResult res = vkWaitForFences( this->logicalDevice, 1, &fence, VK_TRUE, VK_DEFAULT_FENCE_TIMEOUT );
		VK_CHECK_QUEUE_CHECKPOINT( queue, res );

		uf::checkpoint::deallocate(checkpoints[commandBuffer.handle]);
		checkpoints[commandBuffer.handle] = NULL;
		checkpoints.erase(commandBuffer.handle);

		this->destroyFence( fence );

		vkFreeCommandBuffers(logicalDevice, getCommandPool( commandBuffer.queueType, commandBuffer.threadId ), 1, &commandBuffer.handle);
	} else {
		ext::vulkan::mutex.lock();
		auto& transient = this->transient.commandBuffers[commandBuffer.queueType][commandBuffer.threadId];
		transient.commandBuffers.emplace_back(commandBuffer.handle);
		transient.fences.emplace_back(fence);
		ext::vulkan::mutex.unlock();
	}
}

pod::Checkpoint* ext::vulkan::Device::markCommandBuffer( CommandBuffer& commandBuffer, pod::Checkpoint::Type type, const uf::stl::string& name, const uf::stl::string& info ) {
	if ( !ext::vulkan::settings::validation::checkpoints ) return NULL;

	pod::Checkpoint* previous = checkpoints[commandBuffer];
	pod::Checkpoint* pointer = uf::checkpoint::allocate( type, name, info, previous );
	if ( ext::vulkan::vkCmdSetCheckpointNV ) ext::vulkan::vkCmdSetCheckpointNV(commandBuffer, pointer);
	checkpoints[commandBuffer] = pointer;
	return pointer;
}

VkFence ext::vulkan::Device::getFence() {
	VkFence fence;

	if ( !this->reusable.fences.empty() ) {
		fence = this->reusable.fences.top();
		this->reusable.fences.pop();
		return fence;
	}

	VkFenceCreateInfo fenceCreateInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	VK_CHECK_RESULT(vkCreateFence( this->logicalDevice, &fenceCreateInfo, nullptr, &fence ) );
	return fence;
}
void ext::vulkan::Device::destroyFence( VkFence fence ) {
//	vkDestroyFence( this->logicalDevice, fence, nullptr );
	VK_CHECK_RESULT(vkResetFences( this->logicalDevice, 1, &fence ) );
	this->reusable.fences.emplace( fence );
}

ext::vulkan::Buffer ext::vulkan::Device::createBuffer(
	const void* data,
	VkDeviceSize size,
	VkBufferUsageFlags usage,
	VkMemoryPropertyFlags memoryProperties
) {
	ext::vulkan::Buffer buffer;
	VK_CHECK_RESULT(createBuffer(data, size, usage, memoryProperties, buffer));
	return buffer;
}
VkResult ext::vulkan::Device::createBuffer(
	const void* data,
	VkDeviceSize size,
	VkBufferUsageFlags usage,
	VkMemoryPropertyFlags memoryProperties,
	ext::vulkan::Buffer& buffer
) {
	buffer.device = this;
	buffer.usage = usage;
	buffer.memoryProperties = memoryProperties;

	// Create the buffer handle
	VkBufferCreateInfo bufferCreateInfo = ext::vulkan::initializers::bufferCreateInfo(usage, size);
	buffer.allocate( bufferCreateInfo );

	if ( data != nullptr ) {
		void* map = buffer.map();
		memcpy(map, data, size);
		buffer.unmap();
	}

	// Initialize a default descriptor that covers the whole buffer size
	buffer.updateDescriptor();

	// Attach the memory to the buffer object
	return VK_SUCCESS;
}
ext::vulkan::Buffer ext::vulkan::Device::fetchTransientBuffer(
	const void* data,
	VkDeviceSize size,
	VkBufferUsageFlags usage,
	VkMemoryPropertyFlags memoryProperties
) {

	ext::vulkan::mutex.lock();
	auto& buffer = this->transient.buffers.emplace_back();
	VK_CHECK_RESULT(this->createBuffer(data, size, usage, memoryProperties, buffer));
	ext::vulkan::mutex.unlock();
	return buffer.alias();
}
VkCommandPool ext::vulkan::Device::getCommandPool( ext::vulkan::QueueEnum queueEnum) {
	return this->getCommandPool( queueEnum, std::this_thread::get_id() );
}
VkQueue ext::vulkan::Device::getQueue( ext::vulkan::QueueEnum queueEnum ) {
	return this->getQueue( queueEnum, std::this_thread::get_id() );
}
VkCommandPool ext::vulkan::Device::getCommandPool( ext::vulkan::QueueEnum queueEnum, std::thread::id id ) {
	uint32_t index{0};
	uf::ThreadUnique<VkCommandPool>* commandPool{NULL};

	switch ( queueEnum ) {
		case QueueEnum::GRAPHICS:
			index = device.queueFamilyIndices.graphics;
			commandPool = &this->commandPool.graphics;
		break;
		case QueueEnum::COMPUTE:
			index = device.queueFamilyIndices.compute;
			commandPool = &this->commandPool.compute;
		break;
		case QueueEnum::TRANSFER:
			index = device.queueFamilyIndices.transfer;
			commandPool = &this->commandPool.transfer;
		break;
	}
	UF_ASSERT( commandPool );
	auto guard = commandPool->guardMutex();
	bool exists = commandPool->has(id);
	VkCommandPool& pool = commandPool->get(id);
	if ( !exists ) {
		VkCommandPoolCreateFlags createFlags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		VkCommandPoolCreateInfo cmdPoolInfo = {};
		cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPoolInfo.queueFamilyIndex = index;
		cmdPoolInfo.flags = createFlags;
		if ( vkCreateCommandPool( this->logicalDevice, &cmdPoolInfo, nullptr, &pool ) != VK_SUCCESS )
			UF_EXCEPTION("Vulkan error: failed to create command pool for graphics!");
	}
	return pool;
}
VkQueue ext::vulkan::Device::getQueue( ext::vulkan::QueueEnum queueEnum, std::thread::id id ) {
	uint32_t index = 0;
	uf::ThreadUnique<VkQueue>* commandPool{NULL};
	switch ( queueEnum ) {
		case QueueEnum::GRAPHICS:
			index = device.queueFamilyIndices.graphics;
			commandPool = &queues.graphics;
		break;
		case QueueEnum::PRESENT:
			index = device.queueFamilyIndices.present;
			commandPool = &queues.present;
		break;
		case QueueEnum::COMPUTE:
			index = device.queueFamilyIndices.compute;
			commandPool = &queues.compute;
		break;
		case QueueEnum::TRANSFER:
			index = device.queueFamilyIndices.transfer;
			commandPool = &queues.transfer;
		break;
	}
	UF_ASSERT( commandPool );
	auto guard = commandPool->guardMutex();
	bool exists = commandPool->has(id);
	VkQueue& queue = commandPool->get(id);
	if ( !exists ) {
		vkGetDeviceQueue( device, index, 0, &queue );
	}
	return queue;
}

void ext::vulkan::Device::initialize() {	
	uf::stl::vector<uf::stl::string> instanceLayers = {
	//	"VK_LAYER_KHRONOS_synchronization2",
	};
	// Assert validation layers
	if ( ext::vulkan::settings::validation::enabled ) instanceLayers.emplace_back("VK_LAYER_KHRONOS_validation");
	
	{
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
		uf::stl::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for ( auto& layerName : instanceLayers ) {
			bool layerFound = false;
			for ( auto& layerProperties : availableLayers ) {
				if ( layerName == layerProperties.layerName ) {
					layerFound = true;
					break;
				}
			}
			if ( layerFound ) {
				VK_VALIDATION_MESSAGE("Vulkan layer enabled: {}", layerName);
			} else UF_EXCEPTION("Vulkan error: layer requested, but not available: {}", layerName);
		}
	}
	// 
	// Get extensions
	uf::stl::vector<uf::stl::string> requestedExtensions = window->getExtensions( ext::vulkan::settings::validation::enabled );
	// Load any requested extensions
	requestedExtensions.insert( requestedExtensions.end(), ext::vulkan::settings::requested::instanceExtensions.begin(), ext::vulkan::settings::requested::instanceExtensions.end() );

#if UF_USE_OPENVR
	// OpenVR Support
	if ( ext::openvr::enabled ) VRInstanceExtensions(requestedExtensions);
#endif
	{
		if ( ext::vulkan::settings::validation::enabled )
			for ( auto ext : requestedExtensions )
				UF_MSG_INFO("Requested instance extension: {}", ext);

		uint32_t extensionsCount = 0;
		uint32_t enabledExtensionsCount = 0;
		
		VK_CHECK_RESULT(vkEnumerateInstanceExtensionProperties( NULL, &extensionsCount, NULL ));
		extensions.properties.instance.resize(extensionsCount);
		VK_CHECK_RESULT( vkEnumerateInstanceExtensionProperties( NULL, &extensionsCount, extensions.properties.instance.data() ) );

		validateRequestedExtensions( extensions.properties.instance, requestedExtensions, extensions.supported.instance );
	}
	// Create instance
	{
		uf::stl::vector<uf::stl::string> instanceExtensions;
		for ( auto& s : extensions.supported.instance ) {
			VK_VALIDATION_MESSAGE("Enabled instance extension: {}", s);
			instanceExtensions.emplace_back( s );
			extensions.enabled.instance[s] = true;
		}

		auto cInstanceExtensions = uf::string::cStrings( instanceExtensions );
		auto cInstanceLayers = uf::string::cStrings( instanceLayers );

		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Program";
		appInfo.pEngineName = "Engine";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 3, 0);
		appInfo.engineVersion = VK_MAKE_VERSION(1, 3, 0);
		appInfo.apiVersion = VK_API_VERSION_1_3;

		if ( uf::renderer::settings::version <= 1.0 ) {
			appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
			appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
			appInfo.apiVersion = VK_API_VERSION_1_0;
		} else if ( uf::renderer::settings::version <= 1.1 ) {
			appInfo.applicationVersion = VK_MAKE_VERSION(1, 1, 0);
			appInfo.engineVersion = VK_MAKE_VERSION(1, 1, 0);
			appInfo.apiVersion = VK_API_VERSION_1_1;
		} else if ( uf::renderer::settings::version <= 1.2 ) {
			appInfo.applicationVersion = VK_MAKE_VERSION(1, 2, 0);
			appInfo.engineVersion = VK_MAKE_VERSION(1, 2, 0);
			appInfo.apiVersion = VK_API_VERSION_1_2;
		} else if ( uf::renderer::settings::version <= 1.3 ) {
			appInfo.applicationVersion = VK_MAKE_VERSION(1, 3, 0);
			appInfo.engineVersion = VK_MAKE_VERSION(1, 3, 0);
			appInfo.apiVersion = VK_API_VERSION_1_3;
		}

		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		createInfo.enabledExtensionCount = static_cast<uint32_t>(cInstanceExtensions.size());
		createInfo.ppEnabledExtensionNames = cInstanceExtensions.data();

		createInfo.enabledLayerCount = static_cast<uint32_t>(cInstanceLayers.size());
		createInfo.ppEnabledLayerNames = cInstanceLayers.data();

		VK_CHECK_RESULT( vkCreateInstance( &createInfo, nullptr, &this->instance ));

		{
			ext::json::Value payload = ext::json::array();
			for ( auto& s : instanceExtensions ) {
				payload.emplace_back( s );
			}
			uf::hooks.call("vulkan:Instance.ExtensionsEnabled", payload);
		}
	}
	// Setup debug
	if ( ext::vulkan::settings::validation::enabled ) {
		VkDebugUtilsMessengerCreateInfoEXT createInfo = {};

		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = ext::vulkan::debugCallback;
		createInfo.pUserData = nullptr; // Optional

		VK_CHECK_RESULT(CreateDebugUtilsMessengerEXT( this->instance, &createInfo, nullptr, &this->debugMessenger ));
	}
	// Create surface
	{
		window->createSurface( instance, surface );
	}
	// Create physical device
	
	uint32_t deviceCount = 0;
	uf::stl::vector<VkPhysicalDevice> physicalDevices;
	uf::stl::vector<::DeviceInfo> deviceInfos; // defined outside if we want to "multi"-gpu
	{
		vkEnumeratePhysicalDevices( this->instance, &deviceCount, nullptr );
		if ( deviceCount == 0 ) UF_EXCEPTION("Vulkan error: failed to find GPUs with Vulkan support!");


		deviceInfos.reserve(deviceCount);
		physicalDevices.resize(deviceCount);

		vkEnumeratePhysicalDevices( this->instance, &deviceCount, physicalDevices.data() );
		
		size_t bestDeviceIndex = 0;
		for ( size_t i = 0; i < deviceCount; ++i ) {
			auto& deviceInfo = deviceInfos.emplace_back( rate(*this, physicalDevices[i]) );
			VK_VALIDATION_MESSAGE("[{}] Found device: {} (score: {} | device ID: {} | vendor ID: {} | API version: {} | driver version: {})", i, deviceInfo.properties.deviceName, deviceInfo.score, deviceInfo.properties.deviceID, deviceInfo.properties.vendorID, deviceInfo.properties.apiVersion, deviceInfo.properties.driverVersion );
		/*
			VK_VALIDATION_MESSAGE("[" << i << "] "
				"Found device: " << deviceInfo.properties.deviceName << " ("
				"score: " << deviceInfo.score << " | "
				"device ID: " << deviceInfo.properties.deviceID << " | "
				"vendor ID: " << deviceInfo.properties.vendorID << " | "
				"API version: " << deviceInfo.properties.apiVersion << " | "
				"driver version: " << deviceInfo.properties.driverVersion << ")"
			);
		*/
			if ( ext::vulkan::settings::gpuID == deviceInfo.properties.deviceID ) {
				bestDeviceIndex = i;
				break;
			}
			
			if ( settings::experimental::enableMultiGPU && deviceInfos[bestDeviceIndex].properties.vendorID != deviceInfo.properties.vendorID ) settings::experimental::enableMultiGPU = false;
			if ( deviceInfos[bestDeviceIndex].score >= deviceInfo.score ) continue;
			bestDeviceIndex = i;
		}
		if ( 0 <= ext::vulkan::settings::gpuID && ext::vulkan::settings::gpuID < deviceCount ) {
			bestDeviceIndex = ext::vulkan::settings::gpuID;
		}
		auto& deviceInfo = deviceInfos[bestDeviceIndex];
		this->physicalDevice = deviceInfo.handle;
		VK_VALIDATION_MESSAGE("Usind device #{}: (score: {} | device ID: {} | vendor ID: {} | API version: {} | driver version: {})", bestDeviceIndex, deviceInfo.properties.deviceName, deviceInfo.score, deviceInfo.properties.deviceID, deviceInfo.properties.vendorID, deviceInfo.properties.apiVersion, deviceInfo.properties.driverVersion );
	/*
		VK_VALIDATION_MESSAGE("Using device #" << bestDeviceIndex << " ("
			"score: " << deviceInfo.score << " | "
			"device ID: " << deviceInfo.properties.deviceID << " | "
			"vendor ID: " << deviceInfo.properties.vendorID << " | "
			"API version: " << deviceInfo.properties.apiVersion << " | "
			"driver version: " << deviceInfo.properties.driverVersion << ")"
		);
	*/
	}
	// Update properties
	{
		{
			vkGetPhysicalDeviceProperties( this->physicalDevice, &properties );
			// Features should be checked by the examples before using them
			vkGetPhysicalDeviceFeatures( this->physicalDevice, &features );
			// Memory properties are used regularly for creating all kinds of buffers
			vkGetPhysicalDeviceMemoryProperties( this->physicalDevice, &memoryProperties );
		}
		{
			properties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
			features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
			memoryProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;

			vkGetPhysicalDeviceProperties2( this->physicalDevice, &properties2 );
			// Features should be checked by the examples before using them
			vkGetPhysicalDeviceFeatures2( this->physicalDevice, &features2 );
			// Memory properties are used regularly for creating all kinds of buffers
			vkGetPhysicalDeviceMemoryProperties2( this->physicalDevice, &memoryProperties2 );
		}
		// Queue family properties, used for setting up requested queues upon device creation
		uint32_t queueFamilyCount;
		vkGetPhysicalDeviceQueueFamilyProperties( this->physicalDevice, &queueFamilyCount, nullptr );
		// assert(queueFamilyCount > 0);
		queueFamilyProperties.resize(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties( this->physicalDevice, &queueFamilyCount, queueFamilyProperties.data() );
		{
			VkSampleCountFlags counts = properties.limits.framebufferColorSampleCounts & properties.limits.framebufferDepthSampleCounts;
			uint8_t maxSamples = 1;
			if (counts & VK_SAMPLE_COUNT_64_BIT) maxSamples = 64;
			else if (counts & VK_SAMPLE_COUNT_32_BIT) maxSamples = 32;
			else if (counts & VK_SAMPLE_COUNT_16_BIT) maxSamples = 16;
			else if (counts & VK_SAMPLE_COUNT_8_BIT) maxSamples = 8;
			else if (counts & VK_SAMPLE_COUNT_4_BIT) maxSamples = 4;
			else if (counts & VK_SAMPLE_COUNT_2_BIT) maxSamples = 2;
			ext::vulkan::settings::msaa = std::min( maxSamples, ext::vulkan::settings::msaa );
		}
	}
	// Create logical device
	{
		bool useSwapChain = true;
		VkQueueFlags requestedQueueTypes = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT;
		uf::stl::vector<uf::stl::string> requestedExtensions;
		requestedExtensions.insert( requestedExtensions.end(), ext::vulkan::settings::requested::deviceExtensions.begin(), ext::vulkan::settings::requested::deviceExtensions.end() );
	#if UF_USE_OPENVR		
		// OpenVR Support
		if ( ext::openvr::enabled ) {
			VRDeviceExtensions( this->physicalDevice, requestedExtensions);
		}
	#endif
		{
			// Allocate enough ExtensionProperties to support all extensions being enabled
			for ( auto ext : requestedExtensions ) VK_VALIDATION_MESSAGE("Requested device extension: {}", ext);

			uint32_t extensionsCount = 0;
			uint32_t enabledExtensionsCount = 0;
			
			VK_CHECK_RESULT(vkEnumerateDeviceExtensionProperties( this->physicalDevice, NULL, &extensionsCount, NULL ));
			extensions.properties.device.resize( extensionsCount );
			VK_CHECK_RESULT( vkEnumerateDeviceExtensionProperties( this->physicalDevice, NULL, &extensionsCount, extensions.properties.device.data() ) );
			
			validateRequestedExtensions( extensions.properties.device, requestedExtensions, extensions.supported.device );
		}
		uf::stl::vector<uf::stl::string> deviceExtensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};
		for ( auto& s : extensions.supported.device ) {
			VK_VALIDATION_MESSAGE("Enabled device extension: {}", s);
			deviceExtensions.emplace_back( s );
			extensions.enabled.device[s] = true;
		}

		// Desired queues need to be requested upon logical device creation
		// Due to differing queue family configurations of Vulkan implementations this can be a bit tricky, especially if the application
		// requests different queue types
		uf::stl::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};
		// Get queue family indices for the requested queue family types
		// Note that the indices may overlap depending on the implementation
		const float defaultQueuePriority(0.0f);
		// Graphics queue
		if ( requestedQueueTypes & VK_QUEUE_GRAPHICS_BIT ) {
			queueFamilyIndices.graphics = getQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT);
			VkDeviceQueueCreateInfo queueInfo{};
			queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueInfo.queueFamilyIndex = queueFamilyIndices.graphics;
			queueInfo.queueCount = 1;
			queueInfo.pQueuePriorities = &defaultQueuePriority;
			queueCreateInfos.push_back(queueInfo);
		} else {
			queueFamilyIndices.graphics = 0; // VK_NULL_HANDLE;
		}
		// Dedicated compute queue
		if ( requestedQueueTypes & VK_QUEUE_COMPUTE_BIT ) {
			queueFamilyIndices.compute = getQueueFamilyIndex(VK_QUEUE_COMPUTE_BIT);
			if ( queueFamilyIndices.compute != queueFamilyIndices.graphics ) {
				// If compute family index differs, we need an additional queue create info for the compute queue
				VkDeviceQueueCreateInfo queueInfo{};
				queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queueInfo.queueFamilyIndex = queueFamilyIndices.compute;
				queueInfo.queueCount = 1;
				queueInfo.pQueuePriorities = &defaultQueuePriority;
				queueCreateInfos.push_back(queueInfo);
			}
		} else {
			// Else we use the same queue
			queueFamilyIndices.compute = queueFamilyIndices.graphics;
		}
		// Dedicated transfer queue
		if ( requestedQueueTypes & VK_QUEUE_TRANSFER_BIT ) {
			queueFamilyIndices.transfer = getQueueFamilyIndex(VK_QUEUE_TRANSFER_BIT);
			if ((queueFamilyIndices.transfer != queueFamilyIndices.graphics) && (queueFamilyIndices.transfer != queueFamilyIndices.compute)) {
				// If compute family index differs, we need an additional queue create info for the compute queue
				VkDeviceQueueCreateInfo queueInfo{};
				queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queueInfo.queueFamilyIndex = queueFamilyIndices.transfer;
				queueInfo.queueCount = 1;
				queueInfo.pQueuePriorities = &defaultQueuePriority;
				queueCreateInfos.emplace_back(queueInfo);
			}
		} else {
			// Else we use the same queue
			queueFamilyIndices.transfer = queueFamilyIndices.graphics;
		}

		// Create the logical device representation
		if ( useSwapChain ) {
			// If the device will be used for presenting to a display via a swapchain we need to request the swapchain extension
			deviceExtensions.emplace_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
		}

		enableRequestedDeviceFeatures( *this );

		auto cDeviceExtensions = uf::string::cStrings( deviceExtensions );

		VkDeviceCreateInfo deviceCreateInfo = {};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());;
		deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
	//	deviceCreateInfo.pEnabledFeatures = &enabledFeatures;
		deviceCreateInfo.pEnabledFeatures = nullptr;
		
		VkDeviceGroupDeviceCreateInfo groupDeviceCreateInfo = {};
		groupDeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_GROUP_DEVICE_CREATE_INFO;
		groupDeviceCreateInfo.physicalDeviceCount = physicalDevices.size();
		groupDeviceCreateInfo.pPhysicalDevices = physicalDevices.data();

		if ( deviceExtensions.size() > 0 ) {
			deviceCreateInfo.enabledExtensionCount = (uint32_t) cDeviceExtensions.size();
			deviceCreateInfo.ppEnabledExtensionNames = cDeviceExtensions.data();
		}

		struct BaseStructure {
			VkStructureType    sType;
			void*              pNext;
		};
		std::queue<void*> chain = {};

		VkPhysicalDeviceFeatures2 physicalDeviceFeatures2{};
		VkPhysicalDeviceVulkan12Features physicalDeviceVulkan12Features{};
		VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures{};
		VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures{};
		VkPhysicalDeviceShaderDrawParametersFeatures shaderDrawParametersFeatures{};
		VkPhysicalDeviceRobustness2FeaturesEXT robustnessFeatures{};
		VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeatures{};
		VkPhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures{};
		VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{};
		VkPhysicalDeviceShaderClockFeaturesKHR shaderClockFeatures{};
		VkPhysicalDeviceFragmentShaderBarycentricFeaturesKHR fragmentShaderBarycentricFeatures{};
		VkPhysicalDeviceSubgroupSizeControlFeatures subgroupSizeControlFeatures{};

		VkPhysicalDeviceVulkan12Features enabledPhysicalDeviceVulkan12Features{};
		VkPhysicalDeviceFeatures2 enabledDeviceFeatures2{}; {
			enabledPhysicalDeviceVulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;

			enabledDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
			enabledDeviceFeatures2.pNext = &enabledPhysicalDeviceVulkan12Features;

			vkGetPhysicalDeviceFeatures2(device.physicalDevice, &enabledDeviceFeatures2);
		}

		if ( ext::vulkan::settings::requested::featureChain["physicalDevice2"].as<bool>(false) ) {
			physicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
			physicalDeviceFeatures2.features = enabledFeatures;
			chain.push( &physicalDeviceFeatures2 );
			VK_VALIDATION_MESSAGE("Enabled feature chain: {}", "physicalDeviceFeatures2" );
		}
		if ( ext::vulkan::settings::requested::featureChain["physicalDeviceVulkan12"].as<bool>(false) ) {
			physicalDeviceVulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
			enableRequestedDeviceFeatures12( enabledPhysicalDeviceVulkan12Features, physicalDeviceVulkan12Features );
			chain.push( &physicalDeviceVulkan12Features );
			VK_VALIDATION_MESSAGE("Enabled feature chain: {}", "physicalDeviceVulkan12Features" );
		} else {
			if ( ext::vulkan::settings::requested::featureChain["descriptorIndexing"].as<bool>(false) ) {
				// core for Vulkan 1.3+
				descriptorIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
				descriptorIndexingFeatures.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
				descriptorIndexingFeatures.shaderStorageImageArrayNonUniformIndexing = VK_TRUE;
				descriptorIndexingFeatures.runtimeDescriptorArray = VK_TRUE;
				descriptorIndexingFeatures.descriptorBindingVariableDescriptorCount = VK_TRUE;
				chain.push( &descriptorIndexingFeatures );
				VK_VALIDATION_MESSAGE("Enabled feature chain: {}", "descriptorIndexingFeatures" );
			}
			// core for Vulkan 1.3+
			if ( ext::vulkan::settings::requested::featureChain["bufferDeviceAddress"].as<bool>(false) ) {
				bufferDeviceAddressFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
				bufferDeviceAddressFeatures.bufferDeviceAddress = VK_TRUE;
				chain.push( &bufferDeviceAddressFeatures );
				VK_VALIDATION_MESSAGE("Enabled feature chain: {}", "bufferDeviceAddressFeatures" );
			}
		}

		//
		if ( ext::vulkan::settings::requested::featureChain["shaderDrawParameters"].as<bool>(false) ) {
			shaderDrawParametersFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES;
			shaderDrawParametersFeatures.shaderDrawParameters = VK_TRUE;
			chain.push( &shaderDrawParametersFeatures );
			VK_VALIDATION_MESSAGE("Enabled feature chain: {}", "shaderDrawParametersFeatures" );
		}
		if ( ext::vulkan::settings::requested::featureChain["robustness"].as<bool>(false) ) {
			robustnessFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT;
			robustnessFeatures.nullDescriptor = VK_TRUE;
			chain.push( &robustnessFeatures );
			VK_VALIDATION_MESSAGE("Enabled feature chain: {}", "robustnessFeatures" );
		}

		if ( ext::vulkan::settings::requested::featureChain["shaderClock"].as<bool>(false) ) {
			shaderClockFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CLOCK_FEATURES_KHR;
			shaderClockFeatures.shaderSubgroupClock = VK_TRUE;
			shaderClockFeatures.shaderDeviceClock = VK_TRUE;
			chain.push( &shaderClockFeatures );
			VK_VALIDATION_MESSAGE("Enabled feature chain: {}", "shaderClockFeatures" );
		}
		if ( ext::vulkan::settings::requested::featureChain["fragmentShaderBarycentric"].as<bool>(false) ) {
			fragmentShaderBarycentricFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_FEATURES_KHR;
			fragmentShaderBarycentricFeatures.fragmentShaderBarycentric = VK_TRUE;
			chain.push( &fragmentShaderBarycentricFeatures );
			VK_VALIDATION_MESSAGE("Enabled feature chain: {}", "fragmentShaderBarycentricFeatures" );
		}
		if ( ext::vulkan::settings::requested::featureChain["rayTracingPipeline"].as<bool>(false) ) {
			rayTracingPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
			rayTracingPipelineFeatures.rayTracingPipeline = VK_TRUE;
			chain.push( &rayTracingPipelineFeatures );
			VK_VALIDATION_MESSAGE("Enabled feature chain: {}", "rayTracingPipelineFeatures" );
		}
		if ( ext::vulkan::settings::requested::featureChain["rayQuery"].as<bool>(false) ) {
			rayQueryFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
			rayQueryFeatures.rayQuery = VK_TRUE;
			chain.push( &rayQueryFeatures );
			VK_VALIDATION_MESSAGE("Enabled feature chain: {}", "rayQueryFeatures" );
		}
		if ( ext::vulkan::settings::requested::featureChain["accelerationStructure"].as<bool>(false) ) {
			accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
			accelerationStructureFeatures.accelerationStructure = VK_TRUE;
		//	accelerationStructureFeatures.accelerationStructureHostCommands = VK_TRUE;
			chain.push( &accelerationStructureFeatures );
			VK_VALIDATION_MESSAGE("Enabled feature chain: {}", "accelerationStructureFeatures" );
		}
		if ( ext::vulkan::settings::requested::featureChain["subgroupSizeControl"].as<bool>(false) ) {
			subgroupSizeControlFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_FEATURES;
			subgroupSizeControlFeatures.subgroupSizeControl = VK_TRUE;
			chain.push( &subgroupSizeControlFeatures );
			VK_VALIDATION_MESSAGE("Enabled feature chain: {}", "subgroupSizeControlFeatures" );
		}

		BaseStructure* next = (BaseStructure*) &deviceCreateInfo;
		while ( !chain.empty() ) {
			BaseStructure* front = (BaseStructure*) chain.front();
			next->pNext = front;
			next = front;
			chain.pop();
		}
	
		if ( settings::experimental::enableMultiGPU ) {
			UF_MSG_DEBUG("Multiple devices supported, using {} devices...", groupDeviceCreateInfo.physicalDeviceCount);
			accelerationStructureFeatures.pNext = &groupDeviceCreateInfo;
		}

		if ( vkCreateDevice( this->physicalDevice, &deviceCreateInfo, nullptr, &this->logicalDevice) != VK_SUCCESS ) UF_EXCEPTION("Vulkan error: failed to create logical device!"); 

		{
			ext::json::Value payload = ext::json::array();
			for ( auto& s : deviceExtensions ) {
				payload.emplace_back( s );
			}
			uf::hooks.call("vulkan:Device.ExtensionsEnabled", payload);
		}
		{
			ext::json::Value payload = retrieveDeviceFeatures( *this );
			VK_VALIDATION_MESSAGE("{}", payload.dump());
			uf::hooks.call("vulkan:Device.FeaturesEnabled", payload);
		}
	/*
		{
			ext::json::Value payload = retrieveDeviceFeatures12( *this );
			VK_VALIDATION_MESSAGE("{}", payload.dump());
			uf::hooks.call("vulkan:Device.FeaturesEnabled", payload);
		}
	*/
	}
	// Create command pool
	getCommandPool( QueueEnum::GRAPHICS );
	getCommandPool( QueueEnum::COMPUTE );
	getCommandPool( QueueEnum::TRANSFER );
	// Set queue
	{
		uint32_t graphicsQueueNodeIndex = UINT32_MAX;
		uint32_t presentQueueNodeIndex = UINT32_MAX;
		uint32_t computeQueueNodeIndex = UINT32_MAX;
		uint32_t transferQueueNodeIndex = UINT32_MAX;

		int i = 0;
		for (const auto& queueFamily : queueFamilyProperties) {
			if ( queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT )
				graphicsQueueNodeIndex = i;

			if ( queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT )
				computeQueueNodeIndex = i;

			if ( queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT )
				transferQueueNodeIndex = i;

			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR( this->physicalDevice, i, surface, &presentSupport );
			if ( queueFamily.queueCount > 0 && presentSupport )
				presentQueueNodeIndex = i;

			if ( graphicsQueueNodeIndex != UINT32_MAX && presentQueueNodeIndex != UINT32_MAX && computeQueueNodeIndex != UINT32_MAX ) break;

			i++;
		}

		VK_VALIDATION_MESSAGE("Graphics queue: {}", device.queueFamilyIndices.graphics);
		VK_VALIDATION_MESSAGE("Compute queue: {}", device.queueFamilyIndices.compute);
		VK_VALIDATION_MESSAGE("Transfer queue: {}", device.queueFamilyIndices.transfer);
		VK_VALIDATION_MESSAGE("Present queue: {}", device.queueFamilyIndices.present);

		device.queueFamilyIndices.present = presentQueueNodeIndex;
		getQueue( QueueEnum::GRAPHICS );
		getQueue( QueueEnum::PRESENT );
		getQueue( QueueEnum::COMPUTE );
		getQueue( QueueEnum::TRANSFER );
	}
	// Set formats
	{
		uf::stl::vector<VkSurfaceFormatKHR> formats;
		uint32_t formatCount; vkGetPhysicalDeviceSurfaceFormatsKHR( this->physicalDevice, device.surface, &formatCount, nullptr);
		formats.resize( formatCount );
		vkGetPhysicalDeviceSurfaceFormatsKHR( this->physicalDevice, device.surface, &formatCount, formats.data() );

		bool SRGB = true;
		auto TARGET_FORMAT = SRGB ? VK_FORMAT_B8G8R8A8_SRGB : VK_FORMAT_B8G8R8A8_UNORM;
		auto TARGET_COLORSPACE = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
		if ( ext::vulkan::settings::pipelines::hdr ) {
			TARGET_FORMAT = VK_FORMAT_R32G32B32A32_SFLOAT;
			TARGET_COLORSPACE = VK_COLOR_SPACE_HDR10_ST2084_EXT;	
		}

		// If the surface format list only includes one entry with VK_FORMAT_UNDEFINED,
		// there is no preferered format, so we assume VK_FORMAT_B8G8R8A8_SRGB
		if ( formatCount == 1 && formats[0].format == VK_FORMAT_UNDEFINED ) {
			ext::vulkan::settings::formats::color = TARGET_FORMAT;
			ext::vulkan::settings::formats::colorSpace = formats[0].colorSpace;
		} else {
			// iterate over the list of available surface format and
			// check for the presence of VK_FORMAT_B8G8R8A8_SRGB
			bool found = false;
			for ( auto&& surfaceFormat : formats ) {
				if ( surfaceFormat.format == ext::vulkan::settings::formats::color ) {
					ext::vulkan::settings::formats::color = surfaceFormat.format;
					ext::vulkan::settings::formats::colorSpace = surfaceFormat.colorSpace;
					found = true;
					break;
				}
			}
			if ( !found ) {
				for ( auto&& surfaceFormat : formats ) {
					if ( surfaceFormat.format == TARGET_FORMAT ) {
						ext::vulkan::settings::formats::color = surfaceFormat.format;
						ext::vulkan::settings::formats::colorSpace = surfaceFormat.colorSpace;
						found = true;
						break;
					}
				}
				// in case VK_FORMAT_B8G8R8A8_SRGB is not available
				// select the first available color format
				if ( !found ) {
					ext::vulkan::settings::formats::color = formats[0].format;
					ext::vulkan::settings::formats::colorSpace = formats[0].colorSpace;
				}
			}
		}
	}
	// Grab depth/stencil format
	{
		// Since all depth formats may be optional, we need to find a suitable depth format to use
		// Start with the highest precision packed format
		uf::stl::vector<VkFormat> depthFormats = {
			VK_FORMAT_D32_SFLOAT_S8_UINT,
			VK_FORMAT_D32_SFLOAT,
			VK_FORMAT_D24_UNORM_S8_UINT,
			VK_FORMAT_D16_UNORM_S8_UINT,
			VK_FORMAT_D16_UNORM,
			ext::vulkan::settings::formats::depth
		};

		for ( auto& format : depthFormats ) {
			VkFormatProperties formatProps;
			vkGetPhysicalDeviceFormatProperties( this->physicalDevice, format, &formatProps );
			// Format must support depth stencil attachment for optimal tiling
			if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
				ext::vulkan::settings::formats::depth = format;
			}
		}
	}
	// create pipeline cache
	{
		VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
		pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
		
		uf::stl::vector<uint8_t> buffer;
		// read from cache on disk
		if ( uf::io::exists( uf::io::root + "/cache/vulkan/cache.bin" ) ) {
			buffer = uf::io::readAsBuffer( uf::io::root + "/cache/vulkan/cache.bin" );
			pipelineCacheCreateInfo.initialDataSize = buffer.size();
			pipelineCacheCreateInfo.pInitialData    = buffer.data();
		}

		VK_CHECK_RESULT(vkCreatePipelineCache( device, &pipelineCacheCreateInfo, nullptr, &this->pipelineCache));
	}
	// setup allocator
	{
	/*
		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;
		allocatorInfo.physicalDevice = physicalDevice;
		allocatorInfo.instance = instance;
		allocatorInfo.device = logicalDevice;

		allocatorInfo.frameInUseCount = 1;
		vmaCreateAllocator(&allocatorInfo, &allocator);
	*/
		VmaVulkanFunctions vulkanFunctions = {};
		vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
		vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;
		 
		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_2;
		allocatorInfo.physicalDevice = physicalDevice;
		allocatorInfo.instance = instance;
		allocatorInfo.device = logicalDevice;
		allocatorInfo.pVulkanFunctions = &vulkanFunctions;

		if ( uf::renderer::settings::version <= 1.0 ) allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_0;
		else if ( uf::renderer::settings::version <= 1.1 ) allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_1;
		else if ( uf::renderer::settings::version <= 1.2 ) allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_2;
	//	else if ( uf::renderer::settings::version <= 1.3 ) allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;

		if ( uf::renderer::settings::invariant::deviceAddressing ) {
			allocatorInfo.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
		} else {
			allocatorInfo.flags = 0;
		}
		if ( uf::renderer::settings::experimental::memoryBudgetBit ) {
			UF_MSG_DEBUG("MEMORY BUDGET BIT SET");
			allocatorInfo.flags |= VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
		}

		UF_MSG_DEBUG("Allocator flags: {}", allocatorInfo.flags);
		 
		vmaCreateAllocator(&allocatorInfo, &allocator);
	}

	{
		vkGetBufferDeviceAddressKHR = reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(vkGetDeviceProcAddr(device, "vkGetBufferDeviceAddressKHR"));
		vkCmdBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(device, "vkCmdBuildAccelerationStructuresKHR"));
		vkBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(device, "vkBuildAccelerationStructuresKHR"));
		vkCreateAccelerationStructureKHR = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(device, "vkCreateAccelerationStructureKHR"));
		vkDestroyAccelerationStructureKHR = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(vkGetDeviceProcAddr(device, "vkDestroyAccelerationStructureKHR"));
		vkGetAccelerationStructureBuildSizesKHR = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(vkGetDeviceProcAddr(device, "vkGetAccelerationStructureBuildSizesKHR"));
		vkGetAccelerationStructureDeviceAddressKHR = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(vkGetDeviceProcAddr(device, "vkGetAccelerationStructureDeviceAddressKHR"));
		vkCmdTraceRaysKHR = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(device, "vkCmdTraceRaysKHR"));
		vkGetRayTracingShaderGroupHandlesKHR = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(vkGetDeviceProcAddr(device, "vkGetRayTracingShaderGroupHandlesKHR"));
		vkCreateRayTracingPipelinesKHR = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(vkGetDeviceProcAddr(device, "vkCreateRayTracingPipelinesKHR"));
		vkCmdWriteAccelerationStructuresPropertiesKHR = reinterpret_cast<PFN_vkCmdWriteAccelerationStructuresPropertiesKHR>(vkGetDeviceProcAddr(device, "vkCmdWriteAccelerationStructuresPropertiesKHR"));
		vkCmdCopyAccelerationStructureKHR = reinterpret_cast<PFN_vkCmdCopyAccelerationStructureKHR>(vkGetDeviceProcAddr(device, "vkCmdCopyAccelerationStructureKHR"));
		
		if ( extensions.enabled.device["VK_NV_device_diagnostic_checkpoints"] ) {
			vkCmdSetCheckpointNV = reinterpret_cast<PFN_vkCmdSetCheckpointNV>(vkGetDeviceProcAddr(device, "vkCmdSetCheckpointNV"));
			vkGetQueueCheckpointDataNV = reinterpret_cast<PFN_vkGetQueueCheckpointDataNV>(vkGetDeviceProcAddr(device, "vkGetQueueCheckpointDataNV"));
		}
	}
}

void ext::vulkan::Device::destroy() {
	while ( !this->reusable.fences.empty() ) {
		VkFence fence = this->reusable.fences.top();
		this->reusable.fences.pop();

		vkDestroyFence( this->logicalDevice, fence, nullptr );
	}
	if ( this->pipelineCache ) {
		// write cache on disk
		{
			size_t size{};
			VK_CHECK_RESULT(vkGetPipelineCacheData(this->logicalDevice, this->pipelineCache, &size, NULL));

			uf::stl::vector<uint8_t> data(size);
			VK_CHECK_RESULT(vkGetPipelineCacheData(this->logicalDevice, this->pipelineCache, &size, data.data()));

			uf::io::write( uf::io::root + "/cache/vulkan/cache.bin", data );
		}

		vkDestroyPipelineCache( this->logicalDevice, this->pipelineCache, nullptr );
		this->pipelineCache = nullptr;
	}
	for ( auto& pair : this->commandPool.graphics.container() ) {
		vkDestroyCommandPool( this->logicalDevice, pair.second, nullptr );
		pair.second = VK_NULL_HANDLE;
	}
	for ( auto& pair : this->commandPool.compute.container() ) {
		vkDestroyCommandPool( this->logicalDevice, pair.second, nullptr );
		pair.second = VK_NULL_HANDLE;
	}
	for ( auto& pair : this->commandPool.transfer.container() ) {
		vkDestroyCommandPool( this->logicalDevice, pair.second, nullptr );
		pair.second = VK_NULL_HANDLE;
	}
	if ( this->logicalDevice ) {
		vkDestroyDevice( this->logicalDevice, nullptr );
		this->logicalDevice = nullptr;
	}
	if ( this->surface ) {
		vkDestroySurfaceKHR( this->instance, this->surface, nullptr );
		this->surface = nullptr;
	}
	if ( this->debugMessenger ) {
		DestroyDebugUtilsMessengerEXT( this->instance, this->debugMessenger, nullptr );
		this->debugMessenger = nullptr;
	}
	if ( this->instance ) {
		vkDestroyInstance( this->instance, nullptr );
		this->instance = nullptr;
	}
/*
	vmaDestroyAllocator( allocator );
*/
}

#endif