#pragma once

#include <uf/ext/vulkan.h>
#include <uf/ext/vulkan/buffer.h>
#include <uf/utils/window/window.h>

namespace ext {
	namespace vulkan {
		struct UF_API Device {
			VkInstance instance;
			VkDebugUtilsMessengerEXT debugMessenger;
			VkSurfaceKHR surface;
			VkPhysicalDevice physicalDevice;
			VkDevice logicalDevice;
			VkCommandPool commandPool = VK_NULL_HANDLE;

			VkPhysicalDeviceProperties properties;
			VkPhysicalDeviceFeatures features;
			VkPhysicalDeviceFeatures enabledFeatures;
			VkPhysicalDeviceMemoryProperties memoryProperties;
			
			VkPipelineCache pipelineCache;

			std::vector<VkQueueFamilyProperties> queueFamilyProperties;
			std::vector<std::string> supportedExtensions;
			
			VkQueue graphicsQueue;
			VkQueue presentQueue;
			VkQueue computeQueue;

			uf::Window* window;

			struct {
				VkFormat depth;
				VkFormat color;
				VkColorSpaceKHR space;
			} formats;

			struct QueueFamilyIndices {
				uint32_t graphics;
				uint32_t present;
				uint32_t compute;
				uint32_t transfer;

			} queueFamilyIndices;
			operator VkDevice() { return this->logicalDevice; };
			
			// helpers
			uint32_t getQueueFamilyIndex( VkQueueFlagBits queueFlags );
			uint32_t getMemoryType( uint32_t typeBits, VkMemoryPropertyFlags properties, VkBool32 *memTypeFound = nullptr );
			int rate( VkPhysicalDevice device );
			
			VkCommandBuffer createCommandBuffer( VkCommandBufferLevel level, bool begin = false );
			void flushCommandBuffer( VkCommandBuffer commandBuffer, VkQueue queue, bool free = true );

			VkResult createBuffer(
				VkBufferUsageFlags usageFlags,
				VkMemoryPropertyFlags memoryPropertyFlags,
				VkDeviceSize size,
				VkBuffer* buffer, 
				VkDeviceMemory *memory,
				void *data = nullptr
			);
			VkResult createBuffer(
				VkBufferUsageFlags usageFlags,
				VkMemoryPropertyFlags memoryPropertyFlags,
				ext::vulkan::Buffer& buffer,
				VkDeviceSize size,
				void *data = nullptr
			);

			// RAII
			void initialize();
			void destroy();
		};
	}
}