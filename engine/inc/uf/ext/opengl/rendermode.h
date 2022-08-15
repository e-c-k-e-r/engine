#pragma once

#include <uf/ext/opengl/device.h>
#include <uf/ext/opengl/commands.h>
#include <uf/ext/opengl/graphic.h>
#include <uf/utils/mesh/mesh.h>

namespace ext {
	namespace opengl {		
		struct UF_API RenderMode {
			bool execute = false;
			bool executed = false;
			bool rebuild = false;
			bool resized = false;
			
			uint32_t width = 0;
			uint32_t height = 0;
			float scale = 0;

			ext::opengl::Graphic blitter;

			struct {			
				uf::Serializer json;

				uf::stl::string name = "";
				uf::stl::string type = "";
				uf::stl::string target = "";
				uf::stl::string pipeline = "";
				uf::stl::vector<uf::stl::string> pipelines;
			//	uf::stl::vector<uint8_t> outputs;
				
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

			} metadata;
			
			Device* device = GL_NULL_HANDLE;
			RenderTarget renderTarget;

			GLhandle(VkSemaphore) renderCompleteSemaphore;
			uf::stl::vector<GLhandle(VkFence)> fences;
			
			std::thread::id mostRecentCommandPoolId;
			uf::ThreadUnique<CommandBuffer> commands;

			virtual ~RenderMode();

			ext::opengl::Graphic& getBlitter();
			RenderTarget& getRenderTarget();

			bool hasAttachment( const uf::stl::string& ) const;
			const RenderTarget::Attachment& getAttachment( const uf::stl::string& ) const;
			size_t getAttachmentIndex( const uf::stl::string& ) const;

			bool hasBuffer( const uf::stl::string& ) const;
			const Buffer& getBuffer( const uf::stl::string& ) const;
			size_t getBufferIndex( const uf::stl::string& ) const;

			const uf::stl::string getTarget() const;
			void setTarget( const uf::stl::string& );

			virtual const uf::stl::string getName() const;
			virtual const uf::stl::string getType() const;

			virtual CommandBuffer& getCommands( std::thread::id = std::this_thread::get_id() );

			virtual GraphicDescriptor bindGraphicDescriptor( const GraphicDescriptor&, size_t = 0 );
			virtual void bindGraphicPushConstants( ext::opengl::Graphic*, size_t = 0 );
			
			virtual void initialize( Device& device );
			virtual void createCommandBuffers();
			virtual void createCommandBuffers( const uf::stl::vector<ext::opengl::Graphic*>& graphics );
			virtual void bindPipelines();
			virtual void bindPipelines( const uf::stl::vector<ext::opengl::Graphic*>& graphics );
			virtual void tick();
			virtual void render();
			virtual void destroy();
			virtual void synchronize( uint64_t = UINT64_MAX );
			virtual void pipelineBarrier( GLhandle(VkCommandBuffer), uint8_t = -1 );
		};
	}
}