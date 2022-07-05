#if UF_USE_VULKAN

#include <uf/ext/vulkan/vulkan.h>
#include <uf/ext/vulkan/swapchain.h>
#include <uf/ext/vulkan/initializers.h>
#include <uf/utils/window/window.h>

VkResult ext::vulkan::Swapchain::acquireNextImage( uint32_t* imageIndex, VkSemaphore presentCompleteSemaphore ) {
	// By setting timeout to UINT64_MAX we will always wait until the next image has been acquired or an actual error is thrown
	// With that we don't have to handle VK_NOT_READY
	return vkAcquireNextImageKHR( *device, swapChain, UINT64_MAX, presentCompleteSemaphore, (VkFence) nullptr, imageIndex );
}

VkResult ext::vulkan::Swapchain::queuePresent( VkQueue queue, uint32_t imageIndex, VkSemaphore waitSemaphore ) {
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = NULL;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &swapChain;
	presentInfo.pImageIndices = &imageIndex;
	// Check if a wait semaphore has been specified to wait for before presenting the image
	if ( waitSemaphore != VK_NULL_HANDLE ) {
		presentInfo.pWaitSemaphores = &waitSemaphore;
		presentInfo.waitSemaphoreCount = 1;
	}
	return vkQueuePresentKHR(queue, &presentInfo);
}

