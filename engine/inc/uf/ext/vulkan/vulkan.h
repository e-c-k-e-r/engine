#pragma once

#include <uf/ext/vulkan.h>
#include <uf/ext/vulkan/device.h>
#include <uf/ext/vulkan/swapchain.h>
#include <uf/ext/vulkan/graphic.h>

namespace ext {
	namespace vulkan {
		VkResult CreateDebugUtilsMessengerEXT(
			VkInstance instance,
			const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
			const VkAllocationCallbacks* pAllocator,
			VkDebugUtilsMessengerEXT* pDebugMessenger
		);

		void DestroyDebugUtilsMessengerEXT(
			VkInstance instance,
			VkDebugUtilsMessengerEXT debugMessenger,
			const VkAllocationCallbacks* pAllocator
		);

		VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData
		);

		VkShaderModule loadShader(const char *fileName, VkDevice device);
		VkPipelineShaderStageCreateInfo loadShader( std::string fileName, VkShaderStageFlagBits stage, VkDevice device, std::vector<VkShaderModule>& shaderModules );

		uint32_t getMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties);

		typedef VmaAllocator Allocator;
		extern UF_API Allocator allocator;

		extern UF_API Device device;
		extern UF_API Swapchain swapchain;
		extern UF_API std::vector<Graphic*> graphics;
		extern UF_API std::vector<uint32_t> passes;
		extern UF_API bool resizedFramebuffer;
		extern UF_API uint32_t width;
		extern UF_API uint32_t height;
		extern UF_API bool validation;
		extern UF_API std::mutex mutex;

		void UF_API initialize( uint8_t = 0 );
		void UF_API tick();
		void UF_API render();
		void UF_API destroy();
		std::string UF_API allocatorStats();
	}
}