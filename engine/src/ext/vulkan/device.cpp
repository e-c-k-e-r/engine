#include <uf/ext/vulkan/vulkan.h>
#include <uf/ext/vulkan/device.h>
#include <uf/ext/vulkan/initializers.h>
#include <uf/utils/window/window.h>
#include <uf/utils/string/ext.h>
#include <uf/ext/openvr/openvr.h>

#include <set>
#include <map>

#include <uf/utils/serialize/serializer.h>

namespace {
	void VRExtensions( std::vector<std::string>& requested ) {
		if ( !vr::VRCompositor() ) return;
		uint32_t nBufferSize = vr::VRCompositor()->GetVulkanInstanceExtensionsRequired( nullptr, 0 );
		if ( nBufferSize < 0 ) return;
		char pExtensionStr[nBufferSize];
		pExtensionStr[0] = 0;
		vr::VRCompositor()->GetVulkanInstanceExtensionsRequired( pExtensionStr, nBufferSize );
		std::vector<std::string> extensions = uf::string::split( pExtensionStr, " " );
		requested.insert( requested.end(), extensions.begin(), extensions.end() );
	}
	void enableRequestedDeviceFeatures( ext::vulkan::Device& device ) {
		uf::Serializer json;

	#define CHECK_FEATURE( NAME )\
		if ( feature == #NAME ) {\
			if ( device.features.NAME == VK_TRUE ) {\
				device.enabledFeatures.NAME = true;\
				if ( ext::vulkan::validation ) std::cout << "Enabled feature: " << feature << std::endl;\
			} else if ( ext::vulkan::validation ) std::cout << "Failed to enable feature: " << feature << std::endl;\
		}

		for ( auto& feature : ext::vulkan::requestedDeviceFeatures ) {
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
	}
	uf::Serializer retrieveDeviceFeatures( ext::vulkan::Device& device ) {
		uf::Serializer json;

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
	throw std::runtime_error("Could not find a matching queue family index");
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
		throw std::runtime_error("Could not find a matching memory type");
}

int ext::vulkan::Device::rate( VkPhysicalDevice device ) {
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);

	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
	int score = 0;
	// Discrete GPUs have a significant performance advantage
	if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		score += 1000;
	// Maximum possible size of textures affects graphics quality
	score += deviceProperties.limits.maxImageDimension2D;
	// Application can't function without geometry shaders
	if ( !deviceFeatures.geometryShader ) return 0;
	//
	{
		const std::vector<const char*> deviceExtensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties( device, nullptr, &extensionCount, nullptr );
		std::vector<VkExtensionProperties> availableExtensions( extensionCount );
		vkEnumerateDeviceExtensionProperties( device, nullptr, &extensionCount, availableExtensions.data() );
		std::set<std::string> requiredExtensions( deviceExtensions.begin(), deviceExtensions.end() );

		for ( const auto& extension : availableExtensions )
			requiredExtensions.erase( extension.extensionName );

		if ( !requiredExtensions.empty() ) return 0;
	}
	//
	{
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR( device, this->surface, &capabilities );

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, this->surface, &formatCount, nullptr);
		if ( formatCount != 0 ) {
			formats.resize( formatCount );
			vkGetPhysicalDeviceSurfaceFormatsKHR( device, this->surface, &formatCount, formats.data() );
		}
		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR( device, this->surface, &presentModeCount, nullptr );
		if ( presentModeCount != 0 ) {
			presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR( device, this->surface, &presentModeCount, presentModes.data() );
		}
		if ( formats.empty() || presentModes.empty() ) return 0;
	}
	return score;
}

VkCommandBuffer ext::vulkan::Device::createCommandBuffer( VkCommandBufferLevel level, bool begin ){
	VkCommandBufferAllocateInfo cmdBufAllocateInfo = ext::vulkan::initializers::commandBufferAllocateInfo( commandPool.transfer, level, 1 );

	VkCommandBuffer commandBuffer;
	VK_CHECK_RESULT( vkAllocateCommandBuffers( logicalDevice, &cmdBufAllocateInfo, &commandBuffer ) );
	// If requested, also start recording for the new command buffer
	if ( begin ) {
		VkCommandBufferBeginInfo cmdBufInfo = ext::vulkan::initializers::commandBufferBeginInfo();
		VK_CHECK_RESULT( vkBeginCommandBuffer( commandBuffer, &cmdBufInfo ) );
	}
	return commandBuffer;
}

