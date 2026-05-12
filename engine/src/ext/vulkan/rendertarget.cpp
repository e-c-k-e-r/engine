#if UF_USE_VULKAN

#include <uf/ext/vulkan/vulkan.h>
#include <uf/ext/vulkan/rendertarget.h>
#include <uf/ext/vulkan/initializers.h>
#include <uf/utils/window/window.h>

void ext::vulkan::RenderTarget::addPass( VkPipelineStageFlags stage, VkAccessFlags access, const uf::stl::vector<size_t>& colors, const uf::stl::vector<size_t>& inputs, const uf::stl::vector<size_t>& resolves, size_t depth, size_t layer, bool autoBuildPipeline ) {
	Subpass pass;
	pass.stage = stage;
	pass.access = access;
	pass.layer = layer;
	pass.autoBuildPipeline = autoBuildPipeline;
	for ( auto& i : colors )  pass.colors.emplace_back( VkAttachmentReference{ (uint32_t) i, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL } );
	for ( auto& i : inputs )  pass.inputs.emplace_back( VkAttachmentReference{ (uint32_t) i, i == depth ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL } );
	for ( auto& i : resolves ) pass.resolves.emplace_back( VkAttachmentReference{ (uint32_t) i, i == depth ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL } );
	if ( depth < attachments.size() ) pass.depth = { (uint32_t) depth, attachments[depth].descriptor.layout };

	if ( !resolves.empty() && resolves.size() != colors.size() )
		VK_VALIDATION_MESSAGE("Mismatching resolves count: Expecting {}, got {}", colors.size(), resolves.size());

	passes.emplace_back(pass);
}
size_t ext::vulkan::RenderTarget::attach( const Attachment::Descriptor& descriptor, Attachment* attachment ) {
	if ( this->views == 0 ) this->views = 1;

	size_t index = attachments.size();
	uint32_t width = this->width > 0 ? this->width : (ext::vulkan::settings::width * this->scale);
	uint32_t height = this->height > 0 ? this->height : (ext::vulkan::settings::height * this->scale);

	if ( attachment ) {
		for ( auto& view : attachment->views ) {
			vkDestroyImageView(*device, view, nullptr);
			VK_UNREGISTER_HANDLE( view );
		}
		attachment->views.clear();
		if ( attachment->image && attachment->descriptor.layout != VK_IMAGE_LAYOUT_PRESENT_SRC_KHR ) {
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
		//	VK_VALIDATION_MESSAGE("Transient attachment requested yet not supported, disabling...");
			attachment->descriptor.usage &= ~VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
		}
	}
	if ( attachment->descriptor.screenshottable ) {
		attachment->descriptor.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	}

	attachment->descriptor.width = width;
	attachment->descriptor.height = height;
	if ( attachment->descriptor.mips <= 1 ) {
		attachment->descriptor.mips = 1;
	} else {
		attachment->descriptor.mips = uf::vector::mips( pod::Vector2ui{ width, height } );
	}
	attachment->views.resize(this->views * attachment->descriptor.mips);

	bool isSwapchain = attachment->descriptor.layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	bool isDepth = attachment->descriptor.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

	// swapchain specialization
	if ( isSwapchain ) {
		attachment->descriptor.aliased = true;
		attachment->views.resize(ext::vulkan::swapchain.buffers);

		VkImageSubresourceRange subresourceRange;
		subresourceRange.baseMipLevel = 0;
		subresourceRange.levelCount = 1;
		subresourceRange.baseArrayLayer = 0;
		subresourceRange.aspectMask = isDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.layerCount = this->views;

		auto commandBuffer = device->fetchCommandBuffer(uf::renderer::QueueEnum::GRAPHICS);
		for ( size_t i = 0; i < ext::vulkan::swapchain.buffers; ++i ) {
			auto& image = ext::vulkan::swapchain.images[i];

			VkImageViewCreateInfo imageView = {};
			imageView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			imageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
			imageView.format = attachment->descriptor.format;
			imageView.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
			imageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageView.subresourceRange.baseMipLevel = 0;
			imageView.subresourceRange.levelCount = 1;
			imageView.subresourceRange.baseArrayLayer = 0;
			imageView.subresourceRange.layerCount = 1;
			imageView.image = image;

			VK_CHECK_RESULT(vkCreateImageView(*device, &imageView, nullptr, &attachment->views[i]));
			VK_REGISTER_HANDLE( attachment->views[i] );
			uf::renderer::Texture::setImageLayout( commandBuffer, image, VK_IMAGE_LAYOUT_UNDEFINED, attachment->descriptor.layout, subresourceRange );
		}
		device->flushCommandBuffer(commandBuffer, uf::renderer::QueueEnum::GRAPHICS);

		attachment->image = ext::vulkan::swapchain.images[0];
		attachment->view = attachment->views[0];
		attachment->mem = VK_NULL_HANDLE;

		return index;
	}

	VkImageCreateInfo imageCreateInfo = {};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.format = attachment->descriptor.format;
	imageCreateInfo.extent = { width, height, 1 };
	imageCreateInfo.mipLevels = attachment->descriptor.mips;
	imageCreateInfo.arrayLayers = this->views;
	imageCreateInfo.samples = ext::vulkan::sampleCount( attachment->descriptor.samples );
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.usage = attachment->descriptor.usage;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	if ( this->views == 6 ) {
		imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	}

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	if ( attachment->descriptor.usage & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT ) {
		allocInfo.usage = VMA_MEMORY_USAGE_GPU_LAZILY_ALLOCATED;
	}
	VK_CHECK_RESULT(vmaCreateImage(allocator, &imageCreateInfo, &allocInfo, &attachment->image, &attachment->allocation, &attachment->allocationInfo));
	VK_REGISTER_HANDLE( attachment->image );
	attachment->mem = attachment->allocationInfo.deviceMemory;

	VkImageAspectFlags aspectMask = 0;
	// specialization: depth buffer
	if ( isDepth ) {
		aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT; // | VK_IMAGE_ASPECT_STENCIL_BIT;
	} else {
		aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}
	VkImageViewCreateInfo imageView = {};
	imageView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
	if ( this->views > 1 ) {
		imageView.viewType = this->views == 6 ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D_ARRAY;
	}
	imageView.format = attachment->descriptor.format;
	imageView.subresourceRange = {};
	imageView.subresourceRange.aspectMask = aspectMask;
	imageView.subresourceRange.baseMipLevel = 0;
	imageView.subresourceRange.baseArrayLayer = 0;
	imageView.subresourceRange.levelCount = attachment->descriptor.mips;
	imageView.subresourceRange.layerCount = this->views;
	imageView.image = attachment->image;
	VK_CHECK_RESULT(vkCreateImageView(*device, &imageView, nullptr, &attachment->view));
	VK_REGISTER_HANDLE( attachment->view );

	if ( imageView.subresourceRange.levelCount > 1 ) {
		imageView.subresourceRange.levelCount = 1;
		VK_CHECK_RESULT(vkCreateImageView(*device, &imageView, nullptr, &attachment->framebufferView));
		VK_REGISTER_HANDLE( attachment->framebufferView );
	}

	size_t viewIndex = 0;
	for ( size_t layer = 0; layer < this->views; ++layer ) {
		imageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageView.subresourceRange.levelCount = 1;
		imageView.subresourceRange.layerCount = 1;
		for ( size_t mip = 0; mip < attachment->descriptor.mips; ++mip ) {
			imageView.subresourceRange.baseMipLevel = mip;
			imageView.subresourceRange.baseArrayLayer = layer;
			VK_CHECK_RESULT(vkCreateImageView(*device, &imageView, nullptr, &attachment->views[viewIndex]));
			VK_REGISTER_HANDLE( attachment->views[viewIndex] );
			++viewIndex;
		}
	}
	{
		VkBool32 blendEnabled = attachment->descriptor.blend ? VK_TRUE : VK_FALSE;
		VkColorComponentFlags writeMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

		VkPipelineColorBlendAttachmentState blendAttachmentState = ext::vulkan::initializers::pipelineColorBlendAttachmentState(
			writeMask,
			blendEnabled
		);
		if ( attachment->descriptor.blend ) {
			blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
			blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
		}
		attachment->blendState = blendAttachmentState;
	}

	if ( false ) {
		auto commandBuffer = device->fetchCommandBuffer(uf::renderer::QueueEnum::GRAPHICS);
		VkImageSubresourceRange subresourceRange;
		subresourceRange.baseMipLevel = 0;
		subresourceRange.baseArrayLayer = 0;
		subresourceRange.levelCount = attachment->descriptor.mips;
		subresourceRange.layerCount = this->views;
		subresourceRange.aspectMask = isDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
		uf::renderer::Texture::setImageLayout( commandBuffer, attachment->image, VK_IMAGE_LAYOUT_UNDEFINED, attachment->descriptor.layout, subresourceRange );
		device->flushCommandBuffer(commandBuffer, uf::renderer::QueueEnum::GRAPHICS);
	}

	return index;
}
void ext::vulkan::RenderTarget::initialize( Device& device ) {
	// Bind
	this->device = &device;
	uint32_t width = this->width > 0 ? this->width : (ext::vulkan::settings::width * this->scale);
	uint32_t height = this->height > 0 ? this->height : (ext::vulkan::settings::height * this->scale);

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
	bool isSwapchain = false;
	for ( auto& attachment : this->attachments ) {
		if ( attachment.descriptor.layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR ) isSwapchain = true;
	}

	if ( !renderPass ) {
		uf::stl::vector<VkAttachmentDescription> attachments; attachments.reserve( this->attachments.size() );
		for ( auto& attachment : this->attachments ) {
			VkAttachmentDescription description;
			description.format = attachment.descriptor.format;
			description.samples = ext::vulkan::sampleCount( attachment.descriptor.samples );
			description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			description.storeOp = attachment.descriptor.usage & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT ? VK_ATTACHMENT_STORE_OP_DONT_CARE : VK_ATTACHMENT_STORE_OP_STORE;
			description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // isSwapchain ? VK_IMAGE_LAYOUT_UNDEFINED : attachment.descriptor.layout; // VK_IMAGE_LAYOUT_UNDEFINED;
			description.finalLayout = ext::vulkan::Texture::remapRenderpassLayout( attachment.descriptor.layout );
			description.flags = 0;

			attachments.emplace_back(description);
		}

		// ensure that the subpasses are already described
		auto passes = this->passes;
		assert( passes.size() > 0 );
		
		// expand attachment indices
		/*
		for ( auto& pass : passes ) {
			for ( auto& input : pass.inputs ) input.attachment += pass.layer * this->attachments.size();
			for ( auto& color : pass.colors ) color.attachment += pass.layer * this->attachments.size();
			for ( auto& resolve : pass.resolves ) resolve.attachment += pass.layer * this->attachments.size();
			pass.depth.attachment += pass.layer * this->attachments.size();
		}
		*/

		uf::stl::vector<VkSubpassDescription> descriptions;
		uf::stl::vector<VkSubpassDependency> dependencies;

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
			descriptions.emplace_back(description);
		
			// transition dependency between subpasses
			dependency.srcSubpass = dependency.dstSubpass;
			dependency.srcStageMask = dependency.dstStageMask;
			dependency.srcAccessMask = dependency.dstAccessMask;
			dependency.dstSubpass = i++;
			dependency.dstStageMask = pass.stage;
			dependency.dstAccessMask = pass.access;
			dependencies.emplace_back(dependency);
		}
		// dependency: transition to final
		{
			dependency.srcSubpass = dependency.dstSubpass;
			dependency.srcStageMask = dependency.dstStageMask;
			dependency.srcAccessMask = dependency.dstAccessMask;
			dependency.dstSubpass = VK_SUBPASS_EXTERNAL;
			dependency.dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			dependency.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			dependencies.emplace_back(dependency);
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
			
			dependencies.emplace_back(dependency);
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
			
			dependencies.emplace_back(dependency);
		}

		// crunge
		if ( isSwapchain ) {
			dependencies.resize(2);
			dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[0].dstSubpass = 0;
			dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;            
			dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;   
			dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			dependencies[1].srcSubpass = 0;
			dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;   
			dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
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

		uint32_t viewMask = (1 << this->views) - 1;

		uf::stl::vector<uint32_t> viewMasks(descriptions.size(), viewMask);
		uf::stl::vector<uint32_t> correlationMasks = { viewMask };

		VkRenderPassMultiviewCreateInfo multiviewInfo = {};
		multiviewInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_MULTIVIEW_CREATE_INFO;
		
		if ( this->views > 1 ) {
			multiviewInfo.pNext = nullptr;
			multiviewInfo.subpassCount = static_cast<uint32_t>(viewMasks.size());
			multiviewInfo.pViewMasks = viewMasks.data();
			multiviewInfo.dependencyCount = 0;
			multiviewInfo.pViewOffsets = nullptr;
			multiviewInfo.correlationMaskCount = 1;
			multiviewInfo.pCorrelationMasks = correlationMasks.data();

			renderPassInfo.pNext = &multiviewInfo;
		}

		VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));
		VK_REGISTER_HANDLE( renderPass );
	}
	{	
		// destroy previous framebuffers
		for ( auto& framebuffer : framebuffers ) {
			vkDestroyFramebuffer( device, framebuffer, nullptr );
			VK_UNREGISTER_HANDLE( framebuffer );
		}
		framebuffers.clear();
		framebuffers.resize(ext::vulkan::swapchain.buffers);
		for ( size_t frame = 0; frame < framebuffers.size(); ++frame ) {
			uf::stl::vector<VkImageView> attachmentViews;										

			for ( auto& attachment : this->attachments ) {
				if ( attachment.descriptor.layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR ) {
					attachmentViews.emplace_back(attachment.views[frame]);
				} else if ( attachment.framebufferView != VK_NULL_HANDLE )  {
					attachmentViews.emplace_back(attachment.framebufferView);
				} else {
					attachmentViews.emplace_back(attachment.view);
				}
			}

			VkFramebufferCreateInfo frameBufferCreateInfo = {};
			frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			// All frame buffers use the same renderpass setup
			frameBufferCreateInfo.renderPass = renderPass;
			frameBufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachmentViews.size());
			frameBufferCreateInfo.pAttachments = attachmentViews.data();
			frameBufferCreateInfo.width = width;
			frameBufferCreateInfo.height = height;
			frameBufferCreateInfo.layers = 1;
			// Create the framebuffer
			VK_CHECK_RESULT(vkCreateFramebuffer( device, &frameBufferCreateInfo, nullptr, &framebuffers[frame]));
			VK_REGISTER_HANDLE( framebuffers[frame] );
		}
	}

	initialized = true;
}

