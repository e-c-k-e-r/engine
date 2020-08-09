#include <uf/ext/vulkan/vulkan.h>
#include <uf/ext/vulkan/rendermodes/rendertarget.h>
#include <uf/ext/vulkan/initializers.h>
#include <uf/utils/window/window.h>
#include <uf/utils/mesh/mesh.h>

std::string ext::vulkan::RenderTargetRenderMode::getType() const {
	return "RenderTarget";
}

void ext::vulkan::RenderTargetRenderMode::initialize( Device& device ) {
	ext::vulkan::RenderMode::initialize( device );
	{
		renderTarget.device = &device;
		// attach targets
		struct {
			size_t color, depth;
		} attachments;

		attachments.color = renderTarget.attach( VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL ); // albedo
		attachments.depth = renderTarget.attach( device.formats.depth, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ); // depth

		// First pass: write to target
		{
			renderTarget.addPass(
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				{ attachments.color },
				{},
				attachments.depth
			);
		}
	}
	renderTarget.initialize( device );

	blitter.renderTarget = &renderTarget;
	blitter.initializeShaders({
		{"./data/shaders/display.rendertarget.vert.spv", VK_SHADER_STAGE_VERTEX_BIT},
		{"./data/shaders/display.rendertarget.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT}
	});
	// blitter.initialize();
}
void ext::vulkan::RenderTargetRenderMode::tick() {
	ext::vulkan::RenderMode::tick();
	if ( ext::vulkan::resized ) {
		renderTarget.initialize( *renderTarget.device );
	}
}
void ext::vulkan::RenderTargetRenderMode::destroy() {
	ext::vulkan::RenderMode::destroy();
	blitter.destroy();
}
void ext::vulkan::RenderTargetRenderMode::render() {
	// Submit commands
	// Use a fence to ensure that command buffer has finished executing before using it again
	VK_CHECK_RESULT(vkWaitForFences( *device, 1, &fences[currentBuffer], VK_TRUE, UINT64_MAX ));
	VK_CHECK_RESULT(vkResetFences( *device, 1, &fences[currentBuffer] ));

	// Pipeline stage at which the queue submission will wait (via pWaitSemaphores)
	VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	// The submit info structure specifices a command buffer queue submission batch
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pWaitDstStageMask = &waitStageMask;									// Pointer to the list of pipeline stages that the semaphore waits will occur at
/*
	submitInfo.pWaitSemaphores = NULL;				// Semaphore(s) to wait upon before the submitted command buffer starts executing
	submitInfo.waitSemaphoreCount = 0;												// One wait semaphore																				
	submitInfo.pSignalSemaphores = &renderCompleteSemaphore;				// Semaphore(s) to be signaled when command buffers have completed
	submitInfo.signalSemaphoreCount = 1;											// One signal semaphore
*/
	submitInfo.pCommandBuffers = &commands[currentBuffer];		// Command buffers(s) to execute in this batch (submission)
	submitInfo.commandBufferCount = 1;

	VK_CHECK_RESULT(vkQueueSubmit(device->graphicsQueue, 1, &submitInfo, fences[currentBuffer]));
/*
	VkSemaphoreWaitInfo waitInfo = {};
	waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
	waitInfo.flags = VK_SEMAPHORE_WAIT_ANY_BIT;
	waitInfo.semaphoreCount = 1;
	waitInfo.pSemaphores = &renderCompleteSemaphore;
	waitInfo.pValues = NULL;
	VK_CHECK_RESULT(vkWaitSemaphores( *device, &waitInfo, UINT64_MAX ));
*/
}
void ext::vulkan::RenderTargetRenderMode::createCommandBuffers( const std::vector<ext::vulkan::Graphic*>& graphics, const std::vector<std::string>& passes ) {
	// destroy if exists
	float width = this->width > 0 ? this->width : ext::vulkan::width;
	float height = this->height > 0 ? this->height : ext::vulkan::height;

	VkCommandBufferBeginInfo cmdBufInfo = {};
	cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBufInfo.pNext = nullptr;

	for (size_t i = 0; i < commands.size(); ++i) {
		VK_CHECK_RESULT(vkBeginCommandBuffer(commands[i], &cmdBufInfo));
		{
			std::vector<VkClearValue> clearValues; clearValues.resize(2);
			clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
			clearValues[1].depthStencil = { 0.0f, 0 };

			VkRenderPassBeginInfo renderPassBeginInfo = {};
			renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassBeginInfo.pNext = nullptr;
			renderPassBeginInfo.renderArea.offset.x = 0;
			renderPassBeginInfo.renderArea.offset.y = 0;
			renderPassBeginInfo.renderArea.extent.width = width;
			renderPassBeginInfo.renderArea.extent.height = height;
			renderPassBeginInfo.clearValueCount = clearValues.size();
			renderPassBeginInfo.pClearValues = &clearValues[0];
			renderPassBeginInfo.renderPass = renderTarget.renderPass;
			renderPassBeginInfo.framebuffer = renderTarget.framebuffers[i];

			for ( auto graphic : graphics ) {
				graphic->createImageMemoryBarrier(commands[i]);
			}

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

			vkCmdBeginRenderPass(commands[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
				vkCmdSetViewport(commands[i], 0, 1, &viewport);
				vkCmdSetScissor(commands[i], 0, 1, &scissor);
				for ( auto pass : passes ) {
					ext::vulkan::currentPass = pass + ";TOTEXTURE";
					for ( auto graphic : graphics ) {
						if ( graphic->renderMode && graphic->renderMode->getName() != this->getName() ) continue;
						graphic->createCommandBuffer(commands[i] );
					}
				}
			vkCmdEndRenderPass(commands[i]);
		}

		VK_CHECK_RESULT(vkEndCommandBuffer(commands[i]));
	}
}