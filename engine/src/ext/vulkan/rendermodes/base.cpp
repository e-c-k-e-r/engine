#if UF_USE_VULKAN

#include <uf/ext/vulkan/vulkan.h>
#include <uf/ext/vulkan/rendermodes/base.h>
#include <uf/ext/vulkan/initializers.h>
#include <uf/utils/window/window.h>
#include <uf/ext/vulkan/graphic.h>
#include <uf/ext/vulkan/rendertarget.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/utils/io/fmt.h>

const uf::stl::string ext::vulkan::BaseRenderMode::getType() const {
	return "Swapchain";
}


void ext::vulkan::BaseRenderMode::build( bool resized ) {
	
}

void ext::vulkan::BaseRenderMode::initialize( Device& device ) {
	this->metadata.name = "Swapchain";
	auto windowSize = device.window->getSize();
	this->width = windowSize.x;
	this->height = windowSize.y;
	renderTarget.width = this->width;
	renderTarget.height = this->height;

	ext::vulkan::RenderMode::initialize( device );
	swapchain.initialize( device );
	renderTarget.device = &device;
	renderTarget.passes.clear();
	renderTarget.attachments.clear();

	struct {
		size_t color, depth;
	} attachments = {};

	attachments.color = renderTarget.attach(RenderTarget::Attachment::Descriptor{
		/*.format = */ext::vulkan::settings::formats::color,
		/*.layout = */VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		/*.usage = */VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		/*.blend = */false,
		/*.samples = */1,
	});

	attachments.depth = renderTarget.attach(RenderTarget::Attachment::Descriptor{
		/*.format = */ext::vulkan::settings::formats::depth,
		/*.layout = */VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		/*.usage = */VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		/*.blend = */false,
		/*.samples = */1,
	});

	metadata.attachments["color"] = attachments.color;
	metadata.attachments["depth"] = attachments.depth;

	renderTarget.addPass(
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		{ attachments.color }, {}, {}, attachments.depth, 0, true
	);

	renderTarget.initialize( device );

	// set sync objects
	for ( auto i = 0; i < ext::vulkan::swapchain.buffers; ++i ) {
		auto& presentCompleteSemaphore = swapchain.presentCompleteSemaphores.emplace_back();
		VkSemaphoreCreateInfo semaphoreCreateInfo = {};
		semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		semaphoreCreateInfo.pNext = nullptr;

		VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &presentCompleteSemaphore));
		VK_REGISTER_HANDLE(presentCompleteSemaphore);
	}
}

