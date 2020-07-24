#include <uf/ext/vulkan/vulkan.h>
#include <uf/ext/vulkan/rendertarget.h>
#include <uf/ext/vulkan/initializers.h>
#include <uf/utils/window/window.h>

namespace {
	bool USEVR = false;
}

void ext::vulkan::RenderTarget::addPass( VkPipelineStageFlags stage, VkAccessFlags access, const std::vector<size_t>& colors, const std::vector<size_t>& inputs, size_t depth ) {
	Subpass pass;
	pass.stage = stage;
	pass.access = access;
	for ( auto& i : colors ) pass.colors.push_back( { i, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL } );
	for ( auto& i : inputs ) pass.inputs.push_back( { i, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL } );
	if ( depth < attachments.size() ) pass.depth = { depth, attachments[depth].layout };
	passes.push_back(pass);
}
size_t ext::vulkan::RenderTarget::attach( VkFormat format, VkImageUsageFlags usage, VkImageLayout layout, Attachment* attachment ) {
	if ( width == 0 ) width = ext::vulkan::width;
	if ( height == 0 ) height = ext::vulkan::height;

	if ( !attachment ) {
		attachments.resize(attachments.size()+1);
		attachment = &attachments.back();
	} else {
		if ( attachment->view ) {
			vkDestroyImageView(*device, attachment->view, nullptr);
			attachment->view = VK_NULL_HANDLE;
		}
		if ( attachment->image ) {
			vkDestroyImage(*device, attachment->image, nullptr);
			attachment->image = VK_NULL_HANDLE;
		}
		if ( attachment->mem ) {
			vkFreeMemory(*device, attachment->mem, nullptr);
			attachment->mem = VK_NULL_HANDLE;
		}
	}

	VkImageAspectFlags aspectMask = 0;
	// specialization: depth buffer
	if ( usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT ) {
		aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT; // | VK_IMAGE_ASPECT_STENCIL_BIT;
	} else {
		aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	attachment->layout = layout;
	attachment->format = format;
	attachment->usage = usage;

	VkImageCreateInfo image = {};
	image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image.imageType = VK_IMAGE_TYPE_2D;
	image.format = format;
	image.extent = { width, height, 1 };
	image.mipLevels = 1;
	image.arrayLayers = USEVR ? 2 : 1;
	image.samples = VK_SAMPLE_COUNT_1_BIT;
	image.tiling = VK_IMAGE_TILING_OPTIMAL;
	image.usage = usage;
	image.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	VK_CHECK_RESULT(vkCreateImage(*device, &image, nullptr, &attachment->image));

	// Allocate memory for the image (device local) and bind it to our image
	VkMemoryAllocateInfo memAlloc = {};
	memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	VkMemoryRequirements memReqs;
	vkGetImageMemoryRequirements(*device, attachment->image, &memReqs);
	memAlloc.allocationSize = memReqs.size;
	memAlloc.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK_RESULT(vkAllocateMemory(*device, &memAlloc, nullptr, &attachment->mem));
	VK_CHECK_RESULT(vkBindImageMemory(*device, attachment->image, attachment->mem, 0));

	// Create a view for the depth stencil image
	// Images aren't directly accessed in Vulkan, but rather through views described by a subresource range
	// This allows for multiple views of one image with differing ranges (e.g. for different layers)
	VkImageViewCreateInfo imageView = {};
	imageView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageView.format = format;
	imageView.subresourceRange = {};
	imageView.subresourceRange.aspectMask = aspectMask;
	imageView.subresourceRange.baseMipLevel = 0;
	imageView.subresourceRange.levelCount = 1;
	imageView.subresourceRange.baseArrayLayer = 0;
	imageView.subresourceRange.layerCount = USEVR ? 2 : 1;
	imageView.image = attachment->image;

	VK_CHECK_RESULT(vkCreateImageView(*device, &imageView, nullptr, &attachment->view));

	return attachments.size()-1;
}
void ext::vulkan::RenderTarget::initialize( Device& device ) {
	// Bind
	{
		this->device = &device;
		if ( width == 0 ) width = ext::vulkan::width;
		if ( height == 0 ) height = ext::vulkan::height;
	}
	// resize attachments if necessary
	if ( initialized ) {
		for ( auto& attachment: this->attachments ) {
			if ( attachment.aliased ) continue;
			attach( attachment.format, attachment.usage, attachment.layout, &attachment );
		}
	}
	// ensure attachments are already created
	assert( this->attachments.size() > 0 );
	// Create render pass
	if ( !renderPass ) {
		std::vector<VkAttachmentDescription> attachments; attachments.reserve( this->attachments.size() );
		
		for ( auto& attachment : this->attachments ) {
			VkAttachmentDescription description;
			description.format = attachment.format;
			description.samples = VK_SAMPLE_COUNT_1_BIT;
			description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			description.finalLayout = attachment.layout;
			description.flags = 0;

			attachments.push_back(description);
		}

		// ensure that the subpasses are already described
		assert( passes.size() > 0 );

		std::vector<VkSubpassDescription> descriptions;
		std::vector<VkSubpassDependency> dependencies;

		// dependency: transition from final
		VkSubpassDependency dependency;
		dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		dependency.dstSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependency.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		size_t i = 0;
		for ( auto& pass : passes ) {
			VkSubpassDescription description;

			// describe renderpass
			description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;			
			description.colorAttachmentCount = pass.colors.size();
			description.pColorAttachments = &pass.colors[0];
			description.pDepthStencilAttachment = &pass.depth;
			description.inputAttachmentCount = pass.inputs.size();
			description.pInputAttachments = &pass.inputs[0];
			description.preserveAttachmentCount = 0;
			description.pPreserveAttachments = nullptr;
			description.pResolveAttachments = nullptr;
			description.flags = 0;
			descriptions.push_back(description);

			// transition dependency between subpasses
			dependency.srcSubpass = dependency.dstSubpass;
			dependency.srcStageMask = dependency.dstStageMask;
			dependency.srcAccessMask = dependency.dstAccessMask;
			dependency.dstSubpass = i++;
			dependency.dstStageMask = pass.stage;
			dependency.dstAccessMask = pass.access;
			dependencies.push_back(dependency);
		}
		// dependency: transition to final
		{
			dependency.srcSubpass = dependency.dstSubpass;
			dependency.srcStageMask = dependency.dstStageMask;
			dependency.srcAccessMask = dependency.dstAccessMask;
			dependency.dstSubpass = VK_SUBPASS_EXTERNAL;
			dependency.dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			dependency.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			dependencies.push_back(dependency);
		}

		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.flags = 0;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderPassInfo.pAttachments = &attachments[0];
		renderPassInfo.subpassCount = static_cast<uint32_t>(descriptions.size());
		renderPassInfo.pSubpasses = &descriptions[0];
		renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
		renderPassInfo.pDependencies = &dependencies[0];

		VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));
	}

	{	
		// destroy previous framebuffers
		for ( auto& framebuffer : framebuffers ) vkDestroyFramebuffer( device, framebuffer, nullptr );

		RenderMode& base = ext::vulkan::getRenderMode( "Swapchain", false );
		framebuffers.resize(ext::vulkan::swapchain.buffers);
		for ( size_t i = 0; i < framebuffers.size(); ++i ) {
			std::vector<VkImageView> attachments;										
			for ( auto& attachment : this->attachments ) {
				if ( attachment.aliased && attachment.layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR ) {
					attachments.push_back(base.renderTarget.attachments[i].view);
				} else attachments.push_back(attachment.view);
			}

			VkFramebufferCreateInfo frameBufferCreateInfo = {};
			frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			// All frame buffers use the same renderpass setup
			frameBufferCreateInfo.renderPass = renderPass;
			frameBufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			frameBufferCreateInfo.pAttachments = &attachments[0];
			frameBufferCreateInfo.width = width;
			frameBufferCreateInfo.height = height;
			frameBufferCreateInfo.layers = 1;
			// Create the framebuffer
			VK_CHECK_RESULT(vkCreateFramebuffer( device, &frameBufferCreateInfo, nullptr, &framebuffers[i]));
		}
	}
	initialized = true;
}

void ext::vulkan::RenderTarget::destroy() {
	if ( !device ) return;
	vkDestroyRenderPass( *device, renderPass, nullptr );
	for ( auto& framebuffer : framebuffers ) vkDestroyFramebuffer( *device, framebuffer, nullptr );
	
	for ( auto& attachment : attachments ) {
		if ( attachment.aliased ) continue;
		vkDestroyImageView( *device, attachment.view, nullptr );
		attachment.view = VK_NULL_HANDLE;
		vkDestroyImage( *device, attachment.image, nullptr );
		attachment.image = VK_NULL_HANDLE;
		vkFreeMemory( *device, attachment.mem, nullptr );
		attachment.mem = VK_NULL_HANDLE;
	}

	renderPass = VK_NULL_HANDLE;
	device = VK_NULL_HANDLE;
}