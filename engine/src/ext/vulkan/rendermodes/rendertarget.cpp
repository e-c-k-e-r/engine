#include <uf/ext/vulkan/vulkan.h>
#include <uf/ext/vulkan/rendermodes/rendertarget.h>
#include <uf/ext/vulkan/initializers.h>
#include <uf/utils/window/window.h>
#include <uf/utils/mesh/mesh.h>

std::string ext::vulkan::RenderTargetRenderMode::getType() const {
	return "RenderTarget";
}
size_t ext::vulkan::RenderTargetRenderMode::subpasses() const {
	return renderTarget.passes.size();
}

void ext::vulkan::RenderTargetRenderMode::initialize( Device& device ) {
	ext::vulkan::RenderMode::initialize( device );
	{
		renderTarget.device = &device;
		// attach targets
		struct {
			size_t color, depth;
		} attachments;

		attachments.color = renderTarget.attach( VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL ); // albedo
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
	blitter.initialize();
}
void ext::vulkan::RenderTargetRenderMode::destroy() {
	ext::vulkan::RenderMode::destroy();
	blitter.destroy();
}
void ext::vulkan::RenderTargetRenderMode::render() {
	// Submit commands
	// Use a fence to ensure that command buffer has finished executing before using it again
	vkWaitForFences( *device, 1, &fences[currentBuffer], VK_TRUE, UINT64_MAX );
	vkResetFences( *device, 1, &fences[currentBuffer] );

	VkSubmitInfo renderSubmitInfo = ext::vulkan::initializers::submitInfo();
	renderSubmitInfo.commandBufferCount = 1;
	renderSubmitInfo.pCommandBuffers = &commands[currentBuffer];

	VK_CHECK_RESULT(vkQueueSubmit(device->graphicsQueue, 1, &renderSubmitInfo, fences[currentBuffer]));
}
void ext::vulkan::RenderTargetRenderMode::createCommandBuffers( const std::vector<ext::vulkan::Graphic*>& graphics, const std::vector<std::string>& passes ) {
	// destroy if exists
	if ( ext::vulkan::rebuild ) {
		// destroy if exist
		if ( renderTarget.initialized ) {
			auto* device = renderTarget.device;
			renderTarget.initialize( *device );
		}
		renderTarget.initialized = true;

		// update descriptor set
		if ( blitter.initialized ) {
			VkDescriptorImageInfo renderTargetDescription = ext::vulkan::initializers::descriptorImageInfo( 
				renderTarget.attachments[0].view,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				blitter.sampler
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
					VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					1,
					&renderTargetDescription
				),
			};
			vkUpdateDescriptorSets( *device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr );
		}
	}

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
			clearValues[1].depthStencil = { 1.0f, 0 };

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