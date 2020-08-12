#include <uf/ext/vulkan/vulkan.h>
#include <uf/ext/vulkan/rendermodes/deferred.h>
#include <uf/ext/vulkan/rendermodes/stereoscopic_deferred.h>
#include <uf/ext/vulkan/rendermodes/rendertarget.h>
#include <uf/ext/vulkan/initializers.h>
#include <uf/utils/window/window.h>
#include <uf/utils/mesh/mesh.h>

#include <uf/engine/scene/scene.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/math/transform.h>
#include <uf/ext/openvr/openvr.h>

namespace {
	// 0 left 1 right
	uint8_t DOMINANT_EYE = 0;
}

//ext::vulkan::StereoscopicDeferredRenderMode::StereoscopicDeferredRenderMode() : renderTargets({ renderTarget }), blitters({ blitter }) {
ext::vulkan::StereoscopicDeferredRenderMode::StereoscopicDeferredRenderMode() : renderTargets({ renderTarget }) {
}

std::string ext::vulkan::StereoscopicDeferredRenderMode::getType() const {
	return "Deferred (Stereoscopic)";
}

void ext::vulkan::StereoscopicDeferredRenderMode::initialize( Device& device ) {
	ext::vulkan::RenderMode::initialize( device );
	
	struct EYES {
		RenderTarget* renderTarget;
		DeferredRenderingGraphic* blitter;
	};
	std::vector<EYES> eyes = {
		{ &renderTargets.left, &blitters.left },
		{ &renderTargets.right, &blitters.right },
	};
	for ( auto& eye : eyes ) {
		auto& renderTarget = *eye.renderTarget;
		auto& blitter = *eye.blitter;
		renderTarget.device = &device;
		renderTarget.width = this->width;
		renderTarget.height = this->height;
		// attach targets
		struct {
			size_t albedo, position, normals, depth, output;
		} attachments;

		attachments.albedo = renderTarget.attach( VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL ); // albedo
		attachments.position = renderTarget.attach( VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL ); // position
		attachments.normals = renderTarget.attach( VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL ); // normals
		attachments.depth = renderTarget.attach( device.formats.depth, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ); // depth
		attachments.output = renderTarget.attach( VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL ); // albedo
		// Attach swapchain's image as output
	/*
		if ( !true ) {
			attachments.swapchain = renderTarget.attachments.size();
			RenderTarget::Attachment swapchainAttachment;
			swapchainAttachment.format = device.formats.color;
			swapchainAttachment.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			swapchainAttachment.layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			swapchainAttachment.aliased = true;
			renderTarget.attachments.push_back(swapchainAttachment);
		}
	*/
		// First pass: fill the G-Buffer
		{
			renderTarget.addPass(
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				{ attachments.albedo, attachments.position, attachments.normals },
				{},
				attachments.depth
			);
		}
		// Second pass: write to output
		{
			renderTarget.addPass(
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				{ attachments.output },
				{ attachments.albedo, attachments.position, attachments.normals },
				attachments.depth
			);
		}
		// Third render pass: write to swapchain
	/*
		{
			renderTarget.addPass(
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,
				{ attachments.swapchain },
				{ attachments.output },
				attachments.depth
			);
		}
	*/
		renderTarget.initialize( device );
		blitter.renderTarget = &renderTarget;
		blitter.initializeShaders({
			{"./data/shaders/display.subpass.vert.spv", VK_SHADER_STAGE_VERTEX_BIT},
			{"./data/shaders/display.subpass.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT}
		});

		blitter.subpass = 1;
		blitter.initialize( device, *this );
	}
/*
	{
		blitter.renderTarget = &renderTarget;
		blitter.initializeShaders({
			{"./data/shaders/display.vert.spv", VK_SHADER_STAGE_VERTEX_BIT},
			{"./data/shaders/display.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT}
		});
		blitter.subpass = 2;
		blitter.initialize( device, *this );
	}
*/
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
void ext::vulkan::StereoscopicDeferredRenderMode::tick() {
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
		struct EYES {
			RenderTarget* renderTarget;
			DeferredRenderingGraphic* blitter;
		};
		std::vector<EYES> eyes = {
			{ &renderTargets.left, &blitters.left },
			{ &renderTargets.right, &blitters.right },
		};
		for ( auto& eye : eyes ) {
			auto& renderTarget = *eye.renderTarget;
			auto& blitter = *eye.blitter;
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
		}
		{
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
		}
		// update layer rendertargets descriptor sets
		for ( auto layer : layers ) {
			RenderTargetRenderMode* rtLayer = (RenderTargetRenderMode*) layer;
			auto& blitter = rtLayer->blitter;
			auto& renderTarget = rtLayer->renderTarget;
			// update descriptor set
			if ( blitter.initialized ) {
				VkDescriptorImageInfo samplerDescriptor; samplerDescriptor.sampler = blitter.sampler;
				std::vector<VkDescriptorImageInfo> colorDescriptors;
			/*
				colorDescriptors.push_back(ext::vulkan::initializers::descriptorImageInfo( 
					renderTarget.attachments[0].view,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
				));
			*/
		
				for ( auto& attachment : renderTarget.attachments ) {
					colorDescriptors.push_back(ext::vulkan::initializers::descriptorImageInfo( 
						attachment.view,
						VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
					));
					//break;
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
			/*
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
			*/
			}
		}
	}
}
void ext::vulkan::StereoscopicDeferredRenderMode::destroy() {
	ext::vulkan::RenderMode::destroy();
	renderTargets.right.destroy();
	blitters.left.destroy();
	blitters.right.destroy();
	blitter.destroy();
}
void ext::vulkan::StereoscopicDeferredRenderMode::createCommandBuffers( const std::vector<ext::vulkan::Graphic*>& graphics, const std::vector<std::string>& passes ) {
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
		struct EYES {
			RenderTarget* renderTarget;
			DeferredRenderingGraphic* blitter;
		};
		std::vector<EYES> eyes = {
			{ &renderTargets.left, &blitters.left },
			{ &renderTargets.right, &blitters.right },
		};
		for ( ext::openvr::renderPass = 0; ext::openvr::renderPass < eyes.size(); ++ext::openvr::renderPass ) {
			auto& renderTarget = *eyes[ext::openvr::renderPass].renderTarget;
			auto& blitter = *eyes[ext::openvr::renderPass].blitter;
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
					imageMemoryBarrier.image = renderTarget.attachments[0].image;
					imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
					imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_SHADER_READ_BIT;
					imageMemoryBarrier.oldLayout = renderTarget.attachments[0].layout;
					imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					vkCmdPipelineBarrier( commands[i], VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT , VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &imageMemoryBarrier );
					renderTarget.attachments[0].layout = imageMemoryBarrier.newLayout;
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
			/*
				vkCmdNextSubpass(commands[i], VK_SUBPASS_CONTENTS_INLINE);
				if ( ext::openvr::renderPass == DOMINANT_EYE ) {
					viewport.width = (float) ::ext::vulkan::width;
					viewport.height = (float) ::ext::vulkan::height;
					scissor.extent.width = ::ext::vulkan::width;
					scissor.extent.height = ::ext::vulkan::height;

					vkCmdSetViewport(commands[i], 0, 1, &viewport);
					vkCmdSetScissor(commands[i], 0, 1, &scissor);
					this->blitter.createCommandBuffer(commands[i]);
					// render gui layer
					{
						for ( auto layer : layers ) {
							if ( layer->getName() == "Gui" ) {
								RenderTargetRenderMode* guiLayer = (RenderTargetRenderMode*) layer;
								if ( guiLayer->blitter.subpass == 2 ) guiLayer->blitter.createCommandBuffer(commands[i]);
							}
						}
					}
				}
			*/
				vkCmdEndRenderPass(commands[i]);

				for ( auto layer : layers ) {
					if ( layer->getName() == "" ) continue;
					RenderTarget& renderTarget = layer->renderTarget;
					imageMemoryBarrier.image = renderTarget.attachments[0].image;
					imageMemoryBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_SHADER_READ_BIT;
					imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
					imageMemoryBarrier.oldLayout = renderTarget.attachments[0].layout;
					imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
					vkCmdPipelineBarrier( commands[i], VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT , 0, 0, NULL, 0, NULL, 1, &imageMemoryBarrier );
					renderTarget.attachments[0].layout = imageMemoryBarrier.newLayout;
				}
			}
		}
		// Blit eye to swapchain
		{
		//	if ( false ) {
			auto& swapchainRender = ext::vulkan::getRenderMode("Swapchain");
			{
				auto& renderTarget = swapchainRender.renderTarget;
				float width = renderTarget.width;
				float height = renderTarget.height;

				std::vector<VkClearValue> clearValues; clearValues.resize(2);
				clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
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
				vkCmdEndRenderPass(commands[i]);

			}
			{
				auto& renderTarget = DOMINANT_EYE == 0 ? renderTargets.left : renderTargets.right;
				VkImageBlit imageBlitRegion{};
				imageBlitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				imageBlitRegion.srcSubresource.layerCount = 1;
				imageBlitRegion.srcOffsets[1] = { width, height, 1 };
				imageBlitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				imageBlitRegion.dstSubresource.layerCount = 1;
				imageBlitRegion.dstOffsets[1] = {
					swapchainRender.width > 0 ? swapchainRender.width : ext::vulkan::width,
					swapchainRender.height > 0 ? swapchainRender.height : ext::vulkan::height,
					1
				};

				auto& outputAttachment = renderTarget.attachments[renderTarget.attachments.size()-1];
				// Transition to KHR
				{
					imageMemoryBarrier.image = outputAttachment.image;
					imageMemoryBarrier.srcAccessMask = 0;
					imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
					imageMemoryBarrier.oldLayout = outputAttachment.layout;
					imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
					vkCmdPipelineBarrier( commands[i], VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, NULL, 0, NULL, 1, &imageMemoryBarrier );
					outputAttachment.layout = imageMemoryBarrier.newLayout;
				}
				{
					imageMemoryBarrier.image = swapchainRender.renderTarget.attachments[i].image;
					imageMemoryBarrier.srcAccessMask = 0;
					imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
					imageMemoryBarrier.oldLayout = swapchainRender.renderTarget.attachments[i].layout;
					imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
					vkCmdPipelineBarrier( commands[i], VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, NULL, 0, NULL, 1, &imageMemoryBarrier );
					swapchainRender.renderTarget.attachments[i].layout = imageMemoryBarrier.newLayout;
				}

				vkCmdBlitImage(
					commands[i],
					outputAttachment.image,
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					swapchainRender.renderTarget.attachments[i].image,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					1,
					&imageBlitRegion,
					VK_FILTER_LINEAR
				);

				{
					imageMemoryBarrier.image = outputAttachment.image;
					imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
					imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
					imageMemoryBarrier.oldLayout = outputAttachment.layout;
					imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
					vkCmdPipelineBarrier( commands[i], VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, NULL, 0, NULL, 1, &imageMemoryBarrier );
					outputAttachment.layout = imageMemoryBarrier.newLayout;
				}
				{
					imageMemoryBarrier.image = swapchainRender.renderTarget.attachments[i].image;
					imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
					imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
					imageMemoryBarrier.oldLayout = swapchainRender.renderTarget.attachments[i].layout;
					imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
					vkCmdPipelineBarrier( commands[i], VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, NULL, 0, NULL, 1, &imageMemoryBarrier );
					swapchainRender.renderTarget.attachments[i].layout = imageMemoryBarrier.newLayout;
				}
			}
		}
		VK_CHECK_RESULT(vkEndCommandBuffer(commands[i]));
	}
	ext::openvr::renderPass = 0;
}