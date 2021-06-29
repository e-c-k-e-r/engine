#pragma once

#include <uf/ext/vulkan/vk.h>
#include <uf/ext/vulkan/enums.h>
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

		uf::stl::string errorString( VkResult result );
		VkSampleCountFlagBits sampleCount( uint8_t );
		void* alignedAlloc( size_t size, size_t alignment );
		void alignedFree(void* data);

		typedef VmaAllocator Allocator;

		namespace settings {
			extern UF_API uint32_t width;
			extern UF_API uint32_t height;
			extern UF_API uint8_t msaa;
			extern UF_API bool validation;
			extern UF_API size_t viewCount;
			constexpr size_t maxViews = 6;

			extern UF_API uf::stl::vector<uf::stl::string> validationFilters;
			extern UF_API uf::stl::vector<uf::stl::string> requestedDeviceFeatures;
			extern UF_API uf::stl::vector<uf::stl::string> requestedDeviceExtensions;
			extern UF_API uf::stl::vector<uf::stl::string> requestedInstanceExtensions;
			
			extern UF_API VkFilter swapchainUpscaleFilter;

			namespace experimental {
				extern UF_API bool rebuildOnTickBegin;
				extern UF_API bool waitOnRenderEnd;
				extern UF_API bool individualPipelines;
				extern UF_API bool multithreadedCommandRecording;
				extern UF_API bool multithreadedCommandRendering;
				extern UF_API uf::stl::string deferredMode;
				extern UF_API bool deferredReconstructPosition;
				extern UF_API bool deferredAliasOutputToSwapchain;
				extern UF_API bool multiview;
				extern UF_API bool vsync;
				extern UF_API bool hdr;
				extern UF_API bool frustrumCull;
				extern UF_API bool vxgi;
				extern UF_API bool deferredSampling;
			}

			namespace formats {
				extern UF_API VkColorSpaceKHR colorSpace;
				extern UF_API ext::vulkan::enums::Format::type_t color;
				extern UF_API ext::vulkan::enums::Format::type_t depth;
				extern UF_API ext::vulkan::enums::Format::type_t normal;
				extern UF_API ext::vulkan::enums::Format::type_t position;
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
		extern UF_API uf::stl::vector<RenderMode*> renderModes;
		extern UF_API uf::stl::unordered_map<uf::stl::string, RenderMode*> renderModesMap;
		extern UF_API uf::stl::vector<uf::Scene*> scenes;

		bool UF_API hasRenderMode( const uf::stl::string&, bool = true );
		RenderMode& UF_API addRenderMode( RenderMode*, const uf::stl::string& = "" );
		RenderMode& UF_API getRenderMode( const uf::stl::string&, bool = true );
		uf::stl::vector<RenderMode*> UF_API getRenderModes( const uf::stl::string&, bool = true );
		uf::stl::vector<RenderMode*> UF_API getRenderModes( const uf::stl::vector<uf::stl::string>&, bool = true );
		void UF_API removeRenderMode( RenderMode*, bool = true );

		void UF_API initialize( /*uint8_t = 0*/ );
		void UF_API tick();
		void UF_API render();
		void UF_API destroy();
		void UF_API synchronize( uint8_t = 0b11 );
		uf::stl::string UF_API allocatorStats();
		ext::vulkan::enums::Format::type_t UF_API formatFromString( const uf::stl::string& );
	}
}