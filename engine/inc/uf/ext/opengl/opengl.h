#pragma once

#include <uf/ext/opengl/ogl.h>
#include <uf/ext/opengl/enums.h>
#include <uf/ext/opengl/device.h>
#include <uf/ext/opengl/swapchain.h>
#include <uf/ext/opengl/rendermodes/base.h>

#include <uf/engine/scene/scene.h>

namespace ext {
	namespace opengl {
		namespace settings {
			extern UF_API uint32_t width;
			extern UF_API uint32_t height;
			extern UF_API uint8_t msaa;
			extern UF_API bool validation;
			extern UF_API bool defaultStageBuffers;
			extern UF_API size_t viewCount;
			constexpr size_t maxViews = 6;

			extern UF_API uf::stl::vector<uf::stl::string> validationFilters;
			extern UF_API uf::stl::vector<uf::stl::string> requestedDeviceFeatures;
			extern UF_API uf::stl::vector<uf::stl::string> requestedDeviceExtensions;
			extern UF_API uf::stl::vector<uf::stl::string> requestedInstanceExtensions;
			
			extern UF_API ext::opengl::enums::Filter::type_t swapchainUpscaleFilter;

			namespace experimental {
				extern UF_API bool dedicatedThread;
				extern UF_API bool rebuildOnTickBegin;
				extern UF_API bool batchQueueSubmissions;
			}

			namespace invariant {
				extern UF_API bool waitOnRenderEnd;
				extern UF_API bool individualPipelines;
				extern UF_API bool multithreadedRecording;

				extern UF_API uf::stl::string deferredMode;
				extern UF_API bool deferredAliasOutputToSwapchain;
				extern UF_API bool deferredSampling;
				extern UF_API bool multiview;
			}

			namespace pipelines {
				extern UF_API bool vsync;
				extern UF_API bool hdr;
				extern UF_API bool vxgi;
				extern UF_API bool culling;
				extern UF_API bool bloom;
				extern UF_API bool rt;

				namespace names {
					extern UF_API uf::stl::string vsync;
					extern UF_API uf::stl::string hdr;
					extern UF_API uf::stl::string vxgi;
					extern UF_API uf::stl::string culling;
					extern UF_API uf::stl::string bloom;
					extern UF_API uf::stl::string rt;
				}
			}
			
			namespace formats {
				extern UF_API GLhandle(VkColorSpaceKHR) colorSpace;
				extern UF_API ext::opengl::enums::Format::type_t color;
				extern UF_API ext::opengl::enums::Format::type_t depth;
				extern UF_API ext::opengl::enums::Format::type_t normal;
				extern UF_API ext::opengl::enums::Format::type_t position;
			}
		}
		namespace states {
			extern UF_API bool rebuild;
			extern UF_API bool resized;
			extern UF_API uint32_t currentBuffer;
		}

		extern UF_API Device device;
		extern UF_API std::mutex mutex;
		extern UF_API std::mutex immediateModeMutex;

	//	extern UF_API RenderMode* currentRenderMode;
		extern UF_API uf::stl::vector<RenderMode*> renderModes;
		extern UF_API uf::ThreadUnique<RenderMode*> currentRenderMode;
		
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
		
		ext::opengl::enums::Format::type_t UF_API formatFromString( const uf::stl::string& );

		uf::stl::string UF_API errorString();
		uf::stl::string UF_API errorString( GLenum );
	};
}