void ext::vulkan::Swapchain::initialize( Device& device ) {
	// Bind
	{
		this->device = &device;
//		if ( width == 0 ) width = ext::vulkan::settings::width;
//		if ( height == 0 ) height = ext::vulkan::settings::height;
	}
	// Set present
	VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
	{
		// Get available present modes
		uint32_t presentModeCount;
		VK_CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(device.physicalDevice, device.surface, &presentModeCount, NULL));
		assert(presentModeCount > 0);

		uf::stl::vector<VkPresentModeKHR> presentModes(presentModeCount);
		VK_CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(device.physicalDevice, device.surface, &presentModeCount, presentModes.data()));
	#if 1
		if ( settings::pipelines::vsync ) {
			swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
			for ( size_t i = 0; i < presentModeCount; i++ ) {
				if ( presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR ) {
					swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
					break;
				}
			}
		} else {
			swapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
			for ( size_t i = 0; i < presentModeCount; i++ ) {
				if ( presentModes[i] == VK_PRESENT_MODE_FIFO_RELAXED_KHR ) {
					swapchainPresentMode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
					break;
				}
			}
		}
	#else
		if ( !settings::pipelines::vsync ) {
			for ( size_t i = 0; i < presentModeCount; i++ ) {
				if ( presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR ) {
					swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
					break;
				}
				if ( (swapchainPresentMode != VK_PRESENT_MODE_MAILBOX_KHR) && (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR) )
					swapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
			}
		}
	#endif
	
		switch ( swapchainPresentMode ) {
			case VK_PRESENT_MODE_IMMEDIATE_KHR: VK_VALIDATION_MESSAGE("Swapchain present mode: VK_PRESENT_MODE_IMMEDIATE_KHR"); break;
			case VK_PRESENT_MODE_MAILBOX_KHR: VK_VALIDATION_MESSAGE("Swapchain present mode: VK_PRESENT_MODE_MAILBOX_KHR"); break;
			case VK_PRESENT_MODE_FIFO_KHR: VK_VALIDATION_MESSAGE("Swapchain present mode: VK_PRESENT_MODE_FIFO_KHR"); break;
			case VK_PRESENT_MODE_FIFO_RELAXED_KHR: VK_VALIDATION_MESSAGE("Swapchain present mode: VK_PRESENT_MODE_FIFO_RELAXED_KHR"); break;
			default: VK_VALIDATION_MESSAGE("Swapchain present mode: {}", swapchainPresentMode);
		}
	}
	// Set extent
	VkExtent2D swapchainExtent = {};
	{
		VkSurfaceCapabilitiesKHR capabilities;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR( device.physicalDevice, device.surface, &capabilities );
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
			swapchainExtent = capabilities.currentExtent;
		} else {
			auto size = device.window->getSize();
			VkExtent2D swapchainExtent = {
				static_cast<uint32_t>(size[0]),
				static_cast<uint32_t>(size[1])
			};

			swapchainExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, swapchainExtent.width));
			swapchainExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, swapchainExtent.height));
		}
	}
	// Create (re)swap chain
	{
		VkSwapchainKHR oldSwapchain = swapChain;

		// Get physical device surface properties and formats
		VkSurfaceCapabilitiesKHR surfCaps;
		VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.physicalDevice, device.surface, &surfCaps));

		// Determine the number of images
		uint32_t desiredNumberOfSwapchainImages = surfCaps.minImageCount + 1;
		if ( (surfCaps.maxImageCount > 0) && (desiredNumberOfSwapchainImages > surfCaps.maxImageCount) ) {
			desiredNumberOfSwapchainImages = surfCaps.maxImageCount;
		}

		// Find the transformation of the surface
		VkSurfaceTransformFlagsKHR preTransform;
		if ( surfCaps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR ) {
			// We prefer a non-rotated transform
			preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		} else
			preTransform = surfCaps.currentTransform;

		// Find a supported composite alpha format (not all devices support alpha opaque)
		VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		// Simply select the first composite alpha format available
		uf::stl::vector<VkCompositeAlphaFlagBitsKHR> compositeAlphaFlags = {
			VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
			VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
			VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
		};
		for ( auto& compositeAlphaFlag : compositeAlphaFlags ) {
			if ( surfCaps.supportedCompositeAlpha & compositeAlphaFlag ) {
				compositeAlpha = compositeAlphaFlag;
				break;
			};
		}

		VkSwapchainCreateInfoKHR swapchainCI = {};
		swapchainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapchainCI.pNext = NULL;
		swapchainCI.surface = device.surface;
		swapchainCI.minImageCount = desiredNumberOfSwapchainImages;
		swapchainCI.imageFormat = ext::vulkan::settings::formats::color; // device.formats.color;
		swapchainCI.imageColorSpace = ext::vulkan::settings::formats::colorSpace; // device.formats.space;
		swapchainCI.imageExtent = { swapchainExtent.width, swapchainExtent.height };
		swapchainCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		swapchainCI.preTransform = (VkSurfaceTransformFlagBitsKHR)preTransform;
		swapchainCI.imageArrayLayers = 1;
		swapchainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchainCI.queueFamilyIndexCount = 0;
		swapchainCI.pQueueFamilyIndices = NULL;
		swapchainCI.presentMode = swapchainPresentMode;
		swapchainCI.oldSwapchain = oldSwapchain;
		// Setting clipped to VK_TRUE allows the implementation to discard rendering outside of the surface area
		swapchainCI.clipped = VK_TRUE;
		swapchainCI.compositeAlpha = compositeAlpha;

		// Enable transfer source on swap chain images if supported
		if (surfCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
			swapchainCI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		}

		// Enable transfer destination on swap chain images if supported
		if (surfCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
			swapchainCI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		}

		VK_CHECK_RESULT(vkCreateSwapchainKHR( device.logicalDevice, &swapchainCI, nullptr, &swapChain));

		// If an existing swap chain is re-created, destroy the old swap chain
		// This also cleans up all the presentable images
		if (oldSwapchain != VK_NULL_HANDLE) vkDestroySwapchainKHR( device.logicalDevice, oldSwapchain, nullptr);
		VK_CHECK_RESULT(vkGetSwapchainImagesKHR( device.logicalDevice, swapChain, &buffers, NULL));
	}
}

void ext::vulkan::Swapchain::destroy() {
	if ( !device ) return;

	if ( swapChain != VK_NULL_HANDLE ) {
		vkDestroySwapchainKHR( *device, swapChain, nullptr);
	}

	swapChain = VK_NULL_HANDLE;
	device = VK_NULL_HANDLE;
}

#endif