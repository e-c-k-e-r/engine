#pragma once

#include <uf/ext/vulkan/device.h>
#include <uf/ext/vulkan/swapchain.h>
#include <uf/ext/vulkan/initializers.h>
#include <uf/ext/vulkan/texture.h>

namespace ext {
	namespace vulkan {
		struct UF_API Graphic {
			VkPipeline pipeline;
			VkPipelineLayout pipelineLayout;
			VkDescriptorSet descriptorSet;
			VkDescriptorSetLayout descriptorSetLayout;
			VkDescriptorPool descriptorPool;

			struct {
				std::vector<VkShaderModule> modules;
				std::vector<VkPipelineShaderStageCreateInfo> stages;
			} shader;
			std::vector<Buffer> buffers;

			Device* device = VK_NULL_HANDLE;
			Swapchain* swapchain = VK_NULL_HANDLE;

			bool autoAssigned = false;
			bool initialized = false;
			bool process = true;

			virtual ~Graphic();
			//
			template<typename T>
			inline size_t initializeBuffer( T data, VkDeviceSize length, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, bool stage = true ) {
				return initializeBuffer( (void*) &data, length, usageFlags, memoryPropertyFlags, stage );
			}
			template<typename T>
			inline void updateBuffer( T data, VkDeviceSize length, size_t index = 0, bool stage = true ) {
				return updateBuffer( (void*) &data, length, index, stage );
			}
			template<typename T>
			inline size_t initializeBuffer( T data, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, bool stage = true ) {
				return initializeBuffer( (void*) &data, static_cast<VkDeviceSize>(sizeof(T)), usageFlags, memoryPropertyFlags, stage );
			}
			template<typename T>
			inline void updateBuffer( T data, size_t index = 0, bool stage = true ) {
				return updateBuffer( (void*) &data, static_cast<VkDeviceSize>(sizeof(T)), index, stage );
			}
			template<typename T>
			inline void updateBuffer( T data, VkDeviceSize length, Buffer& buffer, bool stage = true ) {
				return updateBuffer( (void*) &data, length, buffer, stage );
			}
			size_t initializeBuffer( void* data, VkDeviceSize length, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, bool stage = true );
			void updateBuffer( void* data, VkDeviceSize length, size_t index = 0, bool stage = true );
			void updateBuffer( void* data, VkDeviceSize length, Buffer& buffer, bool stage = true );

			void initializeShaders( const std::vector<std::pair<std::string, VkShaderStageFlagBits>>& );
			void initializeDescriptorLayout( const std::vector<VkDescriptorSetLayoutBinding>& setLayoutBindings );
			void initializePipeline( VkGraphicsPipelineCreateInfo pipelineCreateInfo );
			void initializeDescriptorPool( const std::vector<VkDescriptorPoolSize>& poolSize, size_t length );
			void initializeDescriptorSet( const std::vector<VkWriteDescriptorSet>& writeDescriptorSets );

			// RAII
			virtual void initialize( Device& device, Swapchain& swapchain );
			virtual void destroy();
			virtual void autoAssign();
			virtual bool autoAssignable() const;
			virtual std::string name() const;
			//
			virtual void render();
			virtual void createCommandBuffer( VkCommandBuffer, uint32_t = 0 );
			virtual void createImageMemoryBarrier( VkCommandBuffer );
			virtual void updateDynamicUniformBuffer(bool = false);
		};
	}
}
