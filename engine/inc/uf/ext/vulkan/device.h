#pragma once

#include <uf/ext/vulkan/vk.h>
#include <uf/ext/vulkan/buffer.h>
#include <uf/utils/window/window.h>
#include <uf/utils/thread/thread.h>
#include <uf/utils/thread/perthread.h>

namespace ext {
	namespace vulkan {
		enum QueueEnum {
			GRAPHICS,
			PRESENT,
			COMPUTE,
			TRANSFER,
		};
		struct CommandBuffer {
			bool immediate{true};
			QueueEnum queueType{QueueEnum::TRANSFER};
			VkCommandBuffer handle{VK_NULL_HANDLE};
			std::thread::id threadId{std::this_thread::get_id()};

			operator VkCommandBuffer() { return handle; }
		};

		struct UF_API Device {
			VkInstance instance;
			VkDebugUtilsMessengerEXT debugMessenger;
			VkSurfaceKHR surface;
			VkPhysicalDevice physicalDevice;
			VkDevice logicalDevice;
			struct {
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
				uf::stl::vector<VkExtensionProperties> instance;
				uf::stl::vector<VkExtensionProperties> device;
			} extensionProperties;
			struct {
				uf::stl::vector<uf::stl::string> instance;
				uf::stl::vector<uf::stl::string> device;
			} supportedExtensions;
			
			VkPipelineCache pipelineCache;

			uf::stl::vector<VkQueueFamilyProperties> queueFamilyProperties;
			
			struct {
				uf::ThreadUnique<VkQueue> graphics;
				uf::ThreadUnique<VkQueue> present;
				uf::ThreadUnique<VkQueue> compute;
				uf::ThreadUnique<VkQueue> transfer;
			} queues;

			struct {
				uf::stl::vector<Buffer> buffers;
				uf::stl::unordered_map<QueueEnum, uf::stl::unordered_map<std::thread::id, std::vector<VkCommandBuffer>>> commandBuffers;
			} transient;

		/*
			struct {
				uf::stl::vector<Buffer> buffers;
				uf::stl::unordered_map<QueueEnum, CommandBuffer> commandBuffers;
			} reusable;
		*/

			uf::Window* window;

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

			VkCommandBuffer createCommandBuffer( VkCommandBufferLevel level, QueueEnum queue, bool begin = true );
			void flushCommandBuffer( VkCommandBuffer commandBuffer, QueueEnum queue, bool wait = false );

		//	CommandBuffer fetchCommandBuffer( QueueEnum queue );
			CommandBuffer fetchCommandBuffer( QueueEnum queue, bool waits = VK_DEFAULT_COMMAND_BUFFER_WAIT );
			void flushCommandBuffer( CommandBuffer commandBuffer );

			ext::vulkan::Buffer createBuffer(
				const void* data,
				VkDeviceSize size,
				VkBufferUsageFlags usage,
				VkMemoryPropertyFlags memoryProperties
			);
			VkResult createBuffer(
				const void* data,
				VkDeviceSize size,
				VkBufferUsageFlags usage,
				VkMemoryPropertyFlags memoryProperties,
				ext::vulkan::Buffer& buffer
			);
			ext::vulkan::Buffer fetchTransientBuffer(
				const void* data,
				VkDeviceSize size,
				VkBufferUsageFlags usage,
				VkMemoryPropertyFlags memoryProperties
			);

			VkQueue getQueue( QueueEnum );
			VkCommandPool getCommandPool( QueueEnum );
			VkQueue getQueue( QueueEnum, std::thread::id );
			VkCommandPool getCommandPool( QueueEnum, std::thread::id );

			// RAII
			void initialize();
			void destroy();
		};
	}
}