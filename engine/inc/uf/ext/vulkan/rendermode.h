#pragma once

#include <uf/utils/mesh/mesh.h>
#include <uf/utils/image/image.h>
#include <uf/ext/vulkan/device.h>
#include <uf/ext/vulkan/buffer.h>
#include <uf/ext/vulkan/texture.h>

namespace ext {
	namespace vulkan {
		struct Graphic;
		
		struct UF_API RenderMode : public Buffers {
			bool execute = false;
			bool executed = false;
			bool rebuild = false;
			bool resized = false;
			
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
				uf::stl::unordered_map<uf::stl::string, uint8_t> attachments;

				struct {
					float frequency = 0.0f;
					float timer = 0.0f;
					bool execute = true;
				} limiter;

				uint8_t subpasses = 1;
				uint8_t samples = 1;
				uint8_t eyes = 1;
				uint8_t views = 1;
				bool compute = false;
			} metadata;
			
			Device* device = VK_NULL_HANDLE;
			RenderTarget renderTarget;

			uf::stl::vector<Texture2D> textures;

			VkSemaphore renderCompleteSemaphore;
			uf::stl::vector<VkFence> fences;
			
			typedef uf::stl::vector<VkCommandBuffer> commands_container_t;
			std::thread::id mostRecentCommandPoolId;
			uf::ThreadUnique<commands_container_t> commands;

			constexpr static int32_t CALLBACK_BEGIN = -1;
			constexpr static int32_t CALLBACK_END = -2;
			constexpr static int32_t EXECUTE_BEGIN = -3;
			constexpr static int32_t EXECUTE_END = -4;
			
			typedef std::function<void(VkCommandBuffer, size_t)> callback_t;
			uf::stl::unordered_map<int32_t, callback_t> commandBufferCallbacks;
			
			void bindCallback( int32_t, const callback_t& );
			
			uf::Image screenshot(size_t = 0, size_t = 0);

			commands_container_t& getCommands( std::thread::id = std::this_thread::get_id() );
			void lockMutex( std::thread::id = std::this_thread::get_id() );
			bool tryMutex( std::thread::id = std::this_thread::get_id() );
			void unlockMutex( std::thread::id = std::this_thread::get_id() );
			std::lock_guard<std::mutex> guardMutex( std::thread::id = std::this_thread::get_id() );
			
			void cleanupAllCommands();
			void cleanupCommands( std::thread::id = std::this_thread::get_id() );

			virtual ~RenderMode();
			// RAII
			virtual const uf::stl::string getName() const;
			virtual const uf::stl::string getType() const;
			virtual RenderTarget& getRenderTarget(size_t = 0);
			virtual const RenderTarget& getRenderTarget(size_t = 0) const;

			virtual bool hasAttachment( const uf::stl::string& ) const;
			virtual const RenderTarget::Attachment& getAttachment( const uf::stl::string& ) const;
			virtual size_t getAttachmentIndex( const uf::stl::string& ) const;

			virtual const size_t blitters() const;
			virtual ext::vulkan::Graphic* getBlitter(size_t = 0);
			virtual uf::stl::vector<ext::vulkan::Graphic*> getBlitters();
			

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

			virtual VkSubmitInfo queue();
		};
	}
}