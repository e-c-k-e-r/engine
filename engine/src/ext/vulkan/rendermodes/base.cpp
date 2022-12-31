#if UF_USE_VULKAN

#include <uf/ext/vulkan/vulkan.h>
#include <uf/ext/vulkan/rendermodes/base.h>
#include <uf/ext/vulkan/initializers.h>
#include <uf/utils/window/window.h>
#include <uf/ext/vulkan/graphic.h>
#include <uf/ext/vulkan/rendertarget.h>
#include <uf/utils/graphic/graphic.h>

namespace {
	uf::stl::vector<VkImage> images;
}

const uf::stl::string ext::vulkan::BaseRenderMode::getType() const {
	return "Swapchain";
}
void ext::vulkan::BaseRenderMode::createCommandBuffers( const uf::stl::vector<ext::vulkan::Graphic*>& graphics ) {
//	if ( ext::vulkan::renderModes.size() > 1 ) return;

	auto windowSize = device->window->getSize();
	float width = windowSize.x; //this->width > 0 ? this->width : windowSize.x;
	float height = windowSize.y; //this->height > 0 ? this->height : windowSize.y;

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

	uf::stl::vector<RenderMode*> layers = ext::vulkan::getRenderModes(uf::stl::vector<uf::stl::string>{"Deferred", "Compute:RT", "RenderTarget"}, false);
#if 0
	if ( settings::pipelines::rt ) {
		std::reverse( layers.begin(), layers.end() );
	}
#endif

	auto& scene = uf::scene::getCurrentScene();
	auto& sceneMetadataJson = scene.getComponent<uf::Serializer>();
	auto& commands = getCommands();

	uf::stl::vector<VkClearValue> clearValues;
	for ( auto& attachment : renderTarget.attachments ) {
		pod::Vector4f clearColor = uf::vector::decode( sceneMetadataJson["system"]["renderer"]["clear values"][(int) clearValues.size()], pod::Vector4f{0, 0, 0, 0} );
		auto& clearValue = clearValues.emplace_back();
		if ( attachment.descriptor.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT ) {
			clearValue.color.float32[0] = clearColor[0];
			clearValue.color.float32[1] = clearColor[1];
			clearValue.color.float32[2] = clearColor[2];
			clearValue.color.float32[3] = clearColor[3];
		} else if ( attachment.descriptor.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT ) {
			if ( uf::matrix::reverseInfiniteProjection ) {
				clearValue.depthStencil = { 0.0f, 0 };
			} else {
				clearValue.depthStencil = { 1.0f, 0 };
			}
		}
	}

	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.pNext = nullptr;
	renderPassBeginInfo.renderPass = renderTarget.renderPass;
	renderPassBeginInfo.renderArea.offset.x = 0;
	renderPassBeginInfo.renderArea.offset.y = 0;
	renderPassBeginInfo.renderArea.extent.width = width;
	renderPassBeginInfo.renderArea.extent.height = height;
	renderPassBeginInfo.clearValueCount = clearValues.size();
	renderPassBeginInfo.pClearValues = &clearValues[0];

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
	
	for (size_t i = 0; i < commands.size(); ++i) {
		
		auto& commandBuffer = commands[i];
		renderPassBeginInfo.framebuffer = renderTarget.framebuffers[i];
		

		VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffer, &cmdBufInfo));
		device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::BEGIN, "begin" );
		{
			size_t currentSubpass = 0;

			{
				VkImageMemoryBarrier imageMemoryBarrier = {};
				imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				imageMemoryBarrier.srcAccessMask = 0;
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				imageMemoryBarrier.oldLayout = renderTarget.attachments[i].descriptor.layout;
				imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

				// explicitly transfer queue-ownership
				if ( ext::vulkan::device.queueFamilyIndices.graphics != ext::vulkan::device.queueFamilyIndices.present ) {
					imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // ext::vulkan::device.queueFamilyIndices.present;
					imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // ext::vulkan::device.queueFamilyIndices.graphics;
				} else {
					imageMemoryBarrier.srcQueueFamilyIndex = ext::vulkan::device.queueFamilyIndices.present;
					imageMemoryBarrier.dstQueueFamilyIndex = ext::vulkan::device.queueFamilyIndices.graphics;
				}
				imageMemoryBarrier.image = renderTarget.attachments[i].image;
				imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
				imageMemoryBarrier.subresourceRange.levelCount = 1;
				imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
				imageMemoryBarrier.subresourceRange.layerCount = 1;
				imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				renderTarget.attachments[i].descriptor.layout = imageMemoryBarrier.newLayout;

				device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::GENERIC, "setImageLayout" );
				vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
			}
			
			// pre-renderpass commands
			if ( commandBufferCallbacks.count(CALLBACK_BEGIN) > 0 ) {
				device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::GENERIC, "callback[begin]" );
				commandBufferCallbacks[CALLBACK_BEGIN]( commandBuffer, i );
			}

			device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::BEGIN, "renderPass[begin]" );
			vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
				vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
				vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
				// render to geometry buffers
				for ( size_t eye = 0; eye < metadata.eyes; ++eye ) {
					size_t currentPass = 0;
					size_t currentDraw = 0;

					// blit any RT's that request this subpass
					for ( auto _ : layers ) {
						RenderTargetRenderMode* layer = (RenderTargetRenderMode*) _;
						auto& blitter = layer->blitter;
						if ( !blitter.initialized || !blitter.process || blitter.descriptor.subpass != currentPass || blitter.descriptor.renderMode != this->getName() ) continue;
						ext::vulkan::GraphicDescriptor descriptor = blitter.descriptor; // bindGraphicDescriptor(blitter.descriptor, currentSubpass);
						device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::GENERIC, ::fmt::format("blitter[{}: {}]", layer->getName(), layer->getType()) );
						blitter.record(commandBuffer, descriptor);
					}
				}
			device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::END, "renderPass[end]" );
			vkCmdEndRenderPass(commandBuffer);

			// post-renderpass commands
			if ( commandBufferCallbacks.count(CALLBACK_END) > 0 ) {
				device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::GENERIC, "callback[end]" );
				commandBufferCallbacks[CALLBACK_END]( commandBuffer, i );
			}

			// need to transfer it back, if they differ
			if ( ext::vulkan::device.queueFamilyIndices.graphics != ext::vulkan::device.queueFamilyIndices.present ) {
				VkImageMemoryBarrier imageMemoryBarrier = {};
				imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
				imageMemoryBarrier.oldLayout = renderTarget.attachments[i].descriptor.layout;
				imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
				imageMemoryBarrier.srcQueueFamilyIndex = ext::vulkan::device.queueFamilyIndices.graphics;
				imageMemoryBarrier.dstQueueFamilyIndex = ext::vulkan::device.queueFamilyIndices.present;
				imageMemoryBarrier.image = renderTarget.attachments[i].image;

				imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
				imageMemoryBarrier.subresourceRange.levelCount = 1;
				imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
				imageMemoryBarrier.subresourceRange.layerCount = 1;
				imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				renderTarget.attachments[i].descriptor.layout = imageMemoryBarrier.newLayout;

				device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::GENERIC, "setImageLayout" );
				vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
			}
		}

		device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::END, "end" );
		VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));
	}
}

