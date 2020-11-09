#include <uf/ext/vulkan/vulkan.h>
#include <uf/ext/vulkan/rendermodes/rendertarget.h>
#include <uf/ext/vulkan/initializers.h>
#include <uf/utils/window/window.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/ext/vulkan/graphic.h>

std::string ext::vulkan::RenderTargetRenderMode::getType() const {
	return "RenderTarget";
}
const std::string& ext::vulkan::RenderTargetRenderMode::getName( bool wantsTarget ) const {
	return wantsTarget ? this->target : this->name;
}

void ext::vulkan::RenderTargetRenderMode::initialize( Device& device ) {
	ext::vulkan::RenderMode::initialize( device );

	this->target = this->name;

	{
		renderTarget.device = &device;
		struct {
			size_t albedo, normals, position, depth, output;
		} attachments;

		attachments.albedo = renderTarget.attach( VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, true ); // albedo
		attachments.normals = renderTarget.attach( VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, false ); // normals
		if ( !settings::experimental::deferredReconstructPosition )
			attachments.position = renderTarget.attach( VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, false ); // position
		attachments.depth = renderTarget.attach( device.formats.depth, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, false ); // depth
		// Attach swapchain's image as output
		if ( !false ) {
			attachments.output = renderTarget.attach( device.formats.color, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, true ); // output
		} else {
			attachments.output = renderTarget.attachments.size();
			RenderTarget::Attachment swapchainAttachment;
			swapchainAttachment.format = device.formats.color;
			swapchainAttachment.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			swapchainAttachment.layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			swapchainAttachment.aliased = true;
			{
				VkBool32 blendEnabled = VK_TRUE;
				VkColorComponentFlags writeMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
				VkPipelineColorBlendAttachmentState blendAttachmentState = ext::vulkan::initializers::pipelineColorBlendAttachmentState(
					writeMask,
					blendEnabled
				);
				if ( blendEnabled == VK_TRUE ) {
					blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
					blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
					blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
					blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
					blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
					blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
				}
				swapchainAttachment.blendState = blendAttachmentState;
			}
			renderTarget.attachments.push_back(swapchainAttachment);
		}

		// First pass: write to target
		if ( settings::experimental::deferredReconstructPosition ) {
			renderTarget.addPass(
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				{ attachments.albedo, attachments.normals },
				{},
				attachments.depth
			);
		} else {
			renderTarget.addPass(
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				{ attachments.albedo, attachments.normals, attachments.position },
				{},
				attachments.depth
			);
		}
		// NOP
		if ( settings::experimental::deferredReconstructPosition ) {
			renderTarget.addPass(
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,
				{ attachments.output },
				{ attachments.albedo, attachments.normals, attachments.depth },
				attachments.depth
			);
		} else {
			renderTarget.addPass(
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,
				{ attachments.output },
				{ attachments.albedo, attachments.normals, attachments.position },
				attachments.depth
			);
		}
	}
	renderTarget.initialize( device );

	if ( blitter.process ) {
		uf::BaseMesh<pod::Vertex_2F2F, uint32_t> mesh;
		mesh.vertices = {
			{ {-1.0f, 1.0f}, {0.0f, 1.0f}, },
			{ {-1.0f, -1.0f}, {0.0f, 0.0f}, },
			{ {1.0f, -1.0f}, {1.0f, 0.0f}, },
			{ {1.0f, 1.0f}, {1.0f, 1.0f}, }
		};
		mesh.indices = {
			0, 1, 2, 2, 3, 0
		};
		blitter.device = &device;
		blitter.material.device = &device;
		blitter.initializeGeometry( mesh );
		blitter.material.initializeShaders({
			{"./data/shaders/display.renderTarget.vert.spv", VK_SHADER_STAGE_VERTEX_BIT},
			{"./data/shaders/display.renderTarget.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT}
		});
		
		for ( auto& attachment : renderTarget.attachments ) {
			if ( !(attachment.usage & VK_IMAGE_USAGE_SAMPLED_BIT) ) continue;

			Texture2D& texture = blitter.material.textures.emplace_back();
			texture.aliasAttachment(attachment);
		}
	// 	blitter.initializePipeline();
	}
}
void ext::vulkan::RenderTargetRenderMode::tick() {
	ext::vulkan::RenderMode::tick();
	if ( ext::vulkan::states::resized ) {
		renderTarget.initialize( *renderTarget.device );
		blitter.material.textures.clear();
		for ( auto& attachment : renderTarget.attachments ) {
			if ( !(attachment.usage & VK_IMAGE_USAGE_SAMPLED_BIT) ) continue;
			Texture2D& texture = blitter.material.textures.emplace_back();
			texture.aliasAttachment(attachment);
		}
		if ( blitter.process ) {
			blitter.getPipeline().update( blitter );
		//	blitter.updatePipelines();
		}
	}
}
void ext::vulkan::RenderTargetRenderMode::destroy() {
	ext::vulkan::RenderMode::destroy();
	blitter.destroy();
}
void ext::vulkan::RenderTargetRenderMode::render() {
	auto& commands = getCommands( this->mostRecentCommandPoolId );
	// Submit commands
	// Use a fence to ensure that command buffer has finished executing before using it again
	VK_CHECK_RESULT(vkWaitForFences( *device, 1, &fences[states::currentBuffer], VK_TRUE, UINT64_MAX ));
	VK_CHECK_RESULT(vkResetFences( *device, 1, &fences[states::currentBuffer] ));

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
	submitInfo.pCommandBuffers = &commands[states::currentBuffer];		// Command buffers(s) to execute in this batch (submission)
	submitInfo.commandBufferCount = 1;

	VK_CHECK_RESULT(vkQueueSubmit(device->getQueue( Device::QueueEnum::GRAPHICS ), 1, &submitInfo, fences[states::currentBuffer]));
	//vkQueueSubmit(device->queues.graphics, 1, &submitInfo, fences[states::currentBuffer]);
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
void ext::vulkan::RenderTargetRenderMode::pipelineBarrier( VkCommandBuffer commandBuffer, uint8_t state ) {
	VkImageMemoryBarrier imageMemoryBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // ext::vulkan::device.queueFamilyIndices.graphics; //VK_QUEUE_FAMILY_IGNORED
	imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // ext::vulkan::device.queueFamilyIndices.graphics; //VK_QUEUE_FAMILY_IGNORED
	imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
	imageMemoryBarrier.subresourceRange.levelCount = 1;
	imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
	imageMemoryBarrier.subresourceRange.layerCount = 1;
	imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

	for ( auto& attachment : renderTarget.attachments ) {
		if ( !(attachment.usage & VK_IMAGE_USAGE_SAMPLED_BIT) ) continue;
		if (  (attachment.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) ) continue;
		
		VkPipelineStageFlags srcStageMask, dstStageMask;
		imageMemoryBarrier.image = attachment.image;
		imageMemoryBarrier.oldLayout = attachment.layout;
		imageMemoryBarrier.newLayout = attachment.layout;
	
		switch ( state ) {
			case 0: {
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_SHADER_READ_BIT;
				imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			} break;
			case 1: {
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_SHADER_READ_BIT;
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				imageMemoryBarrier.newLayout = attachment.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
				dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			} break;
			// ensure 
			default: {
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
				dstStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			}
		}
		
		vkCmdPipelineBarrier( commandBuffer,
			srcStageMask, dstStageMask,
			VK_FLAGS_NONE,
			0, NULL,
			0, NULL,
			1, &imageMemoryBarrier
		);

		attachment.layout = imageMemoryBarrier.newLayout;
	}
}
void ext::vulkan::RenderTargetRenderMode::createCommandBuffers( const std::vector<ext::vulkan::Graphic*>& graphics ) {
	// destroy if exists
	float width = this->width > 0 ? this->width : ext::vulkan::settings::width;
	float height = this->height > 0 ? this->height : ext::vulkan::settings::height;

	VkCommandBufferBeginInfo cmdBufInfo = {};
	cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBufInfo.pNext = nullptr;
	auto& commands = getCommands();
	for (size_t i = 0; i < commands.size(); ++i) {
		VK_CHECK_RESULT(vkBeginCommandBuffer(commands[i], &cmdBufInfo));
		{
			std::vector<VkClearValue> clearValues;
			for ( auto& attachment : renderTarget.attachments ) {
				VkClearValue clearValue;
				if ( attachment.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT ) {
					clearValue.color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
				} else if ( attachment.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT ) {
					clearValue.depthStencil = { 0.0f, 0 };
				}
				clearValues.push_back(clearValue);
			}

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
				for ( auto graphic : graphics ) {
					if ( graphic->descriptor.renderMode != this->target ) continue;
					ext::vulkan::GraphicDescriptor descriptor = graphic->descriptor;
				//	descriptor.renderMode = this->name;
					graphic->record(commands[i], descriptor );
				}
			if ( !false ) vkCmdNextSubpass(commands[i], VK_SUBPASS_CONTENTS_INLINE);
			vkCmdEndRenderPass(commands[i]);
		}

		VK_CHECK_RESULT(vkEndCommandBuffer(commands[i]));
	}
}