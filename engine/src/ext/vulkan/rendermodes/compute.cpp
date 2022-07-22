#if UF_USE_VULKAN

#include <uf/ext/vulkan/vulkan.h>
#include <uf/ext/vulkan/rendermodes/compute.h>
#include <uf/ext/vulkan/initializers.h>
#include <uf/utils/window/window.h>
#include <uf/utils/math/physics.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/ext/vulkan/graphic.h>
#include <uf/engine/graph/graph.h>
#include <uf/utils/camera/camera.h>

const uf::stl::string ext::vulkan::ComputeRenderMode::getTarget() const {
//	auto& metadata = *const_cast<uf::Serializer*>(&this->metadata);
//	return metadata["target"].as<uf::stl::string>();
	return metadata.target;
}
void ext::vulkan::ComputeRenderMode::setTarget( const uf::stl::string& target ) {
//	this->metadata["target"] = target;
	metadata.target = target;
}

const uf::stl::string ext::vulkan::ComputeRenderMode::getType() const {
	return "Compute";
}
const size_t ext::vulkan::ComputeRenderMode::blitters() const {
	return 1;
}
ext::vulkan::Graphic* ext::vulkan::ComputeRenderMode::getBlitter( size_t i ) {
	return &this->blitter;
}
uf::stl::vector<ext::vulkan::Graphic*> ext::vulkan::ComputeRenderMode::getBlitters() {
	return { &this->blitter };
}

ext::vulkan::GraphicDescriptor ext::vulkan::ComputeRenderMode::bindGraphicDescriptor( const ext::vulkan::GraphicDescriptor& reference, size_t pass ) {
	ext::vulkan::GraphicDescriptor descriptor = ext::vulkan::RenderMode::bindGraphicDescriptor(reference, pass);
/*
	descriptor.parse(metadata.json["descriptor"]);
	// invalidate
	if ( metadata.target != "" && descriptor.renderMode != this->getName() && descriptor.renderMode != metadata.target ) {
		descriptor.invalidated = true;
	} else {
		descriptor.renderMode = this->getName();
	}
*/
	return descriptor;
}

void ext::vulkan::ComputeRenderMode::initialize( Device& device ) {
	this->setTarget("Compute");
	ext::vulkan::RenderMode::initialize( device );
#if 0
	{
		uf::Mesh mesh;
		mesh.bind<pod::Vertex_3F>();
	/*
		mesh.insertVertices<pod::Vertex_3F>({
			{ {-0.5f, -0.5f, 0.0f } },
			{ { 0.5f, -0.5f, 0.0f } },
			{ { 0.0f,  0.5f, 0.0f } },
		});
	*/
		mesh.insertVertices<pod::Vertex_3F>({
			{-1.0f,-1.0f,-1.0f,},
			{-1.0f,-1.0f, 1.0f,},
			{-1.0f, 1.0f, 1.0f,},
			{1.0f, 1.0f,-1.0f,},
			{-1.0f,-1.0f,-1.0f,},
			{-1.0f, 1.0f,-1.0f,},
			{1.0f,-1.0f, 1.0f,},
			{-1.0f,-1.0f,-1.0f,},
			{1.0f,-1.0f,-1.0f,},
			{1.0f, 1.0f,-1.0f,},
			{1.0f,-1.0f,-1.0f,},
			{-1.0f,-1.0f,-1.0f,},
			{-1.0f,-1.0f,-1.0f,},
			{-1.0f, 1.0f, 1.0f,},
			{-1.0f, 1.0f,-1.0f,},
			{1.0f,-1.0f, 1.0f,},
			{-1.0f,-1.0f, 1.0f,},
			{-1.0f,-1.0f,-1.0f,},
			{-1.0f, 1.0f, 1.0f,},
			{-1.0f,-1.0f, 1.0f,},
			{1.0f,-1.0f, 1.0f,},
			{1.0f, 1.0f, 1.0f,},
			{1.0f,-1.0f,-1.0f,},
			{1.0f, 1.0f,-1.0f,},
			{1.0f,-1.0f,-1.0f,},
			{1.0f, 1.0f, 1.0f,},
			{1.0f,-1.0f, 1.0f,},
			{1.0f, 1.0f, 1.0f,},
			{1.0f, 1.0f,-1.0f,},
			{-1.0f, 1.0f,-1.0f,},
			{1.0f, 1.0f, 1.0f,},
			{-1.0f, 1.0f,-1.0f,},
			{-1.0f, 1.0f, 1.0f,},
			{1.0f, 1.0f, 1.0f,},
			{-1.0f, 1.0f, 1.0f,},
			{1.0f,-1.0f, 1.0f,},
		});
		mesh.updateDescriptor();

		blitter.initialize( this->getName() );
		blitter.initializeMesh( mesh );

		ext::vulkan::generateAccelerationStructures( blitter );
	}
#endif
}

