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
			ACQUIRE,
		};
		struct CommandBuffer {
			bool immediate{true};
			QueueEnum queueType{QueueEnum::TRANSFER};
			VkCommandBuffer handle{VK_NULL_HANDLE};
			VkFence fence{VK_NULL_HANDLE};
			uf::thread::id_t threadId{uf::thread::current_id()};
			
			pod::Checkpoint* checkpoint = NULL;

			operator VkCommandBuffer() { return handle; }
		};

		struct DescriptorAllocator {
			VkDevice device = VK_NULL_HANDLE;
			VkDescriptorPool currentPool = VK_NULL_HANDLE;
			uf::stl::vector<VkDescriptorPool> usedPools;

			uf::stl::vector<VkDescriptorPoolSize> poolSizes = {
				{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
				{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4000 },
				{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2000 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
				{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 500 },
				{ VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 100 }
			};

			void initialize( VkDevice );
			void destroy();
			VkDescriptorPool createPool();
			bool allocate( VkDescriptorSet* set, VkDescriptorSetLayout layout );
		};

		struct Texture;

		struct UF_API Device {
			VkInstance instance;
			VkDebugUtilsMessengerEXT debugMessenger;
			VkSurfaceKHR surface;
			bool surfaceless = false;
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
			DescriptorAllocator descriptorAllocator;

			uf::stl::vector<VkQueueFamilyProperties> queueFamilyProperties;
			
			struct {
				uf::ThreadUnique<VkQueue> graphics;
				uf::ThreadUnique<VkQueue> present;
				uf::ThreadUnique<VkQueue> compute;
				uf::ThreadUnique<VkQueue> transfer;
				uf::ThreadUnique<VkQueue> acquire;
			} queues;
			
			struct {
				struct CommandBuffer {
					uf::stl::vector<VkCommandBuffer> commandBuffers;
					uf::stl::vector<VkFence> fences;
				};

				uf::stl::unordered_map<QueueEnum, uf::stl::unordered_map<uf::thread::id_t, CommandBuffer>> commandBuffers;
				
				uf::stl::vector<Buffer> buffers;
				uf::stl::vector<AccelerationStructure> ass;
				uf::stl::vector<Texture> textures;
			} transient;

			struct {
				uf::stl::stack<VkFence> fences;
				uf::stl::unordered_map<QueueEnum, uf::stl::unordered_map<uf::thread::id_t, uf::stl::stack<VkCommandBuffer>>> commandBuffers;
			} reusable;

			uf::stl::unordered_map<VkCommandBuffer, pod::Checkpoint*> checkpoints;

			// queue API calls (submit/waitIdle/present/checkpoint reads) must not overlap on the same VkQueue, even from different threads
			struct QueueLocks {
				std::mutex mutex;
				uf::stl::unordered_map<VkQueue, std::unique_ptr<std::mutex>> locks;
				uf::stl::unordered_map<VkCommandPool, std::unique_ptr<std::mutex>> poolLocks;
			};
			std::unique_ptr<QueueLocks> queueLocks;

			uf::Window* window;

			struct QueueFamilyIndices {
				uint32_t graphics;
				uint32_t present;
				uint32_t compute;
				uint32_t transfer;
				uint32_t acquire;
			} queueFamilyIndices, queueIndices;
			operator VkDevice() { return this->logicalDevice; };
			
			// helpers
			uint32_t getQueueFamilyIndex( VkQueueFlagBits queueFlags );
			uint32_t getMemoryType( uint32_t typeBits, VkMemoryPropertyFlags properties, VkBool32 *memTypeFound = nullptr );

			VkCommandBuffer createCommandBuffer( VkCommandBufferLevel level, QueueEnum queue, bool begin = true, bool singleton = true );
			void flushCommandBuffer( VkCommandBuffer commandBuffer, QueueEnum queue, bool wait = VK_DEFAULT_COMMAND_BUFFER_IMMEDIATE );
			pod::Checkpoint* markCommandBuffer( VkCommandBuffer commandBuffer, pod::Checkpoint::Type type, const uf::stl::string& name, const uf::stl::string& info );

			CommandBuffer fetchCommandBuffer( QueueEnum queue, bool immediate = VK_DEFAULT_COMMAND_BUFFER_IMMEDIATE );
			CommandBuffer fetchCommandBuffer( QueueEnum queue, VkCommandBufferLevel, bool immediate = VK_DEFAULT_COMMAND_BUFFER_IMMEDIATE );
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
			VkQueue getQueue( QueueEnum, uf::thread::id_t );
			VkCommandPool getCommandPool( QueueEnum, uf::thread::id_t );
			std::unique_lock<std::mutex> lockQueue( VkQueue queue );
			std::unique_lock<std::mutex> lockPool( VkCommandPool pool );

			// RAII
			void initialize();
			void destroy();
		};
	}
}