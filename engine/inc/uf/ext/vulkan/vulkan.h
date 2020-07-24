#pragma once

#include <uf/ext/vulkan.h>
#include <uf/ext/vulkan/device.h>
#include <uf/ext/vulkan/swapchain.h>
#include <uf/ext/vulkan/graphic.h>
#include <uf/ext/vulkan/rendermodes/base.h>

#include <uf/engine/scene/scene.h>

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


		extern UF_API uint32_t width;
		extern UF_API uint32_t height;

		extern UF_API bool validation;
		extern UF_API Device device;
		typedef VmaAllocator Allocator;
		extern UF_API Allocator allocator;
		extern UF_API Swapchain swapchain;
		extern UF_API std::mutex mutex;

		extern UF_API bool rebuild;
		extern UF_API uint32_t currentBuffer;

		extern UF_API std::string currentPass;
		extern UF_API std::vector<std::string> passes;
	//	extern UF_API std::vector<Graphic*>* graphics;
		extern UF_API std::vector<RenderMode*> renderModes;
		extern UF_API std::vector<uf::Scene*> scenes;

		RenderMode& UF_API addRenderMode( RenderMode*, const std::string& = "" );
		RenderMode& UF_API getRenderMode( const std::string&, bool = true );

		void UF_API initialize( uint8_t = 0 );
		void UF_API tick();
		void UF_API render();
		void UF_API destroy();
		std::string UF_API allocatorStats();
	}
}