void ext::vulkan::Device::flushCommandBuffer( VkCommandBuffer commandBuffer, bool free ) {
	if ( commandBuffer == VK_NULL_HANDLE ) return;

	VK_CHECK_RESULT( vkEndCommandBuffer( commandBuffer ) );

	VkSubmitInfo submitInfo = ext::vulkan::initializers::submitInfo();
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	// Create fence to ensure that the command buffer has finished executing
	VkFenceCreateInfo fenceInfo = ext::vulkan::initializers::fenceCreateInfo(VK_FLAGS_NONE);
	VkFence fence;
	VK_CHECK_RESULT(vkCreateFence(logicalDevice, &fenceInfo, nullptr, &fence));
	
	// Submit to the queue
	VK_CHECK_RESULT(vkQueueSubmit(device.queues.transfer, 1, &submitInfo, fence));
	// Wait for the fence to signal that command buffer has finished executing
	VK_CHECK_RESULT(vkWaitForFences(logicalDevice, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));

	vkDestroyFence(logicalDevice, fence, nullptr);

	if ( free ) vkFreeCommandBuffers(logicalDevice, commandPool.transfer, 1, &commandBuffer);
}

VkResult ext::vulkan::Device::createBuffer( VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkDeviceSize size, VkBuffer* buffer, VkDeviceMemory* memory, void *data ) {
	// Create the buffer handle
	VkBufferCreateInfo bufferCreateInfo = ext::vulkan::initializers::bufferCreateInfo(usageFlags, size);
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	VK_CHECK_RESULT( vkCreateBuffer(logicalDevice, &bufferCreateInfo, nullptr, buffer));

	// Create the memory backing up the buffer handle
	VkMemoryRequirements memReqs;
	VkMemoryAllocateInfo memAlloc = ext::vulkan::initializers::memoryAllocateInfo();
	vkGetBufferMemoryRequirements(logicalDevice, *buffer, &memReqs);
	memAlloc.allocationSize = memReqs.size;
	// Find a memory type index that fits the properties of the buffer
	memAlloc.memoryTypeIndex = getMemoryType(memReqs.memoryTypeBits, memoryPropertyFlags);

	VK_CHECK_RESULT( vkAllocateMemory(logicalDevice, &memAlloc, nullptr, memory));
	
	// If a pointer to the buffer data has been passed, map the buffer and copy over the data
	if ( data != nullptr ) {
		void *mapped;
		VK_CHECK_RESULT(vkMapMemory(logicalDevice, *memory, 0, size, 0, &mapped));
			
		memcpy(mapped, data, size);
		// If host coherency hasn't been requested, do a manual flush to make writes visible
		if ( (memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0 ) {
			VkMappedMemoryRange mappedRange = ext::vulkan::initializers::mappedMemoryRange();
			mappedRange.memory = *memory;
			mappedRange.offset = 0;
			mappedRange.size = size;
			vkFlushMappedMemoryRanges(logicalDevice, 1, &mappedRange);
		}
		vkUnmapMemory(logicalDevice, *memory);
	}

	// Attach the memory to the buffer object
	VK_CHECK_RESULT(vkBindBufferMemory(logicalDevice, *buffer, *memory, 0));

	return VK_SUCCESS;
}

VkResult ext::vulkan::Device::createBuffer(
	VkBufferUsageFlags usageFlags,
	VkMemoryPropertyFlags memoryPropertyFlags,
	ext::vulkan::Buffer& buffer,
	VkDeviceSize size,
	void *data
) {
	buffer.device = logicalDevice;
	buffer.usageFlags = usageFlags;
	buffer.memoryPropertyFlags = memoryPropertyFlags;

	// Create the buffer handle
	VkBufferCreateInfo bufferCreateInfo = ext::vulkan::initializers::bufferCreateInfo(usageFlags, size);
	buffer.allocate( bufferCreateInfo );

/*
//	VK_CHECK_RESULT(vkCreateBuffer(logicalDevice, &bufferCreateInfo, nullptr, &buffer.buffer));
	// Create the memory backing up the buffer handle
	VkMemoryRequirements memReqs;
	VkMemoryAllocateInfo memAlloc = ext::vulkan::initializers::memoryAllocateInfo();
	vkGetBufferMemoryRequirements(logicalDevice, buffer.buffer, &memReqs);
	memAlloc.allocationSize = memReqs.size;
	// Find a memory type index that fits the properties of the buffer
	memAlloc.memoryTypeIndex = getMemoryType(memReqs.memoryTypeBits, memoryPropertyFlags);
	VK_CHECK_RESULT(vkAllocateMemory(logicalDevice, &memAlloc, nullptr, &buffer.memory));

	buffer.alignment = memReqs.alignment;
	buffer.size = memAlloc.allocationSize;
	buffer.usageFlags = usageFlags;
	buffer.memoryPropertyFlags = memoryPropertyFlags;
	buffer.memAlloc = memAlloc;
	buffer.memReqs = memReqs;
*/

	// If a pointer to the buffer data has been passed, map the buffer and copy over the data

	if (data != nullptr) {
		VK_CHECK_RESULT(buffer.map());
		memcpy(buffer.mapped, data, size);
		if ((memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0) buffer.flush();
		buffer.unmap();
	}


	// Initialize a default descriptor that covers the whole buffer size
	buffer.setupDescriptor();

	// Attach the memory to the buffer object
	return buffer.bind();
}

void ext::vulkan::Device::initialize() {	
	const std::vector<const char*> validationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};
	// Assert validation layers
	if ( ext::vulkan::validation ) {
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
		for ( const char* layerName : validationLayers ) {
			bool layerFound = false;
			for ( const auto& layerProperties : availableLayers ) {
				if ( strcmp(layerName, layerProperties.layerName) == 0 ) {
					layerFound = true; break;
				}
			}
			if ( !layerFound ) throw std::runtime_error("validation layers requested, but not available!");
		}
	}
	// 
	// Get extensions
	std::vector<std::string> requestedExtensions = window->getExtensions( ext::vulkan::validation );
	// Load any requested extensions
	requestedExtensions.insert( requestedExtensions.end(), ext::vulkan::requestedInstanceExtensions.begin(), ext::vulkan::requestedInstanceExtensions.end() );
	// OpenVR Support
	if ( ext::openvr::enabled ) VRExtensions(requestedExtensions);

	{
		if ( ext::vulkan::validation )
			for ( auto ext : requestedExtensions ) std::cout << "Requested extension: " << ext << std::endl;
		uint32_t extensionsCount = 0;
		uint32_t enabledExtensionsCount = 0;
		
		VK_CHECK_RESULT(vkEnumerateInstanceExtensionProperties( NULL, &extensionsCount, NULL ));

		std::vector<VkExtensionProperties> extensionProperties(extensionsCount);
		VK_CHECK_RESULT( vkEnumerateInstanceExtensionProperties( NULL, &extensionsCount, &extensionProperties[0] ) );

		for ( size_t i = 0; i < requestedExtensions.size(); ++i ) {
			bool found = false; uint32_t index = 0;
			for ( index = 0; index < extensionsCount; index++ ) {
				if ( strcmp( requestedExtensions[i].c_str(), extensionProperties[index].extensionName ) == 0 ) {
					for ( auto& alreadyAdded : supportedExtensions ) {
						if ( requestedExtensions[i] == alreadyAdded ) {
							found = true;
							break;
						}
					}
					if ( found ) break;
					found = true;
					supportedExtensions.push_back(extensionProperties[index].extensionName);
					break;
				}
			}
			if ( !found ) std::cout << "Vulkan missing requested extension " << requestedExtensions[index] << std::endl;
		}
	}
	// Create instance
	{
		std::vector<const char*> extensions;
		for ( auto& s : supportedExtensions ) {
			if ( ext::vulkan::validation ) std::cout << "Enabled extension: " << s << std::endl;
			extensions.push_back( s.c_str() );
		}

		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Program";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		if ( ext::vulkan::validation ) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		} else {
			createInfo.enabledLayerCount = 0;
		}

		VK_CHECK_RESULT( vkCreateInstance( &createInfo, nullptr, &this->instance ));
	}
	// Setup debug
	if ( ext::vulkan::validation ) {
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
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices( this->instance, &deviceCount, nullptr );
		if ( deviceCount == 0 ) throw std::runtime_error("failed to find GPUs with Vulkan support!");

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices( this->instance, &deviceCount, devices.data() );
		// Use an ordered map to automatically sort candidates by increasing score
		std::multimap<int, VkPhysicalDevice> candidates;
		for ( const VkPhysicalDevice& device : devices ) {
			int score = rate( device );
			candidates.insert( std::make_pair(score, device) );
		}
		// Check if the best candidate is suitable at all
		if ( candidates.rbegin()->first <= 0 )throw std::runtime_error("failed to find a suitable GPU!");
		this->physicalDevice = candidates.rbegin()->second;
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
	}
	// Create logical device
	{
		bool useSwapChain = true;
		VkQueueFlags requestedQueueTypes = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT;
		std::vector<std::string> requestedExtensions;
		std::vector<const char*> deviceExtensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};
		requestedExtensions.insert( requestedExtensions.end(), ext::vulkan::requestedInstanceExtensions.begin(), ext::vulkan::requestedInstanceExtensions.end() );
		/* OpenVR support */ if ( ext::openvr::enabled && vr::VRCompositor() ) {
			uint32_t nBufferSize = vr::VRCompositor()->GetVulkanDeviceExtensionsRequired( ( VkPhysicalDevice_T * ) this->physicalDevice, nullptr, 0 );
			if ( nBufferSize > 0 ) {
				char pExtensionStr[nBufferSize];
				pExtensionStr[0] = 0;
				vr::VRCompositor()->GetVulkanDeviceExtensionsRequired( ( VkPhysicalDevice_T * ) this->physicalDevice, pExtensionStr, nBufferSize );
				std::vector<std::string> vrExtensions = uf::string::split( pExtensionStr, " " );
				requestedExtensions.insert( requestedExtensions.end(), vrExtensions.begin(), vrExtensions.end() );
			}
			// Allocate enough ExtensionProperties to support all extensions being enabled
			uint32_t extensionsCount = 0;
			uint32_t enabledExtensionsCount = 0;
			
			VK_CHECK_RESULT(vkEnumerateDeviceExtensionProperties( this->physicalDevice, NULL, &extensionsCount, NULL ));
			std::vector<VkExtensionProperties> extensionProperties(extensionsCount);
			VK_CHECK_RESULT( vkEnumerateDeviceExtensionProperties( this->physicalDevice, NULL, &extensionsCount, &extensionProperties[0] ) );
			
			for ( size_t i = 0; i < requestedExtensions.size(); ++i ) {
				bool found = false;
				uint32_t index = 0;
				for ( index = 0; index < extensionsCount; index++ ) {
					if ( strcmp( requestedExtensions[i].c_str(), extensionProperties[index].extensionName ) == 0 ) {
						for ( auto alreadyAdded : deviceExtensions ) {
							if ( strcmp( requestedExtensions[i].c_str(), alreadyAdded ) == 0 ) {
								found = true;
								break;
							}
						}
						if ( found ) break;
						found = true;
						deviceExtensions.push_back(extensionProperties[index].extensionName);
					}
				}
				if ( !found ) std::cout << "Vulkan missing requested extension " << requestedExtensions[index] << std::endl;
			}
		}

		// Desired queues need to be requested upon logical device creation
		// Due to differing queue family configurations of Vulkan implementations this can be a bit tricky, especially if the application
		// requests different queue types
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};
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
			queueFamilyIndices.graphics = VK_NULL_HANDLE;
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
				queueCreateInfos.push_back(queueInfo);
			}
		} else {
			// Else we use the same queue
			queueFamilyIndices.transfer = queueFamilyIndices.graphics;
		}

		// Create the logical device representation
		if ( useSwapChain ) {
			// If the device will be used for presenting to a display via a swapchain we need to request the swapchain extension
			deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
		}

		enableRequestedDeviceFeatures( *this );

		VkDeviceCreateInfo deviceCreateInfo = {};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());;
		deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
		deviceCreateInfo.pEnabledFeatures = &enabledFeatures;

		if ( deviceExtensions.size() > 0 ) {
			deviceCreateInfo.enabledExtensionCount = (uint32_t) deviceExtensions.size();
			deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
		}

		if ( vkCreateDevice( this->physicalDevice, &deviceCreateInfo, nullptr, &this->logicalDevice) != VK_SUCCESS )
			throw std::runtime_error("failed to create logical device!"); 

		if ( ext::vulkan::validation )
			std::cout << retrieveDeviceFeatures( *this ) << std::endl;
	}
	// Create command pool
	{
		VkCommandPoolCreateFlags createFlags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		VkCommandPoolCreateInfo cmdPoolInfo = {};
		cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPoolInfo.queueFamilyIndex = queueFamilyIndices.graphics;
		cmdPoolInfo.flags = createFlags;
		if ( vkCreateCommandPool( this->logicalDevice, &cmdPoolInfo, nullptr, &commandPool.graphics ) != VK_SUCCESS )
			throw std::runtime_error("failed to create command pool for graphics!");
	}
	{
		VkCommandPoolCreateFlags createFlags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		VkCommandPoolCreateInfo cmdPoolInfo = {};
		cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPoolInfo.queueFamilyIndex = queueFamilyIndices.compute;
		cmdPoolInfo.flags = createFlags;
		if ( vkCreateCommandPool( this->logicalDevice, &cmdPoolInfo, nullptr, &commandPool.compute ) != VK_SUCCESS )
			throw std::runtime_error("failed to create command pool for compute!");
	}
	{
		VkCommandPoolCreateFlags createFlags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		VkCommandPoolCreateInfo cmdPoolInfo = {};
		cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPoolInfo.queueFamilyIndex = queueFamilyIndices.transfer;
		cmdPoolInfo.flags = createFlags;
		if ( vkCreateCommandPool( this->logicalDevice, &cmdPoolInfo, nullptr, &commandPool.transfer ) != VK_SUCCESS )
			throw std::runtime_error("failed to create command pool for transfer!");
	}
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

		device.queueFamilyIndices.present = presentQueueNodeIndex;

		vkGetDeviceQueue( device, device.queueFamilyIndices.graphics, 0, &queues.graphics );
		vkGetDeviceQueue( device, device.queueFamilyIndices.present, 0, &queues.present );
		vkGetDeviceQueue( device, device.queueFamilyIndices.compute, 0, &queues.compute );
		vkGetDeviceQueue( device, device.queueFamilyIndices.transfer, 0, &queues.transfer );
	}
	// Set formats
	{
		std::vector<VkSurfaceFormatKHR> formats;
		uint32_t formatCount; vkGetPhysicalDeviceSurfaceFormatsKHR( this->physicalDevice, device.surface, &formatCount, nullptr);
		formats.resize( formatCount );
		vkGetPhysicalDeviceSurfaceFormatsKHR( this->physicalDevice, device.surface, &formatCount, formats.data() );
		// If the surface format list only includes one entry with VK_FORMAT_UNDEFINED,
		// there is no preferered format, so we assume VK_FORMAT_B8G8R8A8_UNORM
		if ( (formatCount == 1) && (formats[0].format == VK_FORMAT_UNDEFINED) ) {
			this->formats.color = VK_FORMAT_B8G8R8A8_UNORM;
			this->formats.space = formats[0].colorSpace;
		} else {
			// iterate over the list of available surface format and
			// check for the presence of VK_FORMAT_B8G8R8A8_UNORM
			bool found_B8G8R8A8_UNORM = false;
			for ( auto&& surfaceFormat : formats ) {
				if ( surfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM ) {
					this->formats.color = surfaceFormat.format;
					this->formats.space = surfaceFormat.colorSpace;
					found_B8G8R8A8_UNORM = true;
					break;
				}
			}
			// in case VK_FORMAT_B8G8R8A8_UNORM is not available
			// select the first available color format
			if ( !found_B8G8R8A8_UNORM ) {
				this->formats.color = formats[0].format;
				this->formats.space = formats[0].colorSpace;
			}
		}
	}
	// Grab depth/stencil format
	{
		// Since all depth formats may be optional, we need to find a suitable depth format to use
		// Start with the highest precision packed format
		std::vector<VkFormat> depthFormats = {
			VK_FORMAT_D32_SFLOAT_S8_UINT,
			VK_FORMAT_D32_SFLOAT,
			VK_FORMAT_D24_UNORM_S8_UINT,
			VK_FORMAT_D16_UNORM_S8_UINT,
			VK_FORMAT_D16_UNORM
		};

		for ( auto& format : depthFormats ) {
			VkFormatProperties formatProps;
			vkGetPhysicalDeviceFormatProperties( this->physicalDevice, format, &formatProps );
			// Format must support depth stencil attachment for optimal tiling
			if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
				this->formats.depth = format;
			}
		}
	}
	// create pipeline cache
	{
		VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
		pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
		VK_CHECK_RESULT(vkCreatePipelineCache( device, &pipelineCacheCreateInfo, nullptr, &this->pipelineCache));
	}
}

void ext::vulkan::Device::destroy() {
	if ( this->pipelineCache ) {
		vkDestroyPipelineCache( this->logicalDevice, this->pipelineCache, nullptr );
		this->pipelineCache = nullptr;
	}
	if ( this->commandPool.graphics ) {
		vkDestroyCommandPool( this->logicalDevice, this->commandPool.graphics, nullptr );
		this->commandPool.graphics = nullptr;
	}
	if ( this->commandPool.compute ) {
		vkDestroyCommandPool( this->logicalDevice, this->commandPool.compute, nullptr );
		this->commandPool.compute = nullptr;
	}
	if ( this->commandPool.transfer ) {
		vkDestroyCommandPool( this->logicalDevice, this->commandPool.transfer, nullptr );
		this->commandPool.transfer = nullptr;
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
}