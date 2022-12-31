#pragma once

#include <uf/ext/vulkan/vk.h>
#include <uf/ext/vulkan/buffer.h>
#include <uf/utils/window/window.h>
#include <uf/utils/thread/thread.h>
#include <uf/utils/thread/perthread.h>
#include <uf/utils/memory/stack.h>
#include <uf/utils/debug/checkpoint.h>

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
			VkFence fence{VK_NULL_HANDLE};
			std::thread::id threadId{std::this_thread::get_id()};
			
			pod::Checkpoint* checkpoint = NULL;

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
				struct {
					uf::stl::vector<VkExtensionProperties> instance;
					uf::stl::vector<VkExtensionProperties> device;
				} properties;
				struct {
					uf::stl::vector<uf::stl::string> instance;
					uf::stl::vector<uf::stl::string> device;
				} supported;
				struct {
					uf::stl::unordered_map<uf::stl::string, bool> instance;
					uf::stl::unordered_map<uf::stl::string, bool> device;
				} enabled;
			} extensions;
			
			VkPipelineCache pipelineCache;

			uf::stl::vector<VkQueueFamilyProperties> queueFamilyProperties;
			
			struct {
				uf::ThreadUnique<VkQueue> graphics;
				uf::ThreadUnique<VkQueue> present;
				uf::ThreadUnique<VkQueue> compute;
				uf::ThreadUnique<VkQueue> transfer;
			} queues;

			struct {
				struct CommandBuffer {
					std::vector<VkCommandBuffer> commandBuffers;
					std::vector<VkFence> fences;
				};

				uf::stl::unordered_map<QueueEnum, uf::stl::unordered_map<std::thread::id, CommandBuffer>> commandBuffers;
				
				uf::stl::vector<Buffer> buffers;
				uf::stl::vector<AccelerationStructure> ass;
			} transient;

			struct {
				uf::stl::stack<VkFence> fences;
			} reusable;

			uf::stl::unordered_map<VkCommandBuffer, pod::Checkpoint*> checkpoints;

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
			void flushCommandBuffer( VkCommandBuffer commandBuffer, QueueEnum queue, bool wait = VK_DEFAULT_COMMAND_BUFFER_IMMEDIATE );
			pod::Checkpoint* markCommandBuffer( VkCommandBuffer commandBuffer, pod::Checkpoint::Type type, const uf::stl::string& name, const uf::stl::string& info );

			CommandBuffer fetchCommandBuffer( QueueEnum queue, bool waits = VK_DEFAULT_COMMAND_BUFFER_IMMEDIATE );
			void flushCommandBuffer( CommandBuffer& commandBuffer );
			pod::Checkpoint* markCommandBuffer( CommandBuffer& commandBuffer, pod::Checkpoint::Type type, const uf::stl::string& name, const uf::stl::string& info );

			VkFence getFence();
			void destroyFence( VkFence );

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