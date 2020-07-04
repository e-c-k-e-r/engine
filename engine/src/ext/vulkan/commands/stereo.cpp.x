#include <uf/ext/vulkan/vulkan.h>
#include <uf/ext/vulkan/commands/multiview.h>
#include <uf/ext/vulkan/initializers.h>
#include <uf/utils/window/window.h>
#include <uf/utils/mesh/mesh.h>

namespace {
	uf::GuiMesh mesh;
}
const std::string& ext::vulkan::MultiviewCommand::getName() const {
	return "Multiview";
}
void ext::vulkan::MultiviewCommand::initialize( Device& device ) {
	framebuffers.left.initialize( device );
	framebuffers.right.initialize( device );
/*
	mesh.vertices = {
		{ {-1.0f, 1.0f}, {0.0f, 0.0f}, },
		{ {-1.0f, -1.0f}, {0.0f, 1.0f}, },
		{ {1.0f, -1.0f}, {1.0f, 1.0f}, },

		{ {-1.0f, 1.0f}, {0.0f, 0.0f}, },
		{ {1.0f, -1.0f}, {1.0f, 1.0f}, },
		{ {1.0f, 1.0f}, {1.0f, 0.0f}, }
	};
	mesh.initialize(true);
*/
}
void ext::vulkan::MultiviewCommand::destroy() {
	framebuffers.left.destroy();
	framebuffers.right.destroy();
}
void ext::vulkan::MultiviewCommand::createCommandBuffers( const std::vector<void*>& graphics, const std::vector<std::string>& passes ) {
	//auto& device = ext::vulkan::device;
	//auto& swapchain = ext::vulkan::swapchain;
	//auto& currentBuffer = ext::vulkan::currentBuffer;

	// destroy if exists
	if ( swapchain.commandBufferSet ) {
		auto* device = swapchain.device;
		bool vsync = swapchain.vsync;
		swapchain.destroy();
		swapchain.initialize( *device, 0, 0, vsync );
	}
	swapchain.commandBufferSet = true;

	// destroy if exist
	if ( framebuffers.left.commandBufferSet ) {
		auto* device = framebuffers.left.device;
		framebuffers.left.initialize( *device );
	}
	framebuffers.left.commandBufferSet = true;
	// destroy if exist
	if ( framebuffers.right.commandBufferSet ) {
		auto* device = framebuffers.right.device;
		framebuffers.right.initialize( *device );
	}
	framebuffers.right.commandBufferSet = true;

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
	renderPassBeginInfo.renderArea.offset.x = 0;
	renderPassBeginInfo.renderArea.offset.y = 0;
	renderPassBeginInfo.renderArea.extent.width = width;
	renderPassBeginInfo.renderArea.extent.height = height;
	renderPassBeginInfo.clearValueCount = 2;
	renderPassBeginInfo.pClearValues = clearValues;
	for (int32_t i = 0; i < swapchain.drawCommandBuffers.size(); ++i) {
		// Set target frame buffer

		VK_CHECK_RESULT(vkBeginCommandBuffer(swapchain.drawCommandBuffers[i], &cmdBufInfo));

		for ( auto& pGraphic : graphics ) {
			Graphic& graphic = *((Graphic*) pGraphic);
			graphic.createImageMemoryBarrier(swapchain.drawCommandBuffers[i]);
		}

		// Update dynamic viewport state
		VkViewport viewport = {};
		viewport.height = (float)height;
		viewport.width = (float)width;
		viewport.minDepth = (float) 0.0f;
		viewport.maxDepth = (float) 1.0f;
		// Update dynamic scissor state
		VkRect2D scissor = {};
		scissor.extent.width = width;
		scissor.extent.height = height;
		scissor.offset.x = 0;
		scissor.offset.y = 0;

		VkImageMemoryBarrier imageMemoryBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		imageMemoryBarrier.oldLayout = framebuffers.left.targets[0].layout;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		imageMemoryBarrier.image = framebuffers.left.targets[0].image;
		imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
		imageMemoryBarrier.subresourceRange.levelCount = 1;
		imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
		imageMemoryBarrier.subresourceRange.layerCount = 1;
		imageMemoryBarrier.srcQueueFamilyIndex = ext::vulkan::device.queueFamilyIndices.graphics;
		imageMemoryBarrier.dstQueueFamilyIndex = ext::vulkan::device.queueFamilyIndices.graphics;
		vkCmdPipelineBarrier( swapchain.drawCommandBuffers[i], VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, NULL, 0, NULL, 1, &imageMemoryBarrier );
		framebuffers.left.targets[0].layout = imageMemoryBarrier.newLayout;

		// Transition the depth buffer to VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL on first use
		if ( framebuffers.left.targets[1].layout == VK_IMAGE_LAYOUT_UNDEFINED ) {
			imageMemoryBarrier.image = framebuffers.left.targets[1].image;
			imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			imageMemoryBarrier.srcAccessMask = 0;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			imageMemoryBarrier.oldLayout = framebuffers.left.targets[1].layout;
			imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			vkCmdPipelineBarrier( swapchain.drawCommandBuffers[i], VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, 0, 0, NULL, 0, NULL, 1, &imageMemoryBarrier );
			framebuffers.left.targets[1].layout = imageMemoryBarrier.newLayout;
		}

		// Start the first sub pass specified in our default render pass setup by the base class
		// This will clear the color and depth attachment
		renderPassBeginInfo.renderPass = framebuffers.left.renderPass;
		renderPassBeginInfo.framebuffer = framebuffers.left.framebuffer;
		vkCmdBeginRenderPass(swapchain.drawCommandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdSetViewport(swapchain.drawCommandBuffers[i], 0, 1, &viewport);
			vkCmdSetScissor(swapchain.drawCommandBuffers[i], 0, 1, &scissor);
			for ( auto pass : passes ) {
				ext::vulkan::currentPass = pass + ";STEREO_LEFT";
				for ( auto& pGraphic : graphics ) {
					Graphic& graphic = *((Graphic*) pGraphic);
					graphic.createCommandBuffer(swapchain.drawCommandBuffers[i] );
				}
			}
		vkCmdEndRenderPass(swapchain.drawCommandBuffers[i]);

		{
			// Transition eye image to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL for display on the companion window
			imageMemoryBarrier.image = framebuffers.left.targets[0].image;
			imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			imageMemoryBarrier.oldLayout = framebuffers.left.targets[0].layout;
			imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			vkCmdPipelineBarrier( swapchain.drawCommandBuffers[i], VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &imageMemoryBarrier );
			framebuffers.left.targets[0].layout = imageMemoryBarrier.newLayout;
		}

		imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		imageMemoryBarrier.oldLayout = framebuffers.right.targets[0].layout;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		imageMemoryBarrier.image = framebuffers.right.targets[0].image;
		imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
		imageMemoryBarrier.subresourceRange.levelCount = 1;
		imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
		imageMemoryBarrier.subresourceRange.layerCount = 1;
		imageMemoryBarrier.srcQueueFamilyIndex = ext::vulkan::device.queueFamilyIndices.graphics;
		imageMemoryBarrier.dstQueueFamilyIndex = ext::vulkan::device.queueFamilyIndices.graphics;
		vkCmdPipelineBarrier( swapchain.drawCommandBuffers[i], VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, NULL, 0, NULL, 1, &imageMemoryBarrier );
		framebuffers.right.targets[0].layout = imageMemoryBarrier.newLayout;

		// Transition the depth buffer to VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL on first use
		if ( framebuffers.right.targets[1].layout == VK_IMAGE_LAYOUT_UNDEFINED ) {
			imageMemoryBarrier.image = framebuffers.right.targets[1].image;
			imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			imageMemoryBarrier.srcAccessMask = 0;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			imageMemoryBarrier.oldLayout = framebuffers.right.targets[1].layout;
			imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			vkCmdPipelineBarrier( swapchain.drawCommandBuffers[i], VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, 0, 0, NULL, 0, NULL, 1, &imageMemoryBarrier );
			framebuffers.right.targets[1].layout = imageMemoryBarrier.newLayout;
		}

		renderPassBeginInfo.renderPass = framebuffers.right.renderPass;
		renderPassBeginInfo.framebuffer = framebuffers.right.framebuffer;
		vkCmdBeginRenderPass(swapchain.drawCommandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdSetViewport(swapchain.drawCommandBuffers[i], 0, 1, &viewport);
			vkCmdSetScissor(swapchain.drawCommandBuffers[i], 0, 1, &scissor);
			for ( auto pass : passes ) {
				ext::vulkan::currentPass = pass + ";STEREO_RIGHT";
				for ( auto& pGraphic : graphics ) {
					Graphic& graphic = *((Graphic*) pGraphic);
				//	graphic.createCommandBuffer(swapchain.drawCommandBuffers[i] );
				}
			}
		vkCmdEndRenderPass(swapchain.drawCommandBuffers[i]);

		{
			// Transition eye image to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL for display on the companion window
			imageMemoryBarrier.image = framebuffers.right.targets[0].image;
			imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			imageMemoryBarrier.oldLayout = framebuffers.right.targets[0].layout;
			imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			vkCmdPipelineBarrier( swapchain.drawCommandBuffers[i], VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &imageMemoryBarrier );
			framebuffers.right.targets[0].layout = imageMemoryBarrier.newLayout;
		}

		imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		imageMemoryBarrier.image = swapchain.buffers[i].image;
		imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
		imageMemoryBarrier.subresourceRange.levelCount = 1;
		imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
		imageMemoryBarrier.subresourceRange.layerCount = 1;
		imageMemoryBarrier.srcQueueFamilyIndex = ext::vulkan::device.queueFamilyIndices.graphics;
		imageMemoryBarrier.dstQueueFamilyIndex = ext::vulkan::device.queueFamilyIndices.graphics;
		vkCmdPipelineBarrier( swapchain.drawCommandBuffers[i], VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, NULL, 0, NULL, 1, &imageMemoryBarrier );

		renderPassBeginInfo.renderPass = swapchain.renderPass;
		renderPassBeginInfo.framebuffer = swapchain.frameBuffers[i];
		vkCmdBeginRenderPass(swapchain.drawCommandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdSetViewport(swapchain.drawCommandBuffers[i], 0, 1, &viewport);
			vkCmdSetScissor(swapchain.drawCommandBuffers[i], 0, 1, &scissor);

		//	::mesh.graphic.createCommandBuffer(swapchain.drawCommandBuffers[i]);
	

		vkCmdEndRenderPass(swapchain.drawCommandBuffers[i]);

		// Ending the render pass will add an implicit barrier transitioning the frame buffer color attachment to 
		// VK_IMAGE_LAYOUT_PRESENT_SRC_KHR for presenting it to the windowing system

		VK_CHECK_RESULT(vkEndCommandBuffer(swapchain.drawCommandBuffers[i]));
	}
}

void ext::vulkan::MultiviewCommand::render() {
	//auto& device = ext::vulkan::device;
	//auto& swapchain = ext::vulkan::swapchain;
	//auto& currentBuffer = ext::vulkan::currentBuffer;

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