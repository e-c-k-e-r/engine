#pragma once

#include <uf/ext/vulkan/device.h>
#include <uf/ext/vulkan/swapchain.h>
#include <uf/ext/vulkan/initializers.h>
#include <uf/ext/vulkan/texture.h>
#include <uf/ext/vulkan/rendermodes/base.h>

namespace ext {
	namespace vulkan {
		struct RenderMode;
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
			RenderMode* renderMode = VK_NULL_HANDLE;

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

			template<typename T>
			inline void initializeDescriptorLayout( const std::vector<VkDescriptorSetLayoutBinding>& setLayoutBindings, const T& t ) {
				return initializeDescriptorLayout( setLayoutBindings, sizeof(t) );
			}
			void initializeDescriptorLayout( const std::vector<VkDescriptorSetLayoutBinding>& setLayoutBindings, std::size_t = 0 );
			void initializePipeline( VkGraphicsPipelineCreateInfo pipelineCreateInfo );
			void initializeDescriptorPool( const std::vector<VkDescriptorPoolSize>& poolSize, size_t length );
			void initializeDescriptorSet( const std::vector<VkWriteDescriptorSet>& writeDescriptorSets );

			// RAII
			virtual void initialize( const std::string& = "" );
			virtual void initialize( Device& device, RenderMode& renderMode );
			virtual void destroy();
			virtual void autoAssign();
			virtual bool autoAssignable() const;
			virtual std::string name() const;
			//
			virtual void render();
			virtual void createCommandBuffer( VkCommandBuffer );
			virtual void createImageMemoryBarrier( VkCommandBuffer );
			virtual void updateDynamicUniformBuffer(bool = false);
		};
	}
}