void ext::vulkan::ComputeRenderMode::tick() {
	ext::vulkan::RenderMode::tick();

	bool resized = this->width == 0 && this->height == 0 && ext::vulkan::states::resized;
	bool rebuild = resized || ext::vulkan::states::rebuild || this->rebuild;
#if 0
	{
		auto& shader = blitter.material.getShader("ray:gen");
		auto& image = shader.textures.front();
		
		struct UniformDescriptor {
			struct Matrices {
				alignas(16) pod::Matrix4f view;
				alignas(16) pod::Matrix4f projection;
				alignas(16) pod::Matrix4f iView;
				alignas(16) pod::Matrix4f iProjection;
				alignas(16) pod::Matrix4f iProjectionView;
				alignas(16) pod::Vector4f eyePos;
			} matrices[2];
		} uniforms;

		auto& scene = uf::scene::getCurrentScene();
		auto& controller = scene.getController();
		auto& camera = controller.getComponent<uf::Camera>();

		for ( auto i = 0; i < 2; ++i ) {
			uniforms.matrices[i] = UniformDescriptor::Matrices{
				.view = camera.getView(i),
				.projection = camera.getProjection(i),
				.iView = uf::matrix::inverse( camera.getView(i) ),
				.iProjection = uf::matrix::inverse( camera.getProjection(i) ),
				.iProjectionView = uf::matrix::inverse( camera.getProjection(i) * camera.getView(i) ),
				.eyePos = camera.getEye( i ),
			};
		}

		for ( auto& buffer : shader.buffers ) {
			if ( !(buffer.usage & uf::renderer::enums::Buffer::UNIFORM) ) continue;
			if ( buffer.allocationInfo.size != sizeof(UniformDescriptor) ) continue;
			
			buffer.update( (const void*) &uniforms, sizeof(UniformDescriptor) );
			break;
		}
		
		if ( resized ) {
			image.destroy();
			image.fromBuffers( NULL, 0, uf::renderer::enums::Format::R8G8B8A8_UNORM, uf::renderer::settings::width, uf::renderer::settings::height, 1, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_LAYOUT_GENERAL );

			blitter.descriptor.inputs.width = image.width;
			blitter.descriptor.inputs.height = image.height;
			
			blitter.getPipeline().update( blitter );
		}
	}
#endif

	if ( metadata.limiter.frequency > 0 ) {
		if ( metadata.limiter.timer > metadata.limiter.frequency ) {
			metadata.limiter.timer = 0;
			metadata.limiter.execute = true;
		} else {
			metadata.limiter.timer = metadata.limiter.timer + uf::physics::time::delta;
			metadata.limiter.execute = false;
		}
	}
}
void ext::vulkan::ComputeRenderMode::destroy() {
#if 0
	auto& shader = blitter.material.getShader("ray:gen");
	auto& image = shader.textures.front();

	image.screenshot().save("./data/rt.png");
	image.destroy();

	blitter.destroy();
#endif

	ext::vulkan::RenderMode::destroy();
}

