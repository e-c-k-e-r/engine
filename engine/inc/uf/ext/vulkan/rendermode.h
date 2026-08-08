#pragma once

#include <uf/utils/mesh/mesh.h>
#include <uf/utils/image/image.h>
#include <uf/utils/camera/camera.h>
#include <uf/ext/vulkan/device.h>
#include <uf/ext/vulkan/buffer.h>
#include <uf/ext/vulkan/texture.h>
#include <uf/ext/vulkan/graphic.h>

#define VK_COMMAND_BUFFER_CALLBACK( pass, commandBuffer, i, f ) {\
	auto it = commandBufferCallbacks.find(pass);\
	if ( it != commandBufferCallbacks.end() ) {\
		commandBufferCallbacks[pass]( commandBuffer, i );\
		f;\
	}\
}

namespace ext {
	namespace vulkan {
		struct Graphic;
		
		struct UF_API RenderMode : public Buffers {
			bool execute = false;
			bool executed = false;
			bool rebuild = false;
			bool rerecord = false;
			bool resized = false;
			
			uint32_t width = 0;
			uint32_t height = 0;
			float scale = 0;

			ext::vulkan::Graphic blitter;

			struct {			
				uf::Serializer json;

				uf::stl::string name = "";
				uf::stl::string type = "";
				uf::stl::string target = "";
				uf::stl::string pipeline = "";
				uf::stl::vector<uf::stl::string> pipelines;
				
				uf::stl::unordered_map<uf::stl::string, uint8_t> attachments;
				uf::stl::unordered_map<uf::stl::string, uint8_t> buffers;

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

				uf::Camera camera;
			} metadata;
			
			Device* device = VK_NULL_HANDLE;
			RenderTarget renderTarget;

			uf::stl::vector<VkSemaphore> renderCompleteSemaphores;
			uf::stl::vector<VkFence> fences;
			uf::renderer::QueueEnum queueEnum = {};
			
			typedef uf::stl::vector<VkCommandBuffer> commands_container_t;
			uf::thread::id_t mostRecentCommandPoolId;
			uf::ThreadUnique<commands_container_t> commands;

			constexpr static int32_t CALLBACK_BEGIN = -1;
			constexpr static int32_t CALLBACK_END = -2;
			constexpr static int32_t EXECUTE_BEGIN = -3;
			constexpr static int32_t EXECUTE_END = -4;
			
			typedef std::function<void(VkCommandBuffer, size_t)> callback_t;
			uf::stl::unordered_map<int32_t, callback_t> commandBufferCallbacks;
			
			void bindCallback( int32_t, const callback_t& );
			
			uf::Image screenshot(size_t = 0, size_t = 0);

			commands_container_t& getCommands( uf::thread::id_t = uf::thread::current_id() );
			void lockMutex( uf::thread::id_t = uf::thread::current_id() );
			bool tryMutex( uf::thread::id_t = uf::thread::current_id() );
			void unlockMutex( uf::thread::id_t = uf::thread::current_id() );
			std::lock_guard<std::mutex> guardMutex( uf::thread::id_t = uf::thread::current_id() );
			
			void cleanupAllCommands();
			void cleanupCommands( uf::thread::id_t = uf::thread::current_id() );
			
			ext::vulkan::Graphic& getBlitter();
			virtual RenderTarget& getRenderTarget(size_t = 0);

			bool hasAttachment( const uf::stl::string& ) const;
			const RenderTarget::Attachment& getAttachment( const uf::stl::string& ) const;
			size_t getAttachmentIndex( const uf::stl::string& ) const;

			bool hasBuffer( const uf::stl::string& ) const;
			const Buffer& getBuffer( const uf::stl::string& ) const;
			size_t getBufferIndex( const uf::stl::string& ) const;

			const uf::stl::string getTarget() const;
			void setTarget( const uf::stl::string& );

			virtual ~RenderMode();
			// RAII
			virtual const uf::stl::string getName() const;
			virtual const uf::stl::string getType() const;
			virtual GraphicDescriptor bindGraphicDescriptor( const GraphicDescriptor&, size_t = 0 );
			
			virtual void initialize( Device& device );
			virtual void createCommandBuffers();
			virtual void createCommandBuffers( const uf::stl::vector<ext::vulkan::Graphic*>& graphics );
			virtual void bindPipelines();
			virtual void bindPipelines( const uf::stl::vector<ext::vulkan::Graphic*>& graphics );
			virtual void build( bool = true );
			virtual void tick();
			virtual void render();
			virtual void destroy();
			virtual void synchronize( uint64_t = UINT64_MAX );
			virtual void pipelineBarrier( VkCommandBuffer, uint8_t = -1 );

			virtual VkSubmitInfo queue();
		};
	}
}