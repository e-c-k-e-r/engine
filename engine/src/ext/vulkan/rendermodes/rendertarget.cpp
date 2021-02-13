#if UF_USE_VULKAN

#include <uf/ext/vulkan/vulkan.h>
#include <uf/ext/vulkan/rendermodes/rendertarget.h>
#include <uf/ext/vulkan/initializers.h>
#include <uf/utils/window/window.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/ext/vulkan/graphic.h>

const std::string ext::vulkan::RenderTargetRenderMode::getTarget() const {
	auto& metadata = *const_cast<uf::Serializer*>(&this->metadata);
	return metadata["target"].as<std::string>();
}
void ext::vulkan::RenderTargetRenderMode::setTarget( const std::string& target ) {
	this->metadata["target"] = target;
}

const std::string ext::vulkan::RenderTargetRenderMode::getType() const {
	return "RenderTarget";
}
const size_t ext::vulkan::RenderTargetRenderMode::blitters() const {
	return 1;
}
ext::vulkan::Graphic* ext::vulkan::RenderTargetRenderMode::getBlitter( size_t i ) {
	return &this->blitter;
}
std::vector<ext::vulkan::Graphic*> ext::vulkan::RenderTargetRenderMode::getBlitters() {
	return { &this->blitter };
}

ext::vulkan::GraphicDescriptor ext::vulkan::RenderTargetRenderMode::bindGraphicDescriptor( const ext::vulkan::GraphicDescriptor& reference, size_t pass ) {
	ext::vulkan::GraphicDescriptor descriptor = ext::vulkan::RenderMode::bindGraphicDescriptor(reference, pass);
	descriptor.parse(metadata["descriptor"]);
	std::string type = metadata["type"].as<std::string>();
	if ( type == "depth" ) {
		descriptor.cullMode = VK_CULL_MODE_NONE;
	}
	// allows for "easy" remapping for indices ranges
	if ( ext::json::isObject( metadata["renderMode"]["target"][std::to_string(descriptor.indices)] ) ) {
		auto& map = metadata["renderMode"]["target"][std::to_string(descriptor.indices)];
		std::cout << "Replacing " << descriptor.indices;
		descriptor.indices = map["size"].as<size_t>();
		descriptor.offsets.index = map["offset"].as<size_t>();
		std::cout << " with " << descriptor.offsets.index << " + " << descriptor.indices << std::endl;
	}
	return descriptor;
}

void ext::vulkan::RenderTargetRenderMode::initialize( Device& device ) {
	ext::vulkan::RenderMode::initialize( device );

	this->setTarget( this->getName() );
	std::string type = metadata["type"].as<std::string>();
	size_t subpasses = metadata["subpasses"].as<size_t>();
	if ( subpasses == 0 ) subpasses = 1;
	renderTarget.device = &device;
	for ( size_t currentPass = 0; currentPass < subpasses; ++currentPass ) {
		if ( type == "depth" ) {
			struct {
				size_t depth;
			} attachments;

			attachments.depth = renderTarget.attach(RenderTarget::Attachment::Descriptor{
				/*.format = */ ext::vulkan::settings::formats::depth,
				/*.layout = */ VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
				/*.usage = */ VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				/*.blend = */ false,
				/*.samples = */ 1,
			});
			renderTarget.addPass(
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				{},
				{},
				{},
				attachments.depth,
				true
			);
		} else {
		#if 0
			struct {
				size_t albedo, normals, position, depth;
			} attachments;

			attachments.albedo = renderTarget.attach(RenderTarget::Attachment::Descriptor{
				/*.format = */ ext::vulkan::settings::formats::color,
				/*.layout = */ VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				/*.usage = */ VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				/*.blend = */ true,
				/*.samples = */ 1,
			});
			attachments.normals = renderTarget.attach(RenderTarget::Attachment::Descriptor{
				/*.format = */ ext::vulkan::settings::formats::normal,
				/*.layout = */ VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				/*.usage = */ VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				/*.blend = */ false,
				/*.samples = */ 1,
			});

			if ( !settings::experimental::deferredReconstructPosition )
				attachments.position = renderTarget.attach(RenderTarget::Attachment::Descriptor{
					/*.format = */ ext::vulkan::settings::formats::position,
					/*.layout = */ VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					/*.usage = */ VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
					/*.blend = */ false,
					/*.samples = */ 1,
				});

			attachments.depth = renderTarget.attach(RenderTarget::Attachment::Descriptor{
				/*.format = */ ext::vulkan::settings::formats::depth,
				/*.layout = */ VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
				/*.usage = */ VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				/*.blend = */ false,
				/*.samples = */ 1,
			});

			// First pass: write to target
			if ( settings::experimental::deferredReconstructPosition ) {
				renderTarget.addPass(
					VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
					{ attachments.albedo, attachments.normals },
					{},
					{},
					attachments.depth
				);
			} else {
				renderTarget.addPass(
					VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
					{ attachments.albedo, attachments.normals, attachments.position },
					{},
					{},
					attachments.depth
				);
			}
		#else
			struct {
				size_t id, normals, uvs, albedo, depth, output;
			} attachments;
			size_t msaa = ext::vulkan::settings::msaa;

			attachments.id = renderTarget.attach(RenderTarget::Attachment::Descriptor{
				/*.format = */VK_FORMAT_R16G16_UINT,
				/*.layout = */VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				/*.usage = */VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
				/*.blend = */false,
				/*.samples = */msaa,
			});
			attachments.normals = renderTarget.attach(RenderTarget::Attachment::Descriptor{
				/*.format = */VK_FORMAT_R16G16_SFLOAT,
				/*.layout = */VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				/*.usage = */VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
				/*.blend = */false,
				/*.samples = */msaa,
			});
			if ( ext::vulkan::settings::experimental::deferredMode == "deferredSampling" ) {
				attachments.uvs = renderTarget.attach(RenderTarget::Attachment::Descriptor{
					/*.format = */VK_FORMAT_R16G16_UNORM,
					/*.layout = */VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					/*.usage = */VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
					/*.blend = */false,
					/*.samples = */msaa,
				});
			} else {
				attachments.albedo = renderTarget.attach(RenderTarget::Attachment::Descriptor{
					/*.format = */VK_FORMAT_R8G8B8A8_UNORM,
					/*.layout = */VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					/*.usage = */VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
					/*.blend = */true,
					/*.samples = */msaa,
				});
			}
			attachments.depth = renderTarget.attach(RenderTarget::Attachment::Descriptor{
				/*.format = */ext::vulkan::settings::formats::depth,
				/*.layout = */VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
				/*.usage = */VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
				/*.blend = */false,
				/*.samples = */msaa,
			});
			attachments.output = renderTarget.attach(RenderTarget::Attachment::Descriptor{
				/*.format =*/ VK_FORMAT_R8G8B8A8_UNORM,
				/*.layout = */ VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				/*.usage =*/ VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
				/*.blend =*/ true,
				/*.samples =*/ 1,
			});
			if ( ext::vulkan::settings::experimental::deferredMode == "deferredSampling" ) {
				// First pass: fill the G-Buffer
				{
					renderTarget.addPass(
						VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
						{ attachments.id, attachments.normals, attachments.uvs },
						{},
						{},
						attachments.depth,
						true
					);
				}
				// Second pass: write to output
				{
					renderTarget.addPass(
						VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,
						{ attachments.output },
						{ attachments.id, attachments.normals, attachments.uvs, attachments.depth },
						{},
						attachments.depth,
						false
					);
				}
			} else {
				// First pass: fill the G-Buffer
				{
					renderTarget.addPass(
						VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
						{ attachments.id, attachments.normals, attachments.albedo },
						{},
						{},
						attachments.depth,
						true
					);
				}
				// Second pass: write to output
				{
					renderTarget.addPass(
						VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,
						{ attachments.output },
						{ attachments.id, attachments.normals, attachments.albedo, attachments.depth },
						{},
						attachments.depth,
						false
					);
				}
			}
			metadata["outputs"][0] = attachments.output;
		#endif
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
			if ( !(attachment.descriptor.usage & VK_IMAGE_USAGE_SAMPLED_BIT) ) continue;

			Texture2D& texture = blitter.material.textures.emplace_back();
			texture.aliasAttachment(attachment);
		}
	}
}

