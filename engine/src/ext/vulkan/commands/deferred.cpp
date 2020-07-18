#include <uf/ext/vulkan/vulkan.h>
#include <uf/ext/vulkan/commands/deferred.h>
#include <uf/ext/vulkan/initializers.h>
#include <uf/utils/window/window.h>
#include <uf/utils/mesh/mesh.h>

namespace {
	// 0 left 1 right
	uint8_t DOMINANT_EYE = 1;
}

const std::string& ext::vulkan::DeferredCommand::getName() const {
	return "Defered";
}
size_t ext::vulkan::DeferredCommand::subpasses() const {
	return framebuffer.passes.size();
}
VkRenderPass& ext::vulkan::DeferredCommand::getRenderPass() {
	return framebuffer.renderPass;
}

void ext::vulkan::DeferredCommand::initialize( Device& device ) {
	{
		framebuffer.device = &device;
		// attach targets
		struct {
			size_t albedo, normals, depth, output;
		} attachments;

		attachments.albedo = framebuffer.attach( VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT ); // albedo
		attachments.normals = framebuffer.attach( VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT ); // normals
		attachments.depth = framebuffer.attach( swapchain.depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT ); // depth
		// Attach swapchain's image as output
		{
			attachments.output = framebuffer.attachments.size();
			framebuffer.attachments.push_back({ swapchain.colorFormat, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, });
		}

		// First pass: fill the G-Buffer
		{
			framebuffer.addPass(
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				{ attachments.albedo, attachments.normals, },
				{},
				attachments.depth
			);
		}
		// Second pass: write to output
		{
			framebuffer.addPass(
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT,
				{ attachments.output, },
				{ attachments.albedo, attachments.normals },
				attachments.depth
			);
		}
	}

	framebuffer.initialize( device );

	blitter.framebuffer = &framebuffer;
	blitter.initializeShaders({
		{"./data/shaders/display.vert.spv", VK_SHADER_STAGE_VERTEX_BIT},
		{"./data/shaders/display.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT}
	});
	blitter.initialize( device, ext::vulkan::swapchain );
}
void ext::vulkan::DeferredCommand::destroy() {
	framebuffer.destroy();
}
void ext::vulkan::DeferredCommand::createCommandBuffers( const std::vector<void*>& graphics, const std::vector<std::string>& passes ) {
	// destroy if exists
	if ( swapchain.rebuild ) {
		if ( swapchain.commandBufferSet ) {
			auto* device = swapchain.device;
			bool vsync = swapchain.vsync;
			swapchain.destroy();
			swapchain.initialize( *device, 0, 0, vsync );
		}
		swapchain.commandBufferSet = true;

		// destroy if exist
		if ( framebuffer.commandBufferSet ) {
			auto* device = framebuffer.device;
			framebuffer.initialize( *device );
		}
		framebuffer.commandBufferSet = true;

		// update descriptor set
		if ( blitter.initialized ) {
			VkDescriptorImageInfo textDescriptorAlbedo = ext::vulkan::initializers::descriptorImageInfo( 
				framebuffer.attachments[0].view,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			);
			std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
				// Binding 0 : Projection/View matrix uniform buffer			
				ext::vulkan::initializers::writeDescriptorSet(
					blitter.descriptorSet,
					VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					0,
					&(blitter.buffers.at(0).descriptor)
				),
				// Binding 1 : Albedo input attachment
				ext::vulkan::initializers::writeDescriptorSet(
					blitter.descriptorSet,
					VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
					1,
					&textDescriptorAlbedo
				),
			};
			vkUpdateDescriptorSets( device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr );
		}
	}

	float width = this->width > 0 ? this->width : ext::vulkan::width;
	float height = this->height > 0 ? this->height : ext::vulkan::height;
	blitter.uniforms.screenSize = { width, height };

	VkCommandBufferBeginInfo cmdBufInfo = {};
	cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBufInfo.pNext = nullptr;
	
	for (int32_t i = 0; i < swapchain.drawCommandBuffers.size(); ++i) {

		VK_CHECK_RESULT(vkBeginCommandBuffer(swapchain.drawCommandBuffers[i], &cmdBufInfo));
		// Fill GBuffer
		{
			std::vector<VkClearValue> clearValues; clearValues.resize(4);
			clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
			clearValues[1].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
			clearValues[2].depthStencil = { 1.0f, 0 };
			clearValues[3].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };

			VkRenderPassBeginInfo renderPassBeginInfo = {};
			renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassBeginInfo.pNext = nullptr;
			renderPassBeginInfo.renderArea.offset.x = 0;
			renderPassBeginInfo.renderArea.offset.y = 0;
			renderPassBeginInfo.renderArea.extent.width = width;
			renderPassBeginInfo.renderArea.extent.height = height;
			renderPassBeginInfo.clearValueCount = clearValues.size();
			renderPassBeginInfo.pClearValues = &clearValues[0];
			renderPassBeginInfo.renderPass = framebuffer.renderPass;
			renderPassBeginInfo.framebuffer = framebuffer.framebuffers[i];

			for ( auto& pGraphic : graphics ) {
				Graphic& graphic = *((Graphic*) pGraphic);
				graphic.createImageMemoryBarrier(swapchain.drawCommandBuffers[i]);
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

			vkCmdBeginRenderPass(swapchain.drawCommandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
				vkCmdSetViewport(swapchain.drawCommandBuffers[i], 0, 1, &viewport);
				vkCmdSetScissor(swapchain.drawCommandBuffers[i], 0, 1, &scissor);
				for ( auto pass : passes ) {
					ext::vulkan::currentPass = pass + ";DEFERRED";
					for ( auto& pGraphic : graphics ) {
						Graphic& graphic = *((Graphic*) pGraphic);
						graphic.createCommandBuffer(swapchain.drawCommandBuffers[i] );
					}
				}
			vkCmdNextSubpass(swapchain.drawCommandBuffers[i], VK_SUBPASS_CONTENTS_INLINE);
				blitter.createCommandBuffer(swapchain.drawCommandBuffers[i]);
			vkCmdEndRenderPass(swapchain.drawCommandBuffers[i]);
		}

		VK_CHECK_RESULT(vkEndCommandBuffer(swapchain.drawCommandBuffers[i]));
	}
}

void ext::vulkan::DeferredCommand::render() {
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