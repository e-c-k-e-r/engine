#pragma once

#include <uf/ext/opengl/device.h>
#include <uf/ext/opengl/commands.h>
#include <uf/utils/mesh/mesh.h>

namespace ext {
	namespace opengl {
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
				uf::stl::vector<uint8_t> outputs;

				uint8_t subpasses = 1;
				uint8_t samples = 1;
				uint8_t eyes = 1;

				size_t lightBufferIndex = 0;
				size_t materialBufferIndex = 0;
				size_t textureBufferIndex = 0;
				size_t drawCallBufferIndex = 0;
			} metadata;
			
			Device* device = GL_NULL_HANDLE;
			RenderTarget renderTarget;

			GLhandle(VkSemaphore) renderCompleteSemaphore;
			uf::stl::vector<GLhandle(VkFence)> fences;
			
			std::thread::id mostRecentCommandPoolId;
			uf::ThreadUnique<CommandBuffer> commands;

			virtual ~RenderMode();
			// RAII
			virtual const uf::stl::string getName() const;
			virtual const uf::stl::string getType() const;
			virtual RenderTarget& getRenderTarget(size_t = 0);
			virtual const RenderTarget& getRenderTarget(size_t = 0) const;

			virtual const size_t blitters() const;
			virtual ext::opengl::Graphic* getBlitter(size_t = 0);
			virtual uf::stl::vector<ext::opengl::Graphic*> getBlitters();

			virtual CommandBuffer& getCommands();
			virtual CommandBuffer& getCommands( std::thread::id );

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