void ext::vulkan::BaseRenderMode::tick() {
	ext::vulkan::RenderMode::tick();
	
	if ( ext::vulkan::states::resized ) {
		this->destroy();
		this->initialize( *this->device );
	}
}
void ext::vulkan::BaseRenderMode::render() {
//	if ( ext::vulkan::states::frameSkip ) return;
//	if ( ext::vulkan::renderModes.size() > 1 ) return;
//	if ( ext::vulkan::renderModes.back() != this ) return;

	//lockMutex( this->mostRecentCommandPoolId );
	auto& commands = getCommands( this->mostRecentCommandPoolId );
	// Get next image in the swap chain (back/front buffer)
	VK_CHECK_RESULT(swapchain.acquireNextImage(&states::currentBuffer, swapchain.presentCompleteSemaphore));

	// Use a fence to wait until the command buffer has finished execution before using it again
	VK_CHECK_RESULT(vkWaitForFences(*device, 1, &fences[states::currentBuffer], VK_TRUE, VK_DEFAULT_FENCE_TIMEOUT));
	VK_CHECK_RESULT(vkResetFences(*device, 1, &fences[states::currentBuffer]));

	// Pipeline stage at which the queue submission will wait (via pWaitSemaphores)
	VkPipelineStageFlags waitStageMask[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	// The submit info structure specifices a command buffer queue submission batch
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pWaitDstStageMask = waitStageMask;									// Pointer to the list of pipeline stages that the semaphore waits will occur at
	submitInfo.pWaitSemaphores = &swapchain.presentCompleteSemaphore;				// Semaphore(s) to wait upon before the submitted command buffer starts executing
	submitInfo.waitSemaphoreCount = 1;												// One wait semaphore																				
	submitInfo.pSignalSemaphores = &renderCompleteSemaphore;						// Semaphore(s) to be signaled when command buffers have completed
	submitInfo.signalSemaphoreCount = 1;											// One signal semaphore
	submitInfo.pCommandBuffers = &commands[states::currentBuffer];					// Command buffers(s) to execute in this batch (submission)
	submitInfo.commandBufferCount = 1;

	// Submit to the graphics queue passing a wait fence
//	VK_CHECK_RESULT(vkQueueSubmit( device->getQueue( QueueEnum::GRAPHICS ), 1, &submitInfo, fences[states::currentBuffer]));	
	{
		VkQueue queue = device->getQueue( QueueEnum::GRAPHICS );
		VkResult res = vkQueueSubmit( queue, 1, &submitInfo, fences[states::currentBuffer]);
		VK_CHECK_QUEUE_CHECKPOINT( queue, res );
	}
	
	// Present the current buffer to the swap chain
	// Pass the semaphore signaled by the command buffer submission from the submit info as the wait semaphore for swap chain presentation
	// This ensures that the image is not presented to the windowing system until all commands have been submitted
	VK_CHECK_RESULT(swapchain.queuePresent(device->getQueue( QueueEnum::PRESENT ), states::currentBuffer, renderCompleteSemaphore));
//	VK_CHECK_RESULT(vkQueueWaitIdle(device->getQueue( QueueEnum::PRESENT )));
	{
		VkQueue queue = device->getQueue( QueueEnum::PRESENT );
		VkResult res = vkQueueWaitIdle(device->getQueue( QueueEnum::PRESENT ));
		VK_CHECK_QUEUE_CHECKPOINT( queue, res );
	}

	this->executed = true;

	//unlockMutex( this->mostRecentCommandPoolId );
}

void ext::vulkan::BaseRenderMode::initialize( Device& device ) {
	this->metadata.name = "Swapchain";
	auto windowSize = device.window->getSize();
	this->width = windowSize.x;
	this->height = windowSize.y;

	ext::vulkan::RenderMode::initialize( device );
	// manual initialization
	// recreate swapchain
	// destroy any existing imageviews
	// attachments marked as aliased are actually from the swapchain
	// swapchain.destroy();
	swapchain.initialize( device );
	// bind swapchain images
	images.resize( ext::vulkan::swapchain.buffers );
	VK_CHECK_RESULT(vkGetSwapchainImagesKHR( device, swapchain.swapChain, &swapchain.buffers, images.data()));
	// create image views for swapchain images
	
	renderTarget.attachments.clear();
	renderTarget.attachments.resize( ext::vulkan::swapchain.buffers );

//	uint32_t width = windowSize.x; //this->width > 0 ? this->width : windowSize.x;
//	uint32_t height = windowSize.y; //this->height > 0 ? this->height : windowSize.y;

	size_t attachmentIndex = 0;
	for ( size_t i = 0; i < ext::vulkan::swapchain.buffers; ++i ) {
		VkImageViewCreateInfo colorAttachmentView = {};
		colorAttachmentView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		colorAttachmentView.pNext = NULL;
		colorAttachmentView.format = ext::vulkan::settings::formats::color;
		colorAttachmentView.components = {
			VK_COMPONENT_SWIZZLE_R,
			VK_COMPONENT_SWIZZLE_G,
			VK_COMPONENT_SWIZZLE_B,
			VK_COMPONENT_SWIZZLE_A
		};
		colorAttachmentView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		colorAttachmentView.subresourceRange.baseMipLevel = 0;
		colorAttachmentView.subresourceRange.levelCount = 1;
		colorAttachmentView.subresourceRange.baseArrayLayer = 0;
		colorAttachmentView.subresourceRange.layerCount = 1;
		colorAttachmentView.viewType = VK_IMAGE_VIEW_TYPE_2D;
		colorAttachmentView.flags = 0;
		colorAttachmentView.image = images[i];

		VK_CHECK_RESULT(vkCreateImageView( device, &colorAttachmentView, nullptr, &renderTarget.attachments[i].view));

		renderTarget.attachments[i].descriptor.format = ext::vulkan::settings::formats::color;
	//	renderTarget.attachments[i].descriptor.layout = VK_IMAGE_LAYOUT_UNDEFINED;
		renderTarget.attachments[i].descriptor.layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		renderTarget.attachments[i].descriptor.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		renderTarget.attachments[i].descriptor.aliased = true;
		renderTarget.attachments[i].image = images[i];
		renderTarget.attachments[i].mem = VK_NULL_HANDLE;

		metadata.attachments["color["+std::to_string((int) i)+"]"] = attachmentIndex++;
	}
	{
		// Create depth
		auto& attachment = renderTarget.attachments.emplace_back();
		metadata.attachments["depth"] = attachmentIndex++;
		
		// Create an optimal image used as the depth stencil attachment
		VkImageCreateInfo imageCreateInfo = {};
		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.format = ext::vulkan::settings::formats::depth;
		// Use example's height and width
		imageCreateInfo.extent = { width, height, 1 };
		imageCreateInfo.mipLevels = 1;
		imageCreateInfo.arrayLayers = 1;
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	
		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		VK_CHECK_RESULT(vmaCreateImage(allocator, &imageCreateInfo, &allocInfo, &attachment.image, &attachment.allocation, &attachment.allocationInfo));
		

		attachment.descriptor.format = ext::vulkan::settings::formats::depth;
	//	attachment.descriptor.layout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachment.descriptor.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		attachment.descriptor.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		attachment.descriptor.aliased = true;
		attachment.mem = attachment.allocationInfo.deviceMemory;
	
		VkImageViewCreateInfo depthStencilView = {};
		depthStencilView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D;
		depthStencilView.format = ext::vulkan::settings::formats::depth;
		depthStencilView.subresourceRange = {};
		depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		depthStencilView.subresourceRange.baseMipLevel = 0;
		depthStencilView.subresourceRange.levelCount = 1;
		depthStencilView.subresourceRange.baseArrayLayer = 0;
		depthStencilView.subresourceRange.layerCount = 1;
		depthStencilView.image = attachment.image;

		VK_CHECK_RESULT(vkCreateImageView(device, &depthStencilView, nullptr, &attachment.view));
	}

#if 1
	auto commandBuffer = device.fetchCommandBuffer(uf::renderer::QueueEnum::GRAPHICS);
	for ( auto& attachment : renderTarget.attachments ) {
		bool isDepth = attachment.descriptor.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

		if ( isDepth ) {
			// transition attachments to general attachments for imageStore
			VkImageSubresourceRange subresourceRange;
			subresourceRange.baseMipLevel = 0;
			subresourceRange.levelCount = 1;
			subresourceRange.baseArrayLayer = 0;
			subresourceRange.layerCount = 1;
			subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			subresourceRange.layerCount = 1;

			uf::renderer::Texture::setImageLayout( commandBuffer, attachment.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, subresourceRange );
		} else {
			// transition attachments to general attachments for imageStore
			VkImageSubresourceRange subresourceRange;
			subresourceRange.baseMipLevel = 0;
			subresourceRange.levelCount = 1;
			subresourceRange.baseArrayLayer = 0;
			subresourceRange.layerCount = 1;
			subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			subresourceRange.layerCount = 1;

			uf::renderer::Texture::setImageLayout( commandBuffer, attachment.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, subresourceRange );
		}
	}
	device.flushCommandBuffer(commandBuffer, uf::renderer::QueueEnum::GRAPHICS);
#endif

	// Create FSR dump
/*
	if ( settings::pipelines::fsr ) {
		auto& attachment = renderTarget.attachments.emplace_back();

		VkImageCreateInfo imageCreateInfo = {};
		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.format = ext::vulkan::settings::pipelines::hdr ? enums::Format::HDR : enums::Format::SDR;

		imageCreateInfo.extent = { width, height, 1 };
		imageCreateInfo.mipLevels = 1;
		imageCreateInfo.arrayLayers = 1;
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCreateInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	
		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		VK_CHECK_RESULT(vmaCreateImage(allocator, &imageCreateInfo, &allocInfo, &attachment.image, &attachment.allocation, &attachment.allocationInfo));
		attachment.mem = attachment.allocationInfo.deviceMemory;

		VkImageViewCreateInfo imageViewCreateInfo = {};
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCreateInfo.format = ext::vulkan::settings::pipelines::hdr ? enums::Format::HDR : enums::Format::SDR;
		imageViewCreateInfo.subresourceRange = {};
		imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.layerCount = 1;
		imageViewCreateInfo.image = attachment.image;

		VK_CHECK_RESULT(vkCreateImageView(device, &imageViewCreateInfo, nullptr, &attachment.view));
		metadata.attachments["fsr"] = attachmentIndex++;
	}
*/
	// Create renderpass
	if ( !renderTarget.renderPass ) {// Create render pass
		// This example will use a single render pass with one subpass

		// Descriptors for the attachments used by this renderpass
		std::array<VkAttachmentDescription, 2> attachments = {};

		// Color attachment
		attachments[0].format = ext::vulkan::settings::formats::color;					// Use the color format selected by the swapchain
		attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;									// We don't use multi sampling in this example
		attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;							// Clear this attachment at the start of the render pass
		attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;							// Keep it's contents after the render pass is finished (for displaying it)
		attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;					// We don't use stencil, so don't care for load
		attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;				// Same for store
		attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;						// Layout at render pass start. Initial doesn't matter, so we use undefined
		attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;					// Layout to which the attachment is transitioned when the render pass is finished
																						// As we want to present the color buffer to the swapchain, we transition to PRESENT_KHR	
		// Depth attachment
		attachments[1].format = ext::vulkan::settings::formats::depth;					// A proper depth format is selected in the example base
		attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;						
		attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;							// Clear depth at start of first subpass
		attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;						// We don't need depth after render pass has finished (DONT_CARE may result in better performance)
		attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;					// No stencil
		attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;				// No Stencil
		attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;						// Layout at render pass start. Initial doesn't matter, so we use undefined
		attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;	// Transition to depth/stencil attachment

		// Setup attachment references
		VkAttachmentReference colorReference = {};
		colorReference.attachment = 0;													// Attachment 0 is color
		colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;				// Attachment layout used as color during the subpass

		VkAttachmentReference depthReference = {};
		depthReference.attachment = 1;													// Attachment 1 is color
		depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;		// Attachment used as depth/stemcil used during the subpass

		// Setup a single subpass reference
		VkSubpassDescription subpassDescription = {};
		subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;			
		subpassDescription.colorAttachmentCount = 1;									// Subpass uses one color attachment
		subpassDescription.pColorAttachments = &colorReference;							// Reference to the color attachment in slot 0
		subpassDescription.pDepthStencilAttachment = &depthReference;					// Reference to the depth attachment in slot 1
		subpassDescription.inputAttachmentCount = 0;									// Input attachments can be used to sample from contents of a previous subpass
		subpassDescription.pInputAttachments = nullptr;									// (Input attachments not used by this example)
		subpassDescription.preserveAttachmentCount = 0;									// Preserved attachments can be used to loop (and preserve) attachments through subpasses
		subpassDescription.pPreserveAttachments = nullptr;								// (Preserve attachments not used by this example)
		subpassDescription.pResolveAttachments = nullptr;								// Resolve attachments are resolved at the end of a sub pass and can be used for e.g. multi sampling

		// Setup subpass dependencies
		// These will add the implicit ttachment layout transitionss specified by the attachment descriptions
		// The actual usage layout is preserved through the layout specified in the attachment reference		
		// Each subpass dependency will introduce a memory and execution dependency between the source and dest subpass described by
		// srcStageMask, dstStageMask, srcAccessMask, dstAccessMask (and dependencyFlags is set)
		// Note: VK_SUBPASS_EXTERNAL is a special constant that refers to all commands executed outside of the actual renderpass)
		std::array<VkSubpassDependency, 2> dependencies;

		// First dependency at the start of the renderpass
		// Does the transition from final to initial layout 
		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;								// Producer of the dependency 
		dependencies[0].dstSubpass = 0;													// Consumer is our single subpass that will wait for the execution depdendency
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;			
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;	
		dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		// Second dependency at the end the renderpass
		// Does the transition from the initial to the final layout
		dependencies[1].srcSubpass = 0;													// Producer of the dependency is our single subpass
		dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;								// Consumer are all commands outside of the renderpass
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;	
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		// Create the actual renderpass
		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());		// Number of attachments used by this render pass
		renderPassInfo.pAttachments = attachments.data();								// Descriptions of the attachments used by the render pass
		renderPassInfo.subpassCount = 1;												// We only use one subpass in this example
		renderPassInfo.pSubpasses = &subpassDescription;								// Description of that subpass
		renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());	// Number of subpass dependencies
		renderPassInfo.pDependencies = dependencies.data();								// Subpass dependencies used by the render pass

		VK_CHECK_RESULT(vkCreateRenderPass( device, &renderPassInfo, nullptr, &renderTarget.renderPass));
	}
	// Create framebuffer
	{	
		// Create a frame buffer for every image in the swapchain
		renderTarget.framebuffers.resize(images.size());
		for (size_t i = 0; i < renderTarget.framebuffers.size(); i++)
		{
			std::array<VkImageView, 2> attachments;										
			attachments[0] = renderTarget.attachments[i].view;									// Color attachment is the view of the swapchain image			
			attachments[1] = renderTarget.attachments[metadata.attachments["depth"]].view;		// Depth/Stencil attachment is the same for all frame buffers			

			VkFramebufferCreateInfo frameBufferCreateInfo = {};
			frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			// All frame buffers use the same renderpass setup
			frameBufferCreateInfo.renderPass = renderTarget.renderPass;
			frameBufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			frameBufferCreateInfo.pAttachments = attachments.data();
			frameBufferCreateInfo.width = width;
			frameBufferCreateInfo.height = height;
			frameBufferCreateInfo.layers = 1;
			// Create the framebuffer
			VK_CHECK_RESULT(vkCreateFramebuffer( device, &frameBufferCreateInfo, nullptr, &renderTarget.framebuffers[i]));
		}
	
	}