void ext::vulkan::RenderTarget::destroy() {
	if ( !device ) return;
	vkDestroyRenderPass( *device, renderPass, nullptr );
	VK_UNREGISTER_HANDLE( renderPass );
	for ( auto& framebuffer : framebuffers ) {
		vkDestroyFramebuffer( *device, framebuffer, nullptr );
		VK_UNREGISTER_HANDLE( framebuffer );
	}
	framebuffers.clear();
	
	for ( auto& attachment : attachments ) {
		if ( attachment.descriptor.aliased ) continue;
		if ( attachment.framebufferView ) {
			vkDestroyImageView(*device, attachment.framebufferView, nullptr);
			VK_UNREGISTER_HANDLE( attachment.framebufferView );

		}
		if ( attachment.view ) {
			if ( attachment.view != attachment.views.front() ) {
				vkDestroyImageView(*device, attachment.view, nullptr);
				VK_UNREGISTER_HANDLE( attachment.view );
				attachment.view = VK_NULL_HANDLE;
			}
		}
		for ( size_t i = 0; i < this->views; ++i ) {
			vkDestroyImageView(*device, attachment.views[i], nullptr);
			VK_UNREGISTER_HANDLE( attachment.views[i] );
		}
		attachment.views.clear();
	//	vkDestroyImage( *device, attachment.image, nullptr );
		vmaDestroyImage( allocator, attachment.image, attachment.allocation );
		VK_UNREGISTER_HANDLE( attachment.image );
		attachment.image = VK_NULL_HANDLE;
	//	vkFreeMemory( *device, attachment.mem, nullptr );
		attachment.mem = VK_NULL_HANDLE;
	}

	renderPass = VK_NULL_HANDLE;
	device = VK_NULL_HANDLE;
}

#endif