void ext::vulkan::BaseRenderMode::tick() {
	ext::vulkan::RenderMode::tick();

	bool resized = (this->width == 0 && this->height == 0) || ext::vulkan::states::resized || this->resized;
	bool rebuild = resized || ext::vulkan::states::rebuild || this->rebuild;
	
	auto windowSize = device->window->getSize();

	if ( windowSize.x == this->width && windowSize.y == this->height ) {
		resized = false;
	}

	// rebuild rendertarget
	if ( resized ) {
		this->destroy();
		this->initialize( *this->device );
	/*
		this->width = windowSize.x;
		this->height = windowSize.y;
		this->resized = false;
		rebuild = true;
		renderTarget.width = this->width;
		renderTarget.height = this->height;


		swapchain.initialize( *swapchain.device );
		renderTarget.initialize( *renderTarget.device );
	*/
	}

	// update blitter descriptor set
	if ( rebuild && blitter.initialized ) {
		this->build( resized );
	}
}
void ext::vulkan::BaseRenderMode::render() {
//	if ( ext::vulkan::states::frameSkip ) return;
//	if ( ext::vulkan::renderModes.size() > 1 ) return;
//	if ( ext::vulkan::renderModes.back() != this ) return;

	if ( this->commands.container().empty() ) return;

	//lockMutex( this->mostRecentCommandPoolId );
	auto& commands = getCommands( this->mostRecentCommandPoolId );
	uint32_t imageIndex;
	
	VK_CHECK_RESULT(vkWaitForFences(*device, 1, &fences[states::currentBuffer], VK_TRUE, VK_DEFAULT_FENCE_TIMEOUT));
	VK_CHECK_RESULT(swapchain.acquireNextImage(&imageIndex, swapchain.presentCompleteSemaphores[states::currentBuffer]));
	VK_CHECK_RESULT(vkResetFences(*device, 1, &fences[states::currentBuffer]));

	VkPipelineStageFlags waitStageMask[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pWaitDstStageMask = waitStageMask;
	submitInfo.pWaitSemaphores = &swapchain.presentCompleteSemaphores[states::currentBuffer];
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &renderCompleteSemaphores[states::currentBuffer];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pCommandBuffers = &commands[states::currentBuffer];
	submitInfo.commandBufferCount = 1;

	{
		VkQueue queue = device->getQueue( QueueEnum::GRAPHICS );
		VkResult res = vkQueueSubmit( queue, 1, &submitInfo, fences[states::currentBuffer]);
		VK_CHECK_QUEUE_CHECKPOINT( queue, res );
	}
	
	VK_CHECK_RESULT(swapchain.queuePresent(device->getQueue( QueueEnum::PRESENT ), imageIndex, renderCompleteSemaphores[states::currentBuffer]));
#if 0
	{
		VkQueue queue = device->getQueue( QueueEnum::PRESENT );
		VkResult res = vkQueueWaitIdle(device->getQueue( QueueEnum::PRESENT ));
		VK_CHECK_QUEUE_CHECKPOINT( queue, res );
	}
#endif

	states::currentBuffer = (states::currentBuffer + 1) % ext::vulkan::swapchain.buffers;
	this->executed = true;

	//unlockMutex( this->mostRecentCommandPoolId );
}

void ext::vulkan::BaseRenderMode::destroy() {

	ext::vulkan::RenderMode::destroy();

	for ( auto& presentCompleteSemaphore : swapchain.presentCompleteSemaphores ) {
		vkDestroySemaphore( *device, presentCompleteSemaphore, nullptr);
		VK_UNREGISTER_HANDLE( presentCompleteSemaphore );
	}
	swapchain.presentCompleteSemaphores.clear();
}

ext::vulkan::GraphicDescriptor ext::vulkan::BaseRenderMode::bindGraphicDescriptor( const ext::vulkan::GraphicDescriptor& reference, size_t pass ) {
	ext::vulkan::GraphicDescriptor descriptor = ext::vulkan::RenderMode::bindGraphicDescriptor(reference, pass);

	descriptor.depth.test = false;
	descriptor.depth.write = false;
	return descriptor;
}

void ext::vulkan::BaseRenderMode::createCommandBuffers( const uf::stl::vector<ext::vulkan::Graphic*>& graphics ) {
//	if ( ext::vulkan::renderModes.size() > 1 ) return;

	auto windowSize = device->window->getSize();
	float width = windowSize.x; //this->width > 0 ? this->width : windowSize.x;
	float height = windowSize.y; //this->height > 0 ? this->height : windowSize.y;

	VkCommandBufferBeginInfo cmdBufInfo = ext::vulkan::initializers::commandBufferBeginInfo();
	cmdBufInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	VkImageMemoryBarrier imageMemoryBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // ext::vulkan::device.queueFamilyIndices.graphics; //VK_QUEUE_FAMILY_IGNORED
	imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // ext::vulkan::device.queueFamilyIndices.graphics; //VK_QUEUE_FAMILY_IGNORED
	imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
	imageMemoryBarrier.subresourceRange.levelCount = 1;
	imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
	imageMemoryBarrier.subresourceRange.layerCount = 1;
	imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

	uf::stl::vector<RenderMode*> layers = ext::vulkan::getRenderModes(uf::stl::vector<uf::stl::string>{"Deferred", "Compute:RT", "RenderTarget"}, false);
#if 0
	if ( settings::pipelines::rt ) {
		std::reverse( layers.begin(), layers.end() );
	}
#endif

	auto& scene = uf::scene::getCurrentScene();
	auto& sceneMetadataJson = scene.getComponent<uf::Serializer>();
	auto& commands = getCommands();

	uf::stl::vector<VkClearValue> clearValues;
	for ( auto& attachment : renderTarget.attachments ) {
		pod::Vector4f clearColor = uf::vector::decode( sceneMetadataJson["system"]["renderer"]["clear values"][(int) clearValues.size()], pod::Vector4f{0, 0, 0, 0} );
		auto& clearValue = clearValues.emplace_back();
		if ( attachment.descriptor.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT ) {
			clearValue.color.float32[0] = clearColor[0];
			clearValue.color.float32[1] = clearColor[1];
			clearValue.color.float32[2] = clearColor[2];
			clearValue.color.float32[3] = clearColor[3];
		} else if ( attachment.descriptor.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT ) {
			if ( uf::matrix::reverseInfiniteProjection ) {
				clearValue.depthStencil = { 0.0f, 0 };
			} else {
				clearValue.depthStencil = { 1.0f, 0 };
			}
		}
	}

	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.pNext = nullptr;
	renderPassBeginInfo.renderPass = renderTarget.renderPass;
	renderPassBeginInfo.renderArea.offset.x = 0;
	renderPassBeginInfo.renderArea.offset.y = 0;
	renderPassBeginInfo.renderArea.extent.width = width;
	renderPassBeginInfo.renderArea.extent.height = height;
	renderPassBeginInfo.clearValueCount = clearValues.size();
	renderPassBeginInfo.pClearValues = &clearValues[0];

	// Update dynamic viewport state
	VkViewport viewport = {};
	viewport.width = (float) width;
	viewport.height = (float) height;
	viewport.minDepth = (float) 0.0f;
	viewport.maxDepth = (float) 1.0f;
	
	// Update dynamic scissor state
	VkRect2D scissor = {};
	scissor.extent.width = width;
	scissor.extent.height = height;
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	
	for (size_t frame = 0; frame < commands.size(); ++frame) {
		
		auto& commandBuffer = commands[frame];
		renderPassBeginInfo.framebuffer = renderTarget.framebuffers[frame];

		VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffer, &cmdBufInfo));
		device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::BEGIN, "begin" );
		{
			size_t currentSubpass = 0;
			
			// pre-renderpass commands
			VK_COMMAND_BUFFER_CALLBACK( CALLBACK_BEGIN, commandBuffer, frame, {
				device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::GENERIC, "callback[begin]" );
			} );

			device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::BEGIN, "renderPass[begin]" );
			vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
				vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
				vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
				// render to geometry buffers
				for ( size_t eye = 0; eye < metadata.eyes; ++eye ) {
					size_t currentPass = 0;
					size_t currentDraw = 0;

					// blit any RT's that request this subpass
					for ( auto _ : layers ) {
						RenderTargetRenderMode* layer = (RenderTargetRenderMode*) _;
						auto& blitter = layer->blitter;
						if ( !blitter.initialized || !blitter.process || blitter.descriptor.subpass != currentPass || blitter.descriptor.renderMode != this->getName() ) continue;
						ext::vulkan::GraphicDescriptor descriptor = blitter.descriptor; // bindGraphicDescriptor(blitter.descriptor, currentSubpass);
						device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::GENERIC, ::fmt::format("blitter[{}: {}]", layer->getName(), layer->getType()) );
						blitter.record(commandBuffer, descriptor, 0, 0, frame);
					}
				}
			device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::END, "renderPass[end]" );
			vkCmdEndRenderPass(commandBuffer);

			// post-renderpass commands
			VK_COMMAND_BUFFER_CALLBACK( CALLBACK_END, commandBuffer, frame, {
				device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::GENERIC, "callback[end]" );
			} );
		}

		device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::END, "end" );
		VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));
	}
}

#endif