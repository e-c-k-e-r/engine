#include <uf/ext/vulkan/graphics/compute.h>
#include <uf/ext/vulkan/initializers.h>
#include <uf/ext/vulkan/vulkan.h>
#include <algorithm>

std::string ext::vulkan::ComputeGraphic::name() const {
	return "ComputeGraphic";
}
void ext::vulkan::ComputeGraphic::setStorageBuffers( Device& device, std::vector<Cube>& cubes, std::vector<Light>& lights, std::vector<Tree>& trees ) {
	this->device = &device;
	// Cubes
	if ( !cubes.empty() ) {
		uniforms.ssbo.cubes.start = 0;
		uniforms.ssbo.cubes.end = cubes.size();
	} {
		initializeBuffer(
			(void*) cubes.data(),
			cubes.size() * sizeof(Cube),
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			true
		);
	}
	// Trees
	if ( !trees.empty() ) {
		uniforms.ssbo.root = trees.size() - 1;
	}
	{
		initializeBuffer(
			(void*) trees.data(),
			trees.size() * sizeof(Tree),
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			true
		);
	}
	// Lights
	if ( !lights.empty() ) {
		uniforms.ssbo.lights.start = 0;
		uniforms.ssbo.lights.end = lights.size();
	} {
		initializeBuffer(
			(void*) lights.data(),
			lights.size() * sizeof(Light),
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			true
		);
	}
}
void ext::vulkan::ComputeGraphic::updateStorageBuffers( Device& device, std::vector<Cube>& cubes, std::vector<Light>& lights, std::vector<Tree>& trees ) {
	if ( !cubes.empty() ) {
		uniforms.ssbo.cubes.start = 0;
		uniforms.ssbo.cubes.end = cubes.size();
		updateBuffer(
			(void*) cubes.data(),
			cubes.size() * sizeof(Cube),
			1,
			true
		);
	}
	if ( !lights.empty() ) {
		uniforms.ssbo.lights.start = 0;
		uniforms.ssbo.lights.end = lights.size();
		updateBuffer(
			(void*) lights.data(),
			lights.size() * sizeof(Light),
			3,
			true
		);
	}
	if ( !trees.empty() ) {
		uniforms.ssbo.root = trees.size() - 1;
		updateBuffer(
			(void*) trees.data(),
			trees.size() * sizeof(Tree),
			2,
			true
		);
	}
}
void ext::vulkan::ComputeGraphic::initialize( Device& device, RenderMode& renderMode, uint32_t width, uint32_t height ) {
	assert( buffers.size() >= 3 );
	ext::vulkan::Graphic::initialize( device, renderMode );
	// Set queue
	{
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.pNext = NULL;
		queueCreateInfo.queueFamilyIndex = device.queueFamilyIndices.compute;
		queueCreateInfo.queueCount = 1;
		vkGetDeviceQueue(device, device.queueFamilyIndices.compute, 0, &queue);
	}
	// Create uniform buffer
	initializeBuffer(
		(void*) &uniforms,
		sizeof(uniforms),
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		false
	);
	// Swap buffers
	buffers = {
		buffers.at(3),
		buffers.at(0),
		buffers.at(1),
		buffers.at(2),
	};
	// Create render target
	{
		if ( width == 0 ) width = ext::vulkan::width;
		if ( height == 0 ) height = ext::vulkan::height;
		renderTarget.asRenderTarget( device, width, height, device.graphicsQueue );
	}
	// Set Descriptor Layout
	initializeDescriptorLayout({
		// Binding 0: Storage image (raytraced output)
		ext::vulkan::initializers::descriptorSetLayoutBinding(
			VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			VK_SHADER_STAGE_COMPUTE_BIT,
			0
		),
		// Binding 1: Uniform buffer block
		ext::vulkan::initializers::descriptorSetLayoutBinding(
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			VK_SHADER_STAGE_COMPUTE_BIT,
			1
		),
		// Binding 1: Shader storage buffer for the cubes
		ext::vulkan::initializers::descriptorSetLayoutBinding(
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			VK_SHADER_STAGE_COMPUTE_BIT,
			2
		),
		// Binding 1: Shader storage buffer for the cubes
		ext::vulkan::initializers::descriptorSetLayoutBinding(
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			VK_SHADER_STAGE_COMPUTE_BIT,
			3
		),
		// Binding 1: Shader storage buffer for the cubes
		ext::vulkan::initializers::descriptorSetLayoutBinding(
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			VK_SHADER_STAGE_COMPUTE_BIT,
			4
		),
		// Binding 1: Texture sampler
		ext::vulkan::initializers::descriptorSetLayoutBinding(
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			VK_SHADER_STAGE_COMPUTE_BIT,
			5
		)
	});
	// Set descriptor pool
	initializeDescriptorPool({
		ext::vulkan::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1),			// UBO
		ext::vulkan::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1),	// Graphics image samplers
		ext::vulkan::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1),				// Storage image for ray traced image output
		ext::vulkan::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3), 			// Storage buffer for the scene primitives
	}, 3);
	// Set descriptor set
	initializeDescriptorSet({
		// Binding 0: Output storage image
		ext::vulkan::initializers::writeDescriptorSet(
			descriptorSet,
			VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			0,
			&renderTarget.descriptor
		),
		// Binding 1: Uniform buffer block
		ext::vulkan::initializers::writeDescriptorSet(
			descriptorSet,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			1,
			&(buffers.at(0).descriptor)
		),
		// Binding 2: Shader storage buffer for the cubes
		ext::vulkan::initializers::writeDescriptorSet(
			descriptorSet,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			2,
			&(buffers.at(1).descriptor)
		),
		// Binding 2: Shader storage buffer for the lights
		ext::vulkan::initializers::writeDescriptorSet(
			descriptorSet,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			3,
			&(buffers.at(2).descriptor)
		),
		// Binding 2: Shader storage buffer for the trees
		ext::vulkan::initializers::writeDescriptorSet(
			descriptorSet,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			4,
			&(buffers.at(3).descriptor)
		),
		// Binding 3 : Texture sampler
		ext::vulkan::initializers::writeDescriptorSet(
			descriptorSet,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			5,
			&diffuseTexture.descriptor
		)
	});
	// Create pipeline
	{
		// Create compute shader pipelines
		VkComputePipelineCreateInfo computePipelineCreateInfo = ext::vulkan::initializers::computePipelineCreateInfo(
			pipelineLayout,
			0
		);
		computePipelineCreateInfo.stage = shader.stages.at(0);
		VK_CHECK_RESULT(vkCreateComputePipelines(device, device.pipelineCache, 1, &computePipelineCreateInfo, nullptr, &pipeline));
	}
	// Create command pool
	{
		// Separate command pool as queue family for compute may be different than graphics
		VkCommandPoolCreateInfo cmdPoolInfo = {};
		cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPoolInfo.queueFamilyIndex = device.queueFamilyIndices.compute;
		cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		VK_CHECK_RESULT(vkCreateCommandPool(device, &cmdPoolInfo, nullptr, &commandPool));
	}
	// Create command buffer
	{
		// Create a command buffer for compute operations
		VkCommandBufferAllocateInfo cmdBufAllocateInfo = ext::vulkan::initializers::commandBufferAllocateInfo(
			commandPool,
			VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			1
		);

		VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, &commandBuffer));
	}
	// Create fence
	{
		// Fence for compute CB sync
		VkFenceCreateInfo fenceCreateInfo = ext::vulkan::initializers::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
		VK_CHECK_RESULT(vkCreateFence(device, &fenceCreateInfo, nullptr, &fence));
	}
	// Build command buffer
	this->createCommandBuffer(commandBuffer);
}

