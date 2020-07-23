#include <uf/ext/vulkan/vulkan.h>
#include <uf/ext/vulkan/rendermodes/base.h>
#include <uf/ext/vulkan/initializers.h>
#include <uf/utils/window/window.h>

#include <uf/ext/vulkan/rendertarget.h>
#include <uf/utils/mesh/mesh.h>

/*
ext::vulkan::BaseRenderMode::~BaseRenderMode() {
	this->destroy();
}
*/
std::string ext::vulkan::BaseRenderMode::getType() const {
	return "Base";
}
const std::string& ext::vulkan::BaseRenderMode::getName() const {
	return this->name;
}
size_t ext::vulkan::BaseRenderMode::subpasses() const {
	return 1;
}
VkRenderPass& ext::vulkan::BaseRenderMode::getRenderPass() {
	return swapchain.renderPass;
}

void ext::vulkan::BaseRenderMode::createCommandBuffers() {
	std::vector<ext::vulkan::Graphic*> graphics;
	std::function<void(uf::Entity*)> filter = [&]( uf::Entity* entity ) {
		if ( !entity->hasComponent<uf::Mesh>() ) return;
		uf::MeshBase& mesh = entity->getComponent<uf::Mesh>();
		if ( !mesh.generated ) return;
		ext::vulkan::Graphic& graphic = mesh.graphic;
		if ( !graphic.initialized ) return;
		if ( !graphic.renderMode ) return;
		graphics.push_back(&graphic);
	};
	for ( uf::Scene* scene : ext::vulkan::scenes ) {
		if ( !scene ) continue;
		scene->process(filter);
	}

	createCommandBuffers( graphics, ext::vulkan::passes );
}
void ext::vulkan::BaseRenderMode::createCommandBuffers( const std::vector<ext::vulkan::Graphic*>& graphics, const std::vector<std::string>& passes ) {
	// destroy if exists
	if ( swapchain.initialized ) {
		auto* device = swapchain.device;
		bool vsync = swapchain.vsync;
		swapchain.destroy();
		swapchain.initialize( *device );
	}

	float width = width > 0 ? this->width : ext::vulkan::width;
	float height = height > 0 ? this->height : ext::vulkan::height;

	VkCommandBufferBeginInfo commandBufferInfo = {};
	commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferInfo.pNext = nullptr;

	// Set clear values for all framebuffer attachments with loadOp set to clear
	// We use two attachments (color and depth) that are cleared at the start of the subpass and as such we need to set clear values for both
	VkClearValue clearValues[2];
	clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
	clearValues[1].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.pNext = nullptr;
	renderPassBeginInfo.renderPass = renderTarget.renderPass;
	renderPassBeginInfo.renderArea.offset.x = 0;
	renderPassBeginInfo.renderArea.offset.y = 0;
	renderPassBeginInfo.renderArea.extent.width = width;
	renderPassBeginInfo.renderArea.extent.height = width;
	renderPassBeginInfo.clearValueCount = 2;
	renderPassBeginInfo.pClearValues = clearValues;
	for (size_t i = 0; i < commands.size(); ++i) {
		// Set target frame buffer
		renderPassBeginInfo.framebuffer = renderTarget.framebuffers[i];

		VK_CHECK_RESULT(vkBeginCommandBuffer(commands[i], &commandBufferInfo));

		for ( auto graphic : graphics ) {
			graphic->createImageMemoryBarrier(commands[i]);
		}

		// Start the first sub pass specified in our default render pass setup by the base class
		// This will clear the color and depth attachment
		vkCmdBeginRenderPass(commands[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
			// Update dynamic viewport state
			VkViewport viewport = {};
			viewport.height = (float) height;
			viewport.width = (float) width;
			viewport.minDepth = (float) 0.0f;
			viewport.maxDepth = (float) 1.0f;
			vkCmdSetViewport(commands[i], 0, 1, &viewport);
			// Update dynamic scissor state
			VkRect2D scissor = {};
			scissor.extent.width = width;
			scissor.extent.height = height;
			scissor.offset.x = 0;
			scissor.offset.y = 0;
			vkCmdSetScissor(commands[i], 0, 1, &scissor);

			for ( auto pass : passes ) {
				ext::vulkan::currentPass = pass;
				for ( auto graphic : graphics ) {
					graphic->createCommandBuffer(commands[i] );
				}
			}
		vkCmdEndRenderPass(commands[i]);

		// Ending the render pass will add an implicit barrier transitioning the frame buffer color attachment to 
		// VK_IMAGE_LAYOUT_PRESENT_SRC_KHR for presenting it to the windowing system

		VK_CHECK_RESULT(vkEndCommandBuffer(commands[i]));
	}
}

void ext::vulkan::BaseRenderMode::render() {
	// Get next image in the swap chain (back/front buffer)
	VK_CHECK_RESULT(swapchain.acquireNextImage(&currentBuffer));

	// Use a fence to wait until the command buffer has finished execution before using it again
	VK_CHECK_RESULT(vkWaitForFences(*device, 1, &waitFences[currentBuffer], VK_TRUE, UINT64_MAX));
	VK_CHECK_RESULT(vkResetFences(*device, 1, &waitFences[currentBuffer]));

	// Pipeline stage at which the queue submission will wait (via pWaitSemaphores)
	VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	// The submit info structure specifices a command buffer queue submission batch
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pWaitDstStageMask = &waitStageMask;									// Pointer to the list of pipeline stages that the semaphore waits will occur at
	submitInfo.pWaitSemaphores = &presentCompleteSemaphore;				// Semaphore(s) to wait upon before the submitted command buffer starts executing
	submitInfo.waitSemaphoreCount = 1;												// One wait semaphore																				
	submitInfo.pSignalSemaphores = &renderCompleteSemaphore;				// Semaphore(s) to be signaled when command buffers have completed
	submitInfo.signalSemaphoreCount = 1;											// One signal semaphore
	submitInfo.pCommandBuffers = &commands[currentBuffer];		// Command buffers(s) to execute in this batch (submission)
	submitInfo.commandBufferCount = 1;

	// Submit to the graphics queue passing a wait fence
	VK_CHECK_RESULT(vkQueueSubmit(device->graphicsQueue, 1, &submitInfo, waitFences[currentBuffer]));
	
	// Present the current buffer to the swap chain
	// Pass the semaphore signaled by the command buffer submission from the submit info as the wait semaphore for swap chain presentation
	// This ensures that the image is not presented to the windowing system until all commands have been submitted
	VK_CHECK_RESULT(swapchain.queuePresent(device->presentQueue, currentBuffer, renderCompleteSemaphore));
	VK_CHECK_RESULT(vkQueueWaitIdle(device->presentQueue));
}

void ext::vulkan::BaseRenderMode::initialize( Device& device ) {
	this->device = &device;

	this->width = ext::vulkan::width;
	this->height = ext::vulkan::height;

	{
		renderTarget.device = &device;
		// attach targets
		struct {
			size_t color, depth;
		} attachments;

		attachments.color = renderTarget.attach( device.formats.color, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR ); // color
		attachments.depth = renderTarget.attach( device.formats.depth, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT ); // depth

		// First pass: fill the G-Buffer
		{
			renderTarget.addPass(
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				{ attachments.color },
				{},
				attachments.depth
			);
		}
	}
	renderTarget.initialize( device );

	// Create command buffers
	{
		commands.resize( buffers );

		VkCommandBufferAllocateInfo cmdBufAllocateInfo = ext::vulkan::initializers::commandBufferAllocateInfo(
			device.commandPool,
			VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			static_cast<uint32_t>(commands.size())
		);

		VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, commands.data()));
	}
	// Set sync objects
	{
		// Semaphores (Used for correct command ordering)
		VkSemaphoreCreateInfo semaphoreCreateInfo = {};
		semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		semaphoreCreateInfo.pNext = nullptr;

		// Semaphore used to ensures that image presentation is complete before starting to submit again
		VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &presentCompleteSemaphore));

		// Semaphore used to ensures that all commands submitted have been finished before submitting the image to the queue
		VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &renderCompleteSemaphore));

		// Fences (Used to check draw command buffer completion)
		VkFenceCreateInfo fenceCreateInfo = {};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		// Create in signaled state so we don't wait on first render of each command buffer
		fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		waitFences.resize( buffers );
		for ( auto& fence : waitFences ) {
			VK_CHECK_RESULT(vkCreateFence(device, &fenceCreateInfo, nullptr, &fence));
		}

		VkPipelineStageFlags submitPipelineStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	}
}

void ext::vulkan::BaseRenderMode::destroy() {
	if ( commands.size() > 0 ) {
		vkFreeCommandBuffers( *device, device->commandPool, static_cast<uint32_t>(commands.size()), commands.data());
	}
	if ( presentCompleteSemaphore != VK_NULL_HANDLE ) {
		vkDestroySemaphore( *device, presentCompleteSemaphore, nullptr);
	}
	if ( renderCompleteSemaphore != VK_NULL_HANDLE ) {
		vkDestroySemaphore( *device, renderCompleteSemaphore, nullptr);
	}
}