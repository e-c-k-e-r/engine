#pragma once

#include <uf/ext/vulkan.h>
#include <uf/ext/vulkan/device.h>
#include <uf/ext/vulkan/swapchain.h>
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
		uint32_t getMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties);

		struct VertexDescriptor {
			VkFormat format; // VK_FORMAT_R32G32B32_SFLOAT
			std::size_t offset; // offsetof(Vertex, position)
		};
		typedef VmaAllocator Allocator;

		namespace settings {
			extern UF_API uint32_t width;
			extern UF_API uint32_t height;
			extern UF_API uint8_t msaa;
			extern UF_API bool validation;

			extern UF_API std::vector<std::string> validationFilters;
			extern UF_API std::vector<std::string> requestedDeviceFeatures;
			extern UF_API std::vector<std::string> requestedDeviceExtensions;
			extern UF_API std::vector<std::string> requestedInstanceExtensions;
			
			extern UF_API VkFilter swapchainUpscaleFilter;

			namespace experimental {
				extern UF_API bool rebuildOnTickBegin;
				extern UF_API bool waitOnRenderEnd;
				extern UF_API bool individualPipelines;
				extern UF_API bool multithreadedCommandRecording;
				extern UF_API bool deferredReconstructPosition;
				extern UF_API bool deferredAliasOutputToSwapchain;
				extern UF_API bool multiview;
				extern UF_API bool hdr;
			}

			namespace formats {
				extern UF_API VkColorSpaceKHR colorSpace;
				extern UF_API VkFormat color;
				extern UF_API VkFormat depth;
				extern UF_API VkFormat normal;
				extern UF_API VkFormat position;
			}
		}
		namespace states {
			extern UF_API bool rebuild;
			extern UF_API bool resized;
			extern UF_API uint32_t currentBuffer;
		}

		extern UF_API Device device;
		extern UF_API Allocator allocator;
		extern UF_API Swapchain swapchain;
		extern UF_API std::mutex mutex;

		extern UF_API RenderMode* currentRenderMode;
		extern UF_API std::vector<RenderMode*> renderModes;
		extern UF_API std::vector<uf::Scene*> scenes;

		bool UF_API hasRenderMode( const std::string&, bool = true );
		RenderMode& UF_API addRenderMode( RenderMode*, const std::string& = "" );
		RenderMode& UF_API getRenderMode( const std::string&, bool = true );
		std::vector<RenderMode*> UF_API getRenderModes( const std::string&, bool = true );
		std::vector<RenderMode*> UF_API getRenderModes( const std::vector<std::string>&, bool = true );
		void UF_API removeRenderMode( RenderMode*, bool = true );

		void UF_API initialize( uint8_t = 0 );
		void UF_API tick();
		void UF_API render();
		void UF_API destroy();
		void UF_API synchronize( uint8_t = 0b11 );
		std::string UF_API allocatorStats();
		VkFormat UF_API formatFromString( const std::string& );
	}
}