void ext::vulkan::ComputeGraphic::updateUniformBuffer() {
	updateBuffer( (void*) &uniforms, sizeof(uniforms), 0, false );
}

void ext::vulkan::ComputeGraphic::createCommandBuffer( VkCommandBuffer commandBuffer ) {
	VkCommandBufferBeginInfo cmdBufInfo = ext::vulkan::initializers::commandBufferBeginInfo();

	VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffer, &cmdBufInfo));

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &descriptorSet, 0, 0);

	vkCmdDispatch(commandBuffer, renderTarget.width / 32, renderTarget.height / 32, 1);

	vkEndCommandBuffer(commandBuffer);
}

void ext::vulkan::ComputeGraphic::destroy() {
	renderTarget.destroy();
	diffuseTexture.destroy();
	if ( fence != VK_NULL_HANDLE ) {
		vkDestroyFence(*device, fence, nullptr);
		fence = VK_NULL_HANDLE;
	}
	if ( commandPool != VK_NULL_HANDLE )  {
		vkDestroyCommandPool(*device, commandPool, nullptr);
		commandPool = VK_NULL_HANDLE;
	}
	ext::vulkan::Graphic::destroy();
}

////////////////////////////////////////////////////////////////
bool ext::vulkan::RTGraphic::autoAssignable() const {
	return true;
}
std::string ext::vulkan::RTGraphic::name() const {
	return "RTGraphic";
}
void ext::vulkan::RTGraphic::updateUniformBuffer() {
	compute.updateUniformBuffer();
}
void ext::vulkan::RTGraphic::initialize( const std::string& renderMode ) {
	return initialize(this->device ? *device : ext::vulkan::device, ext::vulkan::getRenderMode(renderMode));
}
void ext::vulkan::RTGraphic::initialize( Device& device, RenderMode& renderMode ) {
	ext::vulkan::Graphic::initialize( device, renderMode );
	compute.initialize( device, renderMode, this->width, this->height );
	// Set Descriptor Layout
	initializeDescriptorLayout({
		ext::vulkan::initializers::descriptorSetLayoutBinding(
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			VK_SHADER_STAGE_FRAGMENT_BIT,
			0
		)
	});
	// Load shaders
	initializeShaders({
		{"./data/shaders/texture.vert.spv", VK_SHADER_STAGE_VERTEX_BIT},
		{"./data/shaders/texture.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT},
	});
	// Create pipeline
	{
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = ext::vulkan::initializers::pipelineInputAssemblyStateCreateInfo(
			VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			0,
			VK_FALSE
		);

		VkPipelineRasterizationStateCreateInfo rasterizationState = ext::vulkan::initializers::pipelineRasterizationStateCreateInfo(
			VK_POLYGON_MODE_FILL,
			VK_CULL_MODE_FRONT_BIT,
			VK_FRONT_FACE_COUNTER_CLOCKWISE,
			0
		);

		VkPipelineColorBlendAttachmentState blendAttachmentState = ext::vulkan::initializers::pipelineColorBlendAttachmentState(
			0xf,
			VK_FALSE
		);

		VkPipelineColorBlendStateCreateInfo colorBlendState = ext::vulkan::initializers::pipelineColorBlendStateCreateInfo(
			1,
		
		&blendAttachmentState
		);

		VkPipelineDepthStencilStateCreateInfo depthStencilState = ext::vulkan::initializers::pipelineDepthStencilStateCreateInfo(
			VK_FALSE,
			VK_FALSE,
			VK_COMPARE_OP_LESS_OR_EQUAL
		);

		VkPipelineViewportStateCreateInfo viewportState = ext::vulkan::initializers::pipelineViewportStateCreateInfo(
			1, 1, 0
		);

		VkPipelineMultisampleStateCreateInfo multisampleState = ext::vulkan::initializers::pipelineMultisampleStateCreateInfo(
			VK_SAMPLE_COUNT_1_BIT,
			0
		);

		std::vector<VkDynamicState> dynamicStateEnables = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};
		VkPipelineDynamicStateCreateInfo dynamicState = ext::vulkan::initializers::pipelineDynamicStateCreateInfo(
			dynamicStateEnables.data(),
			static_cast<uint32_t>(dynamicStateEnables.size()),
			0
		);

		VkGraphicsPipelineCreateInfo pipelineCreateInfo = ext::vulkan::initializers::pipelineCreateInfo(
			pipelineLayout,
			renderMode.getRenderPass(),
			0
		);
		VkPipelineVertexInputStateCreateInfo emptyInputState = {};
		emptyInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		emptyInputState.vertexAttributeDescriptionCount = 0;
		emptyInputState.pVertexAttributeDescriptions = nullptr;
		emptyInputState.vertexBindingDescriptionCount = 0;
		emptyInputState.pVertexBindingDescriptions = nullptr;
		pipelineCreateInfo.pVertexInputState = &emptyInputState;
		pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
		pipelineCreateInfo.pRasterizationState = &rasterizationState;
		pipelineCreateInfo.pColorBlendState = &colorBlendState;
		pipelineCreateInfo.pMultisampleState = &multisampleState;
		pipelineCreateInfo.pViewportState = &viewportState;
		pipelineCreateInfo.pDepthStencilState = &depthStencilState;
		pipelineCreateInfo.pDynamicState = &dynamicState;
		pipelineCreateInfo.stageCount = static_cast<uint32_t>(shader.stages.size());
		pipelineCreateInfo.pStages = shader.stages.data();
		initializePipeline( pipelineCreateInfo );
	}
	// Set descriptor pool
	initializeDescriptorPool({
		ext::vulkan::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2),			// Compute UBO
		ext::vulkan::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4),	// Graphics image samplers
		ext::vulkan::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1),				// Storage image for ray traced image output
		ext::vulkan::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2), 			// // Storage buffer for the scene primitives
	}, 3);
	// Set descriptor set
	initializeDescriptorSet({
		// Binding 0 : Fragment shader texture sampler
		ext::vulkan::initializers::writeDescriptorSet(
			descriptorSet,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			0,
			&compute.renderTarget.descriptor
		),
	});
}
void ext::vulkan::RTGraphic::destroy() {
	compute.destroy();
	ext::vulkan::Graphic::destroy();
}
void ext::vulkan::RTGraphic::createCommandBuffer( VkCommandBuffer commandBuffer ) {
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, NULL);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
	vkCmdDraw(commandBuffer, 3, 1, 0, 0);
}
void ext::vulkan::RTGraphic::render() {
	// Submit compute commands
	// Use a fence to ensure that compute command buffer has finished executing before using it again
	vkWaitForFences( *device, 1, &compute.fence, VK_TRUE, UINT64_MAX );
	vkResetFences( *device, 1, &compute.fence );

	VkSubmitInfo computeSubmitInfo = ext::vulkan::initializers::submitInfo();
	computeSubmitInfo.commandBufferCount = 1;
	computeSubmitInfo.pCommandBuffers = &compute.commandBuffer;

	VK_CHECK_RESULT(vkQueueSubmit(compute.queue, 1, &computeSubmitInfo, compute.fence));
}
void ext::vulkan::RTGraphic::createImageMemoryBarrier( VkCommandBuffer commandBuffer ) {
	// Image memory barrier to make sure that compute shader writes are finished before sampling from the texture
	VkImageMemoryBarrier imageMemoryBarrier = {};
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
	imageMemoryBarrier.image = compute.renderTarget.image;
	imageMemoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
	imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	vkCmdPipelineBarrier(
		commandBuffer,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_FLAGS_NONE,
		0, nullptr,
		0, nullptr,
		1, &imageMemoryBarrier
	);
}