#if 0
	if ( true ) {
		auto commandBuffer = device.fetchCommandBuffer(uf::renderer::QueueEnum::TRANSFER);
		for ( size_t i = 0; i < images.size(); ++i ) {
			VkImageMemoryBarrier imageMemoryBarrier = {};
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.srcAccessMask = 0;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			imageMemoryBarrier.oldLayout = renderTarget.attachments[i].descriptor.layout;
			imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

			imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // ext::vulkan::device.queueFamilyIndices.present;
			imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // ext::vulkan::device.queueFamilyIndices.graphics;

			imageMemoryBarrier.image = renderTarget.attachments[i].image;
			imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
			imageMemoryBarrier.subresourceRange.levelCount = 1;
			imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
			imageMemoryBarrier.subresourceRange.layerCount = 1;
			imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			renderTarget.attachments[i].descriptor.layout = imageMemoryBarrier.newLayout;

			vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
		}
		device.flushCommandBuffer(uf::renderer::QueueEnum::TRANSFER);
	}
#endif
/*
	{
		renderTarget.device = &device;
		// attach targets
		struct {
			size_t color, depth;
		} attachments;

		attachments.color = 0; {

		}
		attachments.depth = renderTarget.attach( ext::vulkan::settings::formats::depth, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ); // depth

		// First pass: fill the G-Buffer
		{
			renderTarget.addPass(
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				{ attachments.color },
				{},
				attachments.depth
			);
		}
	}
	renderTarget.initialize( device );
*/
	// Set sync objects
	{
		// Semaphores (Used for correct command ordering)
		VkSemaphoreCreateInfo semaphoreCreateInfo = {};
		semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		semaphoreCreateInfo.pNext = nullptr;

		// Semaphore used to ensures that image presentation is complete before starting to submit again
		VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &swapchain.presentCompleteSemaphore));
	}
}

