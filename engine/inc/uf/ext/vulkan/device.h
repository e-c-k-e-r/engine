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
			struct {
				VkCommandPool graphics = VK_NULL_HANDLE;
				VkCommandPool compute = VK_NULL_HANDLE;
				VkCommandPool transfer = VK_NULL_HANDLE;
			} commandPool;

			VkPhysicalDeviceProperties properties;
			VkPhysicalDeviceFeatures features;
			VkPhysicalDeviceFeatures enabledFeatures;
			VkPhysicalDeviceMemoryProperties memoryProperties;
			
			VkPhysicalDeviceProperties2 properties2;
			VkPhysicalDeviceFeatures2 features2;
			VkPhysicalDeviceFeatures2 enabledFeatures2;
			VkPhysicalDeviceMemoryProperties2 memoryProperties2;
			
			VkPipelineCache pipelineCache;

			std::vector<VkQueueFamilyProperties> queueFamilyProperties;
			std::vector<std::string> supportedExtensions;
			
			struct {
				VkQueue graphics;
				VkQueue present;
				VkQueue compute;
				VkQueue transfer;
			} queues;

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
			void flushCommandBuffer( VkCommandBuffer commandBuffer, bool free = true );

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