void ext::vulkan::RenderTargetRenderMode::tick() {
	ext::vulkan::RenderMode::tick();
	if ( ext::vulkan::states::resized ) {
		renderTarget.initialize( *renderTarget.device );
		blitter.material.textures.clear();
		for ( auto& attachment : renderTarget.attachments ) {
			if ( !(attachment.descriptor.usage & VK_IMAGE_USAGE_SAMPLED_BIT) ) continue;
			Texture2D& texture = blitter.material.textures.emplace_back();
			texture.aliasAttachment(attachment);
		}
		if ( blitter.process ) {
			blitter.getPipeline().update( blitter );
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
	submitInfo.pWaitDstStageMask = &waitStageMask;						// Pointer to the list of pipeline stages that the semaphore waits will occur at
	submitInfo.pWaitSemaphores = NULL;									// Semaphore(s) to wait upon before the submitted command buffer starts executing
	submitInfo.waitSemaphoreCount = 0;									// One wait semaphore																				
//	submitInfo.pSignalSemaphores = &renderCompleteSemaphore;			// Semaphore(s) to be signaled when command buffers have completed
//	submitInfo.signalSemaphoreCount = 1;								// One signal semaphore
	submitInfo.pSignalSemaphores = NULL;								// Semaphore(s) to be signaled when command buffers have completed
	submitInfo.signalSemaphoreCount = 0;								// One signal semaphore
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
		if ( !(attachment.descriptor.usage & VK_IMAGE_USAGE_SAMPLED_BIT) ) continue;
		if (  (attachment.descriptor.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) ) continue;
		
		VkPipelineStageFlags srcStageMask, dstStageMask;
		imageMemoryBarrier.image = attachment.image;
		imageMemoryBarrier.oldLayout = attachment.descriptor.layout;
		imageMemoryBarrier.newLayout = attachment.descriptor.layout;
	
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
				imageMemoryBarrier.newLayout = attachment.descriptor.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
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

		attachment.descriptor.layout = imageMemoryBarrier.newLayout;
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
				if ( attachment.descriptor.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT ) {
					clearValue.color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
				} else if ( attachment.descriptor.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT ) {
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

			size_t subpasses = renderTarget.passes.size();
			size_t currentPass = 0;
			vkCmdBeginRenderPass(commands[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
				vkCmdSetViewport(commands[i], 0, 1, &viewport);
				vkCmdSetScissor(commands[i], 0, 1, &scissor);
				for ( ; currentPass < subpasses; ++currentPass ) {
					size_t currentDraw = 0;
					for ( auto graphic : graphics ) {
						if ( graphic->descriptor.renderMode != this->getTarget() ) continue;
						ext::vulkan::GraphicDescriptor descriptor = bindGraphicDescriptor(graphic->descriptor, currentPass);
						graphic->record( commands[i], descriptor, currentPass, currentDraw++ );
					}
					if ( currentPass + 1 < subpasses ) vkCmdNextSubpass(commands[i], VK_SUBPASS_CONTENTS_INLINE);
				}
			vkCmdEndRenderPass(commands[i]);
		}

		VK_CHECK_RESULT(vkEndCommandBuffer(commands[i]));
	}
}

#endif