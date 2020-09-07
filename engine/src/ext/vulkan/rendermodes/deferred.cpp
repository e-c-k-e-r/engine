#include <uf/ext/vulkan/vulkan.h>
#include <uf/ext/vulkan/rendermodes/deferred.h>
#include <uf/ext/vulkan/rendermodes/rendertarget.h>
#include <uf/ext/vulkan/initializers.h>
#include <uf/utils/window/window.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/ext/vulkan/graphic.h>
#include <uf/engine/scene/scene.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/math/transform.h>

#include <uf/ext/vulkan/graphic.h>

std::string ext::vulkan::DeferredRenderMode::getType() const {
	return "Deferred";
}

void ext::vulkan::DeferredRenderMode::initialize( Device& device ) {
	ext::vulkan::RenderMode::initialize( device );
	{
		renderTarget.device = &device;
		// attach targets
		struct {
			size_t albedo, normals, position, depth, output, ping, pong;
		} attachments;

		attachments.albedo = renderTarget.attach( VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, true ); // albedo
		attachments.normals = renderTarget.attach( VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, true ); // normals
		attachments.position = renderTarget.attach( VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, true ); // position
		attachments.depth = renderTarget.attach( device.formats.depth, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, true ); // depth
	//	attachments.ping = renderTarget.attach( VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL ); // albedo
	//	attachments.pong = renderTarget.attach( VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL ); // albedo
		// Attach swapchain's image as output
		{
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
					blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
					blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
					blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
				}
				swapchainAttachment.blendState = blendAttachmentState;
			}
			renderTarget.attachments.push_back(swapchainAttachment);
		}

		// First pass: fill the G-Buffer
		{
			renderTarget.addPass(
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				{ attachments.albedo, attachments.normals, attachments.position },
				{},
				attachments.depth
			);
		}
		// Second pass: write to output
		{
			renderTarget.addPass(
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,
				{ attachments.output },
				{ attachments.albedo, attachments.normals, attachments.position/*, attachments.depth*/ },
				attachments.depth
			);
		}
	}
	renderTarget.initialize( device );

	{
		uf::BaseMesh<pod::Vertex_2F2F, uint16_t> mesh;
		mesh.vertices = {
			{ {-1.0f, 1.0f}, {0.0f, 1.0f}, },
			{ {-1.0f, -1.0f}, {0.0f, 0.0f}, },
			{ {1.0f, -1.0f}, {1.0f, 0.0f}, },
			{ {1.0f, 1.0f}, {1.0f, 1.0f}, }
		};
		mesh.indices = {
			0, 1, 2, 0, 2, 3
		};
		blitter.descriptor.subpass = 1;
		blitter.descriptor.depthTest.test = false;
		blitter.descriptor.depthTest.write = false;

		blitter.initialize( this->getName() );
		blitter.initializeGeometry( mesh );
		blitter.material.initializeShaders({
			{"./data/shaders/display.subpass.vert.spv", VK_SHADER_STAGE_VERTEX_BIT},
			{"./data/shaders/display.subpass.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT}
		});
		blitter.initializePipeline();
	}
}
void ext::vulkan::DeferredRenderMode::tick() {
	ext::vulkan::RenderMode::tick();
	
	if ( ext::vulkan::resized ) {
		renderTarget.initialize( *renderTarget.device );
		// update blitter descriptor set
		if ( blitter.initialized ) {
			auto& pipeline = blitter.getPipeline();
			pipeline.update( blitter );
		}
	}
}
void ext::vulkan::DeferredRenderMode::destroy() {
	ext::vulkan::RenderMode::destroy();
	blitter.destroy();
}
void ext::vulkan::DeferredRenderMode::createCommandBuffers( const std::vector<ext::vulkan::Graphic*>& graphics ) {
	// destroy if exists
	// ext::vulkan::RenderMode& swapchain = 
	float width = this->width > 0 ? this->width : ext::vulkan::width;
	float height = this->height > 0 ? this->height : ext::vulkan::height;

	VkCommandBufferBeginInfo cmdBufInfo = {};
	cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBufInfo.pNext = nullptr;

	VkImageMemoryBarrier imageMemoryBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // ext::vulkan::device.queueFamilyIndices.graphics; //VK_QUEUE_FAMILY_IGNORED
	imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // ext::vulkan::device.queueFamilyIndices.graphics; //VK_QUEUE_FAMILY_IGNORED
	imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
	imageMemoryBarrier.subresourceRange.levelCount = 1;
	imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
	imageMemoryBarrier.subresourceRange.layerCount = 1;
	imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		
	std::vector<RenderMode*> layers = ext::vulkan::getRenderModes("RenderTarget", false);

	for (size_t i = 0; i < commands.size(); ++i) {
		VK_CHECK_RESULT(vkBeginCommandBuffer(commands[i], &cmdBufInfo));
		// Fill GBuffer
		{
			std::vector<VkClearValue> clearValues;
			for ( auto& attachment : renderTarget.attachments ) {
				VkClearValue clearValue;
				if ( attachment.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT ) {
					clearValue.color = { { clearColor.x, clearColor.y, clearColor.z, clearColor.w } };
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

			// transition layers for read
		
			for ( auto layer : layers ) {
				if ( layer->getName() == "" ) continue;
				RenderTarget& renderTarget = layer->renderTarget;
				for ( auto& attachment : renderTarget.attachments ) {
					if ( !(attachment.usage & VK_IMAGE_USAGE_SAMPLED_BIT) ) continue;
					if (  (attachment.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) ) continue;
					imageMemoryBarrier.image = attachment.image;
					imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
					imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_SHADER_READ_BIT;
					imageMemoryBarrier.oldLayout = attachment.layout;
					imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					vkCmdPipelineBarrier( commands[i], VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT , VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &imageMemoryBarrier );
					attachment.layout = imageMemoryBarrier.newLayout;
				}
			}
		
			vkCmdBeginRenderPass(commands[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
				vkCmdSetViewport(commands[i], 0, 1, &viewport);
				vkCmdSetScissor(commands[i], 0, 1, &scissor);
				for ( auto graphic : graphics ) {
					// only draw graphics that are assigned to this type of render mode
					if ( graphic->descriptor.renderMode != this->getName() ) continue;
					graphic->record(commands[i] );
				}
				// render gui layer
				{
					for ( auto _ : layers ) {
						RenderTargetRenderMode* layer = (RenderTargetRenderMode*) _;
						auto& blitter = layer->blitter;
						if ( !blitter.initialized ) continue;
						if ( blitter.descriptor.subpass != 0 ) continue;
						blitter.record(commands[i]);
					}
				}
			vkCmdNextSubpass(commands[i], VK_SUBPASS_CONTENTS_INLINE);
				{
					blitter.record(commands[i]);
				}
				// render gui layer
				{
					for ( auto _ : layers ) {
						RenderTargetRenderMode* layer = (RenderTargetRenderMode*) _;
						auto& blitter = layer->blitter;
						if ( !blitter.initialized ) continue;
						if ( blitter.descriptor.subpass != 1 ) continue;
						blitter.record(commands[i]);
					}
				}
			vkCmdEndRenderPass(commands[i]);

			for ( auto layer : layers ) {
				if ( layer->getName() == "" ) continue;
				RenderTarget& renderTarget = layer->renderTarget;
				for ( auto& attachment : renderTarget.attachments ) {
					if ( !(attachment.usage & VK_IMAGE_USAGE_SAMPLED_BIT) ) continue;
					if (  (attachment.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) ) continue;
					imageMemoryBarrier.image = attachment.image;
					imageMemoryBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_SHADER_READ_BIT;
					imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
					imageMemoryBarrier.oldLayout = attachment.layout;
					imageMemoryBarrier.newLayout = attachment.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
					vkCmdPipelineBarrier( commands[i], VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT , 0, 0, NULL, 0, NULL, 1, &imageMemoryBarrier );
					attachment.layout = imageMemoryBarrier.newLayout;
				}
			}
		}

		VK_CHECK_RESULT(vkEndCommandBuffer(commands[i]));
	}
}