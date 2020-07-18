#include <uf/ext/vulkan/vulkan.h>
#include <uf/ext/vulkan/commands/base.h>
#include <uf/ext/vulkan/initializers.h>
#include <uf/utils/window/window.h>

#include <uf/ext/vulkan/rendertarget.h>

const std::string& ext::vulkan::Command::getName() const {
	return "Base";
}
size_t ext::vulkan::Command::subpasses() const {
	return 1;
}
VkRenderPass& ext::vulkan::Command::getRenderPass() {
	return swapchain.renderPass;
}

void ext::vulkan::Command::createCommandBuffers() {
	std::vector<void*> graphics;
	if ( ext::vulkan::graphics ) {
		graphics.reserve( ext::vulkan::graphics->size() );
		for ( Graphic* graphic : *ext::vulkan::graphics ) {
			if ( !graphic || !graphic->process ) continue;
			graphics.push_back( (void*) graphic);
		}
	}
	createCommandBuffers( graphics, ext::vulkan::passes );
}
void ext::vulkan::Command::createCommandBuffers( const std::vector<void*>& graphics, const std::vector<std::string>& passes ) {
	// destroy if exists
	if ( swapchain.commandBufferSet ) {
		auto* device = swapchain.device;
		bool vsync = swapchain.vsync;
		swapchain.destroy();
		swapchain.initialize( *device, 0, 0, vsync );
	}

	float width = width > 0 ? this->width : ext::vulkan::width;
	float height = height > 0 ? this->height : ext::vulkan::height;

	swapchain.commandBufferSet = true;

	VkCommandBufferBeginInfo cmdBufInfo = {};
	cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBufInfo.pNext = nullptr;

	// Set clear values for all framebuffer attachments with loadOp set to clear
	// We use two attachments (color and depth) that are cleared at the start of the subpass and as such we need to set clear values for both
	VkClearValue clearValues[2];
	clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
	clearValues[1].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.pNext = nullptr;
	renderPassBeginInfo.renderPass = swapchain.renderPass;
	renderPassBeginInfo.renderArea.offset.x = 0;
	renderPassBeginInfo.renderArea.offset.y = 0;
	renderPassBeginInfo.renderArea.extent.width = width;
	renderPassBeginInfo.renderArea.extent.height = width;
	renderPassBeginInfo.clearValueCount = 2;
	renderPassBeginInfo.pClearValues = clearValues;
	for (int32_t i = 0; i < swapchain.drawCommandBuffers.size(); ++i) {
		// Set target frame buffer
		renderPassBeginInfo.framebuffer = swapchain.frameBuffers[i];

		VK_CHECK_RESULT(vkBeginCommandBuffer(swapchain.drawCommandBuffers[i], &cmdBufInfo));

		for ( auto& pGraphic : graphics ) {
			Graphic& graphic = *((Graphic*) pGraphic);
			graphic.createImageMemoryBarrier(swapchain.drawCommandBuffers[i]);
		}

		// Start the first sub pass specified in our default render pass setup by the base class
		// This will clear the color and depth attachment
		vkCmdBeginRenderPass(swapchain.drawCommandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
			// Update dynamic viewport state
			VkViewport viewport = {};
			viewport.height = (float) height;
			viewport.width = (float) width;
			viewport.minDepth = (float) 0.0f;
			viewport.maxDepth = (float) 1.0f;
			vkCmdSetViewport(swapchain.drawCommandBuffers[i], 0, 1, &viewport);
			// Update dynamic scissor state
			VkRect2D scissor = {};
			scissor.extent.width = width;
			scissor.extent.height = height;
			scissor.offset.x = 0;
			scissor.offset.y = 0;
			vkCmdSetScissor(swapchain.drawCommandBuffers[i], 0, 1, &scissor);

			for ( auto pass : passes ) {
				ext::vulkan::currentPass = pass;
				for ( auto& pGraphic : graphics ) {
					Graphic& graphic = *((Graphic*) pGraphic);
					graphic.createCommandBuffer(swapchain.drawCommandBuffers[i] );
				}
			}
		vkCmdEndRenderPass(swapchain.drawCommandBuffers[i]);

		// Ending the render pass will add an implicit barrier transitioning the frame buffer color attachment to 
		// VK_IMAGE_LAYOUT_PRESENT_SRC_KHR for presenting it to the windowing system

		VK_CHECK_RESULT(vkEndCommandBuffer(swapchain.drawCommandBuffers[i]));
	}
}

void ext::vulkan::Command::render() {
	// Get next image in the swap chain (back/front buffer)
	VK_CHECK_RESULT(swapchain.acquireNextImage(&currentBuffer));

	// Use a fence to wait until the command buffer has finished execution before using it again
	VK_CHECK_RESULT(vkWaitForFences(device, 1, &swapchain.waitFences[currentBuffer], VK_TRUE, UINT64_MAX));
	VK_CHECK_RESULT(vkResetFences(device, 1, &swapchain.waitFences[currentBuffer]));

	// Pipeline stage at which the queue submission will wait (via pWaitSemaphores)
	VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	// The submit info structure specifices a command buffer queue submission batch
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pWaitDstStageMask = &waitStageMask;									// Pointer to the list of pipeline stages that the semaphore waits will occur at
	submitInfo.pWaitSemaphores = &swapchain.presentCompleteSemaphore;				// Semaphore(s) to wait upon before the submitted command buffer starts executing
	submitInfo.waitSemaphoreCount = 1;												// One wait semaphore																				
	submitInfo.pSignalSemaphores = &swapchain.renderCompleteSemaphore;				// Semaphore(s) to be signaled when command buffers have completed
	submitInfo.signalSemaphoreCount = 1;											// One signal semaphore
	submitInfo.pCommandBuffers = &swapchain.drawCommandBuffers[currentBuffer];		// Command buffers(s) to execute in this batch (submission)
	submitInfo.commandBufferCount = 1;

	// Submit to the graphics queue passing a wait fence
	VK_CHECK_RESULT(vkQueueSubmit(device.graphicsQueue, 1, &submitInfo, swapchain.waitFences[currentBuffer]));
	
	// Present the current buffer to the swap chain
	// Pass the semaphore signaled by the command buffer submission from the submit info as the wait semaphore for swap chain presentation
	// This ensures that the image is not presented to the windowing system until all commands have been submitted
	VK_CHECK_RESULT(swapchain.queuePresent(device.presentQueue, currentBuffer, swapchain.renderCompleteSemaphore));
	VK_CHECK_RESULT(vkQueueWaitIdle(device.presentQueue));
}

void ext::vulkan::Command::initialize( Device& device ) {
	this->width = 0;
	this->height = 0;
}

void ext::vulkan::Command::destroy() {

}