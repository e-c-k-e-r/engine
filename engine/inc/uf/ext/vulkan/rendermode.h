#pragma once

#include <uf/utils/mesh/mesh.h>
#include <uf/utils/image/image.h>
#include <uf/ext/vulkan/device.h>

namespace ext {
	namespace vulkan {
		struct Graphic;
		
		struct UF_API RenderMode {
			bool execute = false;
			bool executed = false;
			bool rebuild = false;
			
			uint32_t width = 0;
			uint32_t height = 0;

			struct {			
				uf::Serializer json;

				uf::stl::string name = "";
				uf::stl::string type = "";
				uf::stl::string target = "";
				uf::stl::string pipeline = "";
				uf::stl::vector<uf::stl::string> pipelines;
				uf::stl::vector<uint8_t> outputs;

				uint8_t subpasses = 1;
				uint8_t samples = 1;
				uint8_t eyes = 1;
			} metadata;
			
			Device* device = VK_NULL_HANDLE;
			RenderTarget renderTarget;

			VkSemaphore renderCompleteSemaphore;
			uf::stl::vector<VkFence> fences;
			
			typedef uf::stl::vector<VkCommandBuffer> commands_container_t;
			std::thread::id mostRecentCommandPoolId;
			uf::ThreadUnique<commands_container_t> commands;

			virtual ~RenderMode();
			// RAII
			virtual const uf::stl::string getName() const;
			virtual const uf::stl::string getType() const;
			virtual RenderTarget& getRenderTarget(size_t = 0);
			virtual const RenderTarget& getRenderTarget(size_t = 0) const;

			virtual const size_t blitters() const;
			virtual ext::vulkan::Graphic* getBlitter(size_t = 0);
			virtual uf::stl::vector<ext::vulkan::Graphic*> getBlitters();
			
			virtual uf::Image screenshot(size_t = 0);

			virtual commands_container_t& getCommands();
			virtual commands_container_t& getCommands( std::thread::id );

			virtual GraphicDescriptor bindGraphicDescriptor( const GraphicDescriptor&, size_t = 0 );
			
			virtual void initialize( Device& device );
			virtual void createCommandBuffers();
			virtual void createCommandBuffers( const uf::stl::vector<ext::vulkan::Graphic*>& graphics );
			virtual void bindPipelines();
			virtual void bindPipelines( const uf::stl::vector<ext::vulkan::Graphic*>& graphics );
			virtual void tick();
			virtual void render();
			virtual void destroy();
			virtual void synchronize( uint64_t = UINT64_MAX );
			virtual void pipelineBarrier( VkCommandBuffer, uint8_t = -1 );
		};
	}
}