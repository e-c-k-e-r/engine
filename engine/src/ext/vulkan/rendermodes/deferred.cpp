#include <uf/ext/vulkan/vulkan.h>
#include <uf/ext/vulkan/rendermodes/deferred.h>
#include <uf/ext/vulkan/rendermodes/rendertarget.h>
#include <uf/ext/vulkan/initializers.h>
#include <uf/utils/window/window.h>
#include <uf/utils/mesh/mesh.h>

#include <uf/engine/scene/scene.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/math/transform.h>

std::string ext::vulkan::DeferredRenderMode::getType() const {
	return "Deferred";
}

void ext::vulkan::DeferredRenderMode::initialize( Device& device ) {
	ext::vulkan::RenderMode::initialize( device );
	{
		renderTarget.device = &device;
		// attach targets
		struct {
			size_t albedo, position, normals, depth, output;
		} attachments;

		attachments.albedo = renderTarget.attach( VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL ); // albedo
		attachments.position = renderTarget.attach( VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL ); // position
		attachments.normals = renderTarget.attach( VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL ); // normals
		attachments.depth = renderTarget.attach( device.formats.depth, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ); // depth
		// Attach swapchain's image as output
		{
			attachments.output = renderTarget.attachments.size();
			RenderTarget::Attachment swapchainAttachment;
			swapchainAttachment.format = device.formats.color;
			swapchainAttachment.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			swapchainAttachment.layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			swapchainAttachment.aliased = true;
			renderTarget.attachments.push_back(swapchainAttachment);
		}

		// First pass: fill the G-Buffer
		{
			renderTarget.addPass(
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				{ attachments.albedo, attachments.position, attachments.normals },
				{},
				attachments.depth
			);
		}
		// Second pass: write to output
		{
			renderTarget.addPass(
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,
				{ attachments.output },
				{ attachments.albedo, attachments.position, attachments.normals },
				attachments.depth
			);
		}
	}
	renderTarget.initialize( device );

	blitter.renderTarget = &renderTarget;
	blitter.initializeShaders({
		{"./data/shaders/display.subpass.vert.spv", VK_SHADER_STAGE_VERTEX_BIT},
		{"./data/shaders/display.subpass.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT}
	});
	blitter.subpass = 1;
	blitter.initialize( device, *this );

	// update layer rendertargets descriptor sets
	{
		std::vector<RenderMode*> layers = ext::vulkan::getRenderModes("RenderTarget", false); //{ &ext::vulkan::getRenderMode("Gui") };
		for ( auto layer : layers ) {
			RenderTargetRenderMode* rtLayer = (RenderTargetRenderMode*) layer;
			auto& blitter = rtLayer->blitter;
			// blitter.subpass = 1;
			blitter.initialize( device, *this );
		}
	}
}
void ext::vulkan::DeferredRenderMode::tick() {
	ext::vulkan::RenderMode::tick();
	std::vector<RenderMode*> layers = ext::vulkan::getRenderModes("RenderTarget", false);
	for ( auto layer : layers ) {
		RenderTargetRenderMode* rtLayer = (RenderTargetRenderMode*) layer;
		auto& blitter = rtLayer->blitter;
		// update descriptor set
		if ( !blitter.initialized ) {
			// blitter.subpass = 1;
			if ( blitter.renderTarget ) blitter.initialize( *device, *this );
		}
	}
	if ( ext::vulkan::resized ) {
		// destroy if exist
		{
			renderTarget.initialize( *renderTarget.device );
		}
		// update blitter descriptor set
		if ( blitter.initialized ) {
			std::vector<VkDescriptorImageInfo> inputDescriptors;
			auto& subpass = renderTarget.passes[blitter.subpass];
			for ( auto& input : subpass.inputs ) {
				inputDescriptors.push_back(ext::vulkan::initializers::descriptorImageInfo( 
					renderTarget.attachments[input.attachment].view,
					input.layout
				));
			}
			// Set descriptor set
			std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
				// Binding 0 : Projection/View matrix uniform buffer			
				ext::vulkan::initializers::writeDescriptorSet(
					blitter.descriptorSet,
					VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					0,
					&(blitter.buffers.at(0).descriptor)
				)
			};
			for ( size_t i = 0; i < inputDescriptors.size(); ++i ) {
				writeDescriptorSets.push_back(ext::vulkan::initializers::writeDescriptorSet(
					blitter.descriptorSet,
					VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
					i + 1,
					&inputDescriptors[i]
				));
			}
			blitter.initializeDescriptorSet( writeDescriptorSets );
		}
		// update layer rendertargets descriptor sets
		for ( auto layer : layers ) {
			RenderTargetRenderMode* rtLayer = (RenderTargetRenderMode*) layer;
			auto& blitter = rtLayer->blitter;
			auto& renderTarget = rtLayer->renderTarget;
			// update descriptor set
			if ( blitter.initialized ) {
				renderTarget.initialize( *renderTarget.device );
				VkDescriptorImageInfo samplerDescriptor; samplerDescriptor.sampler = blitter.sampler;
				std::vector<VkDescriptorImageInfo> colorDescriptors;
				for ( auto& attachment : renderTarget.attachments ) {
					if ( !(attachment.usage & VK_IMAGE_USAGE_SAMPLED_BIT) ) continue;
					colorDescriptors.push_back(ext::vulkan::initializers::descriptorImageInfo( 
						attachment.view,
						VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
					));
				}
			
				// Set descriptor set
				std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
					// Binding 0 : Projection/View matrix uniform buffer			
					ext::vulkan::initializers::writeDescriptorSet(
						blitter.descriptorSet,
						VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
						0,
						&(blitter.buffers.at(0).descriptor)
					),
					// Binding 1 : Sampler
					ext::vulkan::initializers::writeDescriptorSet(
						blitter.descriptorSet,
						VK_DESCRIPTOR_TYPE_SAMPLER,
						1,
						&samplerDescriptor
					),
				};
				for ( size_t i = 0; i < colorDescriptors.size(); ++i ) {
					writeDescriptorSets.push_back(ext::vulkan::initializers::writeDescriptorSet(
						blitter.descriptorSet,
						VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
						i + 2,
						&colorDescriptors[i]
					));
				}
				vkUpdateDescriptorSets( *device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr );
			}
		}
	}
}
void ext::vulkan::DeferredRenderMode::destroy() {
	ext::vulkan::RenderMode::destroy();
	blitter.destroy();
}
void ext::vulkan::DeferredRenderMode::createCommandBuffers( const std::vector<ext::vulkan::Graphic*>& graphics, const std::vector<std::string>& passes ) {
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

			// transition layers for read
		
			for ( auto layer : layers ) {
				if ( layer->getName() == "" ) continue;
				RenderTarget& renderTarget = layer->renderTarget;
				for ( auto& attachment : renderTarget.attachments ) {
					if ( !(attachment.usage & VK_IMAGE_USAGE_SAMPLED_BIT) ) continue;
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
					if ( graphic->renderMode->getName() != this->getName() ) continue;
					graphic->createCommandBuffer(commands[i] );
				}
				// render gui layer
				{
					for ( auto layer : layers ) {
						RenderTargetRenderMode* rtLayer = (RenderTargetRenderMode*) layer;
						if ( !rtLayer->blitter.initialized ) continue;
						if ( rtLayer->blitter.subpass != 0 ) continue;
						rtLayer->blitter.createCommandBuffer(commands[i]);
					}
				}
			vkCmdNextSubpass(commands[i], VK_SUBPASS_CONTENTS_INLINE);
				blitter.createCommandBuffer(commands[i]);
				// render gui layer
				{
					for ( auto layer : layers ) {
						RenderTargetRenderMode* rtLayer = (RenderTargetRenderMode*) layer;
						if ( !rtLayer->blitter.initialized ) continue;
						if ( rtLayer->blitter.subpass != 1 ) continue;
						rtLayer->blitter.createCommandBuffer(commands[i]);
					}
				}
			vkCmdEndRenderPass(commands[i]);

			for ( auto layer : layers ) {
				if ( layer->getName() == "" ) continue;
				RenderTarget& renderTarget = layer->renderTarget;
				for ( auto& attachment : renderTarget.attachments ) {
					if ( !(attachment.usage & VK_IMAGE_USAGE_SAMPLED_BIT) ) continue;
					imageMemoryBarrier.image = attachment.image;
					imageMemoryBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_SHADER_READ_BIT;
					imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
					imageMemoryBarrier.oldLayout = attachment.layout;
					imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
					vkCmdPipelineBarrier( commands[i], VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT , 0, 0, NULL, 0, NULL, 1, &imageMemoryBarrier );
					attachment.layout = imageMemoryBarrier.newLayout;
				}
			}
		}

		VK_CHECK_RESULT(vkEndCommandBuffer(commands[i]));
	}
}