void ext::vulkan::BaseRenderMode::destroy() {
	if ( renderTarget.renderPass != VK_NULL_HANDLE ) {
		vkDestroyRenderPass( *device, renderTarget.renderPass, nullptr );
		renderTarget.renderPass = VK_NULL_HANDLE;
	}
	
	for ( uint32_t i = 0; i < renderTarget.framebuffers.size(); i++ ) {
		if ( renderTarget.framebuffers[i] != VK_NULL_HANDLE ) {
			vkDestroyFramebuffer( *device, renderTarget.framebuffers[i], nullptr );
			renderTarget.framebuffers[i] = VK_NULL_HANDLE;
		}
	}
	for ( auto& attachment : renderTarget.attachments ) {
		if ( attachment.view != VK_NULL_HANDLE ) {
			vkDestroyImageView( *device, attachment.view, nullptr);
			attachment.view = VK_NULL_HANDLE;
		}
		if ( attachment.descriptor.aliased ) continue;
		if ( attachment.image != VK_NULL_HANDLE ) {
		//	vkDestroyImage( *device, attachment.image, nullptr );
			vmaDestroyImage( allocator, attachment.image, attachment.allocation );
			attachment.image = VK_NULL_HANDLE;
		}
		if ( attachment.mem != VK_NULL_HANDLE ) {
		//	vkFreeMemory( *device, attachment.mem, nullptr );
			attachment.mem = VK_NULL_HANDLE;
		}
	}
	
	ext::vulkan::RenderMode::destroy();

	if ( swapchain.presentCompleteSemaphore != VK_NULL_HANDLE ) {
		vkDestroySemaphore( *device, swapchain.presentCompleteSemaphore, nullptr);
	}
}

ext::vulkan::GraphicDescriptor ext::vulkan::BaseRenderMode::bindGraphicDescriptor( const ext::vulkan::GraphicDescriptor& reference, size_t pass ) {
	ext::vulkan::GraphicDescriptor descriptor = ext::vulkan::RenderMode::bindGraphicDescriptor(reference, pass);
/*
	descriptor.parse(metadata.json["descriptor"]);
	// invalidate
	if ( metadata.target != "" && descriptor.renderMode != this->getName() && descriptor.renderMode != metadata.target ) {
		descriptor.invalidated = true;
	} else {
		descriptor.renderMode = this->getName();
	}
*/

	descriptor.depth.test = false;
	descriptor.depth.write = false;
	return descriptor;
}

#endif