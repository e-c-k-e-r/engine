#pragma once

#include <uf/ext/vulkan/device.h>

namespace ext {
	namespace vulkan {
		struct Graphic;
		
		struct UF_API RenderMode {
			bool execute = false;
			bool rebuild = false;
			uint32_t width = 0, height = 0;
			std::string name = "";
			
			Device* device = VK_NULL_HANDLE;
			RenderTarget renderTarget;

			VkSemaphore renderCompleteSemaphore;
			std::vector<VkFence> fences;
			
			typedef std::vector<VkCommandBuffer> commands_container_t;
			std::thread::id mostRecentCommandPoolId;
		//	std::unordered_map<std::thread::id, commands_container_t> commands;
			uf::ThreadUnique<commands_container_t> commands;

			virtual ~RenderMode();
			// RAII
			virtual std::string getType() const;
			const std::string& getName( bool = false ) const;
			virtual RenderTarget& getRenderTarget(size_t = 0);
			virtual const RenderTarget& getRenderTarget(size_t = 0) const;

			virtual commands_container_t& getCommands();
			virtual commands_container_t& getCommands( std::thread::id );
			
			virtual void initialize( Device& device );
			virtual void createCommandBuffers();
			virtual void createCommandBuffers( const std::vector<ext::vulkan::Graphic*>& graphics );
			virtual void bindPipelines();
			virtual void bindPipelines( const std::vector<ext::vulkan::Graphic*>& graphics );
			virtual void tick();
			virtual void render();
			virtual void destroy();
			virtual void synchronize( uint64_t = UINT64_MAX );
			virtual void pipelineBarrier( VkCommandBuffer, uint8_t = -1 );
		};
	}
}