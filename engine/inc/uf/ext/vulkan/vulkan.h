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

		extern UF_API PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddressKHR;
		extern UF_API PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR;
		extern UF_API PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR;
		extern UF_API PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR;
		extern UF_API PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR;
		extern UF_API PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR;
		extern UF_API PFN_vkBuildAccelerationStructuresKHR vkBuildAccelerationStructuresKHR;
		extern UF_API PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR;
		extern UF_API PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR;
		extern UF_API PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR;
		extern UF_API PFN_vkCmdWriteAccelerationStructuresPropertiesKHR vkCmdWriteAccelerationStructuresPropertiesKHR;
		extern UF_API PFN_vkCmdCopyAccelerationStructureKHR vkCmdCopyAccelerationStructureKHR;

		uf::stl::string errorString( VkResult result );
		VkSampleCountFlagBits sampleCount( uint8_t );

		typedef VmaAllocator Allocator;

		namespace settings {
			constexpr size_t maxViews = 6;

			extern UF_API uint32_t width;
			extern UF_API uint32_t height;
			extern UF_API uint8_t msaa;
			extern UF_API bool validation;
			extern UF_API size_t viewCount;
			extern UF_API size_t gpuID;
			extern UF_API size_t scratchBufferAlignment;
			extern UF_API size_t scratchBufferInitialSize;
			extern UF_API size_t defaultTimeout;

			extern UF_API uf::stl::vector<uf::stl::string> validationFilters;
			extern UF_API uf::stl::vector<uf::stl::string> requestedDeviceFeatures;
			extern UF_API uf::stl::vector<uf::stl::string> requestedDeviceExtensions;
			extern UF_API uf::stl::vector<uf::stl::string> requestedInstanceExtensions;
			
			extern UF_API VkFilter swapchainUpscaleFilter;

			namespace experimental {
				extern UF_API bool dedicatedThread;
				extern UF_API bool rebuildOnTickBegin;
				extern UF_API bool batchQueueSubmissions;
				extern UF_API bool enableMultiGPU;
			}

			namespace invariant {
				extern UF_API bool waitOnRenderEnd;
				extern UF_API bool individualPipelines;
				extern UF_API bool multithreadedRecording;

				extern UF_API uf::stl::string deferredMode;
				extern UF_API bool multiview;
			}

			namespace pipelines {
				extern UF_API bool deferred;
				extern UF_API bool vsync;
				extern UF_API bool hdr;
				extern UF_API bool vxgi;
				extern UF_API bool culling;
				extern UF_API bool occlusion;
				extern UF_API bool bloom;
				extern UF_API bool rt;
				extern UF_API bool postProcess;
				extern UF_API bool fsr;

				namespace names {
					extern UF_API uf::stl::string deferred;
					extern UF_API uf::stl::string vsync;
					extern UF_API uf::stl::string hdr;
					extern UF_API uf::stl::string vxgi;
					extern UF_API uf::stl::string culling;
					extern UF_API uf::stl::string occlusion;
					extern UF_API uf::stl::string bloom;
					extern UF_API uf::stl::string rt;
					extern UF_API uf::stl::string postProcess;
					extern UF_API uf::stl::string fsr;
				}
			}

			namespace formats {
				extern UF_API VkColorSpaceKHR colorSpace;
				extern UF_API ext::vulkan::enums::Format::type_t color;
				extern UF_API ext::vulkan::enums::Format::type_t depth;
			}
		}
		namespace states {
			extern UF_API bool rebuild;
			extern UF_API bool resized;
			extern UF_API uint32_t currentBuffer;
			
			extern UF_API uint32_t frameAccumulate;
			extern UF_API bool frameAccumulateReset;
		}

		extern UF_API Device device;
		extern UF_API Allocator allocator;
		extern UF_API Swapchain swapchain;
		extern UF_API std::mutex mutex;

	//	extern UF_API RenderMode* currentRenderMode;
		extern UF_API uf::stl::vector<RenderMode*> renderModes;
		extern UF_API uf::stl::unordered_map<uf::stl::string, RenderMode*> renderModesMap;
		extern UF_API uf::ThreadUnique<RenderMode*> currentRenderMode;

		extern UF_API Buffer scratchBuffer;

		bool UF_API hasRenderMode( const uf::stl::string&, bool = true );
		RenderMode& UF_API addRenderMode( RenderMode*, const uf::stl::string& = "" );
		RenderMode& UF_API getRenderMode( const uf::stl::string&, bool = true );
		uf::stl::vector<RenderMode*> UF_API getRenderModes( const uf::stl::string&, bool = true );
		uf::stl::vector<RenderMode*> UF_API getRenderModes( const uf::stl::vector<uf::stl::string>&, bool = true );
		void UF_API removeRenderMode( RenderMode*, bool = true );
		RenderMode* UF_API getCurrentRenderMode();
		RenderMode* UF_API getCurrentRenderMode( std::thread::id );
		void UF_API setCurrentRenderMode( RenderMode* renderMode );
		void UF_API setCurrentRenderMode( RenderMode* renderMode,  std::thread::id );

		void UF_API initialize( /*uint8_t = 0*/ );
		void UF_API tick();
		void UF_API render();
		void UF_API destroy();
		void UF_API synchronize( uint8_t = 0b11 );
		uf::stl::string UF_API allocatorStats();
		ext::vulkan::enums::Format::type_t UF_API formatFromString( const uf::stl::string& );
	}
}