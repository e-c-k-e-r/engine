#pragma once

#include <uf/ext/vulkan/vk.h>
#include <uf/ext/vulkan/buffer.h>
#include <uf/utils/window/window.h>
#include <uf/utils/thread/thread.h>
#include <uf/utils/thread/perthread.h>

namespace ext {
	namespace vulkan {
		struct UF_API Device {
			VkInstance instance;
			VkDebugUtilsMessengerEXT debugMessenger;
			VkSurfaceKHR surface;
			VkPhysicalDevice physicalDevice;
			VkDevice logicalDevice;
			struct {
			//	std::unordered_map<std::thread::id, VkCommandPool> graphics;
			//	std::unordered_map<std::thread::id, VkCommandPool> compute;
			//	std::unordered_map<std::thread::id, VkCommandPool> transfer;
				uf::ThreadUnique<VkCommandPool> graphics;
				uf::ThreadUnique<VkCommandPool> compute;
				uf::ThreadUnique<VkCommandPool> transfer;
			} commandPool;

			VkPhysicalDeviceProperties properties;
			VkPhysicalDeviceFeatures features;
			VkPhysicalDeviceFeatures enabledFeatures;
			VkPhysicalDeviceMemoryProperties memoryProperties;
			
			VkPhysicalDeviceProperties2 properties2;
			VkPhysicalDeviceFeatures2 features2;
			VkPhysicalDeviceFeatures2 enabledFeatures2;
			VkPhysicalDeviceMemoryProperties2 memoryProperties2;

			struct {
				std::vector<VkExtensionProperties> instance;
				std::vector<VkExtensionProperties> device;
			} extensionProperties;
			struct {
				std::vector<std::string> instance;
				std::vector<std::string> device;
			} supportedExtensions;
			
			VkPipelineCache pipelineCache;

			std::vector<VkQueueFamilyProperties> queueFamilyProperties;
			
			struct {
			//	std::unordered_map<std::thread::id,VkQueue> graphics;
			//	std::unordered_map<std::thread::id,VkQueue> present;
			//	std::unordered_map<std::thread::id,VkQueue> compute;
			//	std::unordered_map<std::thread::id,VkQueue> transfer;
				uf::ThreadUnique<VkQueue> graphics;
				uf::ThreadUnique<VkQueue> present;
				uf::ThreadUnique<VkQueue> compute;
				uf::ThreadUnique<VkQueue> transfer;
			} queues;

			uf::Window* window;

		/*
			struct {
				VkFormat depth;
				VkFormat color;
				VkColorSpaceKHR space;
			} formats;
		*/

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
				VkBufferUsageFlags usage,
				VkMemoryPropertyFlags memoryProperties,
				VkDeviceSize size,
				VkBuffer* buffer, 
				VkDeviceMemory *memory,
				void *data = nullptr
			);
			VkResult createBuffer(
				VkBufferUsageFlags usage,
				VkMemoryPropertyFlags memoryProperties,
				ext::vulkan::Buffer& buffer,
				VkDeviceSize size,
				void *data = nullptr
			);

			enum QueueEnum {
				GRAPHICS,
				PRESENT,
				COMPUTE,
				TRANSFER,
			};
			VkQueue& getQueue( QueueEnum );
			VkCommandPool& getCommandPool( QueueEnum );
			VkQueue& getQueue( QueueEnum, std::thread::id );
			VkCommandPool& getCommandPool( QueueEnum, std::thread::id );

			// RAII
			void initialize();
			void destroy();
		};
	}
}