void ext::vulkan::ComputeRenderMode::render() {	
	if ( commandBufferCallbacks.count(EXECUTE_BEGIN) > 0 ) commandBufferCallbacks[EXECUTE_BEGIN]( VkCommandBuffer{}, 0 );

	//lockMutex( this->mostRecentCommandPoolId );
	auto& commands = getCommands( this->mostRecentCommandPoolId );

	// Submit commands
	// Use a fence to ensure that command buffer has finished executing before using it again
	VK_CHECK_RESULT(vkWaitForFences( *device, 1, &fences[states::currentBuffer], VK_TRUE, VK_DEFAULT_FENCE_TIMEOUT ));
	VK_CHECK_RESULT(vkResetFences( *device, 1, &fences[states::currentBuffer] ));

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pWaitDstStageMask = NULL; 								// Pointer to the list of pipeline stages that the semaphore waits will occur at
	submitInfo.pWaitSemaphores = NULL;									// Semaphore(s) to wait upon before the submitted command buffer starts executing
	submitInfo.waitSemaphoreCount = 0;									// One wait semaphore																				
	submitInfo.pSignalSemaphores = NULL;								// Semaphore(s) to be signaled when command buffers have completed
	submitInfo.signalSemaphoreCount = 0;								// One signal semaphore
	submitInfo.pCommandBuffers = &commands[states::currentBuffer];		// Command buffers(s) to execute in this batch (submission)
	submitInfo.commandBufferCount = 1;

	VK_CHECK_RESULT(vkQueueSubmit(device->getQueue( uf::renderer::QueueEnum::COMPUTE ), 1, &submitInfo, fences[states::currentBuffer]));

	if ( commandBufferCallbacks.count(EXECUTE_END) > 0 ) commandBufferCallbacks[EXECUTE_END]( VkCommandBuffer{}, 0 );

	this->executed = true;
	//unlockMutex( this->mostRecentCommandPoolId );
}
void ext::vulkan::ComputeRenderMode::pipelineBarrier( VkCommandBuffer commandBuffer, uint8_t state ) {
}
void ext::vulkan::ComputeRenderMode::createCommandBuffers( const uf::stl::vector<ext::vulkan::Graphic*>& graphics ) {
	// destroy if exists
	float width = this->width > 0 ? this->width : ext::vulkan::settings::width;
	float height = this->height > 0 ? this->height : ext::vulkan::settings::height;

	VkCommandBufferBeginInfo cmdBufInfo = {};
	cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBufInfo.pNext = nullptr;

	size_t subpasses = 1;
	auto& commands = getCommands();
	for (size_t i = 0; i < commands.size(); ++i) {
		VK_CHECK_RESULT(vkBeginCommandBuffer(commands[i], &cmdBufInfo));
		
		// pre-renderpass commands
		if ( commandBufferCallbacks.count(CALLBACK_BEGIN) > 0 ) commandBufferCallbacks[CALLBACK_BEGIN]( commands[i], i );

	//	if ( blitter.process ) blitter.getPipeline().record( blitter, blitter.descriptor, commands[i] );
	/*
		for ( ; currentPass < subpasses; ++currentPass ) {
			size_t currentDraw = 0;
			for ( auto graphic : graphics ) {
				if ( graphic->descriptor.renderMode != this->getTarget() ) continue;
				ext::vulkan::GraphicDescriptor descriptor = bindGraphicDescriptor(graphic->descriptor, currentPass);
				graphic->record( commands[i], descriptor, currentPass, currentDraw++ );
			}
			if ( commandBufferCallbacks.count( currentPass ) > 0 ) commandBufferCallbacks[currentPass]( commands[i], i );
			if ( currentPass + 1 < subpasses ) vkCmdNextSubpass(commands[i], VK_SUBPASS_CONTENTS_INLINE);
		}
	*/
		
		// post-renderpass commands
		if ( commandBufferCallbacks.count(CALLBACK_END) > 0 ) commandBufferCallbacks[CALLBACK_END]( commands[i], i );

		VK_CHECK_RESULT(vkEndCommandBuffer(commands[i]));
	}
}

#endif