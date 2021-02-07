#include <uf/ext/vulkan/vulkan.h>
#include <uf/ext/vulkan/rendertarget.h>
#include <uf/ext/vulkan/initializers.h>
#include <uf/utils/window/window.h>

void ext::vulkan::RenderTarget::addPass( VkPipelineStageFlags stage, VkAccessFlags access, const std::vector<size_t>& colors, const std::vector<size_t>& inputs, const std::vector<size_t>& resolves, size_t depth ) {
	Subpass pass;
	pass.stage = stage;
	pass.access = access;
	for ( auto& i : colors )  pass.colors.push_back(  { (uint32_t) i, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL } );
	for ( auto& i : inputs )  pass.inputs.push_back(  { (uint32_t) i, i == depth ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL  : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL } );
	for ( auto& i : resolves ) pass.resolves.push_back( { (uint32_t) i, i == depth ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL  : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL } );
//	for ( auto& i : inputs )  pass.inputs.push_back(  { (uint32_t) i, i == depth ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL } );
	if ( depth < attachments.size() ) pass.depth = { (uint32_t) depth, attachments[depth].descriptor.layout };

	if ( !resolves.empty() && resolves.size() != colors.size() )
		VK_VALIDATION_MESSAGE("Mismatching resolves count: Expecting " << colors.size() << ", got " << resolves.size());

	passes.push_back(pass);
}
size_t ext::vulkan::RenderTarget::attach( const Attachment::Descriptor& descriptor, Attachment* attachment ) {
	uint32_t width = this->width > 0 ? this->width : ext::vulkan::settings::width;
	uint32_t height = this->height > 0 ? this->height : ext::vulkan::settings::height;

	if ( attachment ) {
		if ( attachment->view ) {
			vkDestroyImageView(*device, attachment->view, nullptr);
			attachment->view = VK_NULL_HANDLE;
		}
		if ( attachment->image ) {
			vmaDestroyImage( allocator, attachment->image, attachment->allocation );
			attachment->image = VK_NULL_HANDLE;
		}
		if ( attachment->mem ) {
			attachment->mem = VK_NULL_HANDLE;
		}
	} else {
		attachment = &attachments.emplace_back();
		attachment->descriptor = descriptor;
	}
	// un-request transient attachments if not supported yet requested
	if ( attachment->descriptor.usage & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT ) {
		bool supported = false;
		auto& properties = device->memoryProperties;
		for ( uint32_t i = 0; i < properties.memoryTypeCount; ++i ) {
			auto prop = properties.memoryTypes[i].propertyFlags;
			if ( prop & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT ) {
				supported = true;
			}
		}
		if ( !supported  ) {
			VK_VALIDATION_MESSAGE("Transient attachment requested yet not supported, disabling...");
			attachment->descriptor.usage &= ~VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
		}
	}

	VkImageCreateInfo imageCreateInfo = {};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.format = attachment->descriptor.format;
	imageCreateInfo.extent = { width, height, 1 };
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.arrayLayers = this->views;
	imageCreateInfo.samples = ext::vulkan::sampleCount( attachment->descriptor.samples );
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.usage = attachment->descriptor.usage;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	if ( attachment->descriptor.usage & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT ) {
		allocInfo.usage = VMA_MEMORY_USAGE_GPU_LAZILY_ALLOCATED;
	}
	VK_CHECK_RESULT(vmaCreateImage(allocator, &imageCreateInfo, &allocInfo, &attachment->image, &attachment->allocation, &attachment->allocationInfo));
	attachment->mem = attachment->allocationInfo.deviceMemory;

	VkImageAspectFlags aspectMask = 0;
	// specialization: depth buffer
	if ( attachment->descriptor.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT ) {
		aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT; // | VK_IMAGE_ASPECT_STENCIL_BIT;
	} else {
		aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}
	VkImageViewCreateInfo imageView = {};
	imageView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageView.format = attachment->descriptor.format;
	imageView.subresourceRange = {};
	imageView.subresourceRange.aspectMask = aspectMask;
	imageView.subresourceRange.baseMipLevel = 0;
	imageView.subresourceRange.levelCount = 1;
	imageView.subresourceRange.baseArrayLayer = 0;
	imageView.subresourceRange.layerCount = this->views;
	imageView.image = attachment->image;

	VK_CHECK_RESULT(vkCreateImageView(*device, &imageView, nullptr, &attachment->view));

	{
		VkBool32 blendEnabled = VK_FALSE;
		VkColorComponentFlags writeMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT;

		if ( attachment->descriptor.blend ) {
			blendEnabled = VK_TRUE;
			writeMask |= VK_COLOR_COMPONENT_A_BIT;
		}

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
		attachment->blendState = blendAttachmentState;
	}

	return attachments.size()-1;
}
void ext::vulkan::RenderTarget::initialize( Device& device ) {
	// Bind
	this->device = &device;
	uint32_t width = this->width > 0 ? this->width : ext::vulkan::settings::width;
	uint32_t height = this->height > 0 ? this->height : ext::vulkan::settings::height;

	// resize attachments if necessary
	if ( initialized ) {
		for ( auto& attachment: this->attachments ) {
			if ( attachment.descriptor.aliased ) continue;
			attach( attachment.descriptor, &attachment );
		}
	}
	// ensure attachments are already created
	assert( this->attachments.size() > 0 );

	// Create render pass
	if ( !renderPass ) {
		std::vector<VkAttachmentDescription> attachments; attachments.reserve( this->attachments.size() );
		
		for ( auto& attachment : this->attachments ) {
			VkAttachmentDescription description;
			description.format = attachment.descriptor.format;
			description.samples = ext::vulkan::sampleCount( attachment.descriptor.samples );
			description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			description.storeOp = attachment.descriptor.usage & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT ? VK_ATTACHMENT_STORE_OP_DONT_CARE : VK_ATTACHMENT_STORE_OP_STORE;
			description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			description.finalLayout = attachment.descriptor.layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : attachment.descriptor.layout;
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
	//	std::cout << this << ": " << std::endl;
		for ( auto& pass : passes ) {
			VkSubpassDescription description;
			// describe renderpass
			description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;			
			description.colorAttachmentCount = pass.colors.size();
			description.pColorAttachments = &pass.colors[0];
			description.pResolveAttachments = &pass.resolves[0];
			description.inputAttachmentCount = pass.inputs.size();
			description.pInputAttachments = &pass.inputs[0];
			description.pDepthStencilAttachment = &pass.depth;
			description.preserveAttachmentCount = 0;
			description.pPreserveAttachments = nullptr;
			description.flags = 0;
			for ( auto& input : pass.inputs ) {
				if ( input.attachment == pass.depth.attachment ) {
					description.pDepthStencilAttachment = NULL;
					break;
				}
			}
			descriptions.push_back(description);
		/*
			std::cout << "Pass: " << descriptions.size() - 1 << std::endl;
			for ( auto& color : pass.colors ) std::cout << "Color: " << color.attachment << "\t" << std::hex << color.layout << "\t" << this->attachments[color.attachment].image << std::endl;
			for ( auto& input : pass.inputs ) std::cout << "Input: " << input.attachment << "\t" << std::hex << input.layout << "\t" << this->attachments[input.attachment].image << std::endl;
			std::cout << std::endl;
		*/
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

		// depth dependency
		{
			VkSubpassDependency dependency;
			dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			
			dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
			dependency.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			dependency.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;

			dependency.dstSubpass = 0;
			dependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			
			dependencies.push_back(dependency);
		}
		{
			VkSubpassDependency dependency;
			dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
			
			dependency.srcSubpass = 0;
			dependency.srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			dependency.dstSubpass = i == 1 ? VK_SUBPASS_EXTERNAL : 1;
			dependency.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			dependency.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			
			dependencies.push_back(dependency);
		}
	/*
		for ( auto& dependency : dependencies  ) {
			std::cout << "Pass: " << dependency.srcSubpass << " -> " << dependency.dstSubpass << std::endl;
			std::cout << "\tStage: " << std::hex << dependency.srcStageMask << " -> " << std::hex << dependency.dstStageMask << std::endl;
			std::cout << "\tAccess: " << std::hex << dependency.srcAccessMask << " -> " << std::hex << dependency.dstAccessMask << std::endl;
		}
		std::cout << std::endl;
	*/
	
		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.flags = 0;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderPassInfo.pAttachments = &attachments[0];
		renderPassInfo.subpassCount = static_cast<uint32_t>(descriptions.size());
		renderPassInfo.pSubpasses = &descriptions[0];
		renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
		renderPassInfo.pDependencies = &dependencies[0];

/*
		uint32_t mask = 0;
		std::vector<uint32_t> masks;
		for ( size_t i = 0; i < this->views; ++i ) {
			mask |= (0b00000001 << i);
		}
		for ( size_t i = 0; i < static_cast<uint32_t>(descriptions.size()); ++i ) {
			masks.emplace_back(mask);
		}
		VkRenderPassMultiviewCreateInfo renderPassMultiviewInfo = {};
		renderPassMultiviewInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_MULTIVIEW_CREATE_INFO;
		renderPassMultiviewInfo.subpassCount = 1; //static_cast<uint32_t>(descriptions.size());
	//	renderPassMultiviewInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
		renderPassMultiviewInfo.pViewMasks = masks.data();
		renderPassMultiviewInfo.pCorrelationMasks = masks.data();
		renderPassMultiviewInfo.correlationMaskCount = 1;
		if ( this->views > 1 ) {
			renderPassInfo.pNext = &renderPassMultiviewInfo;
		}
*/
		VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));

	//	std::cout << renderPass << ": " << attachments.size() << std::endl;
	}

	{	
		// destroy previous framebuffers
		for ( auto& framebuffer : framebuffers ) vkDestroyFramebuffer( device, framebuffer, nullptr );

		RenderMode& base = ext::vulkan::getRenderMode( "Swapchain", false );
		framebuffers.resize(ext::vulkan::swapchain.buffers);
		for ( size_t i = 0; i < framebuffers.size(); ++i ) {
			std::vector<VkImageView> attachments;										
			for ( auto& attachment : this->attachments ) {
				if ( attachment.descriptor.aliased && attachment.descriptor.layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR ) {
					attachments.push_back(base.renderTarget.attachments[i].view);
				} else attachments.push_back(attachment.view);
			}

			VkFramebufferCreateInfo frameBufferCreateInfo = {};
			frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			// All frame buffers use the same renderpass setup
			frameBufferCreateInfo.renderPass = renderPass;
			frameBufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			frameBufferCreateInfo.pAttachments = attachments.data();
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
		if ( attachment.descriptor.aliased ) continue;
		vkDestroyImageView( *device, attachment.view, nullptr );
		attachment.view = VK_NULL_HANDLE;
	//	vkDestroyImage( *device, attachment.image, nullptr );
		vmaDestroyImage( allocator, attachment.image, attachment.allocation );
		attachment.image = VK_NULL_HANDLE;
	//	vkFreeMemory( *device, attachment.mem, nullptr );
		attachment.mem = VK_NULL_HANDLE;
	}

	renderPass = VK_NULL_HANDLE;
	device = VK_NULL_HANDLE;
}