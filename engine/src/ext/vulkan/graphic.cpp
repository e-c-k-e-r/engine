#include <uf/ext/vulkan/initializers.h>
#include <uf/ext/vulkan/graphic.h>
#include <uf/ext/vulkan/vulkan.h>

// Buffers
size_t ext::vulkan::Graphic::initializeBuffer( void* data, VkDeviceSize length, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, bool stage ) {
	Buffer buffer;

	if ( autoAssignable() ) autoAssign();
	
	// Stage
	if ( !stage ) {
		VK_CHECK_RESULT(device->createBuffer(
			usageFlags,
			memoryPropertyFlags,
			buffer,
			length
		));
		size_t index = buffers.size();
		buffers.push_back( std::move(buffer) );
		this->updateBuffer( data, length, index, stage );
		return index;
	}

	VkQueue queue = device->graphicsQueue;
	Buffer staging;
	VkDeviceSize storageBufferSize = length;
	device->createBuffer(
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		staging,
		storageBufferSize,
		data
	);

	device->createBuffer(
		usageFlags,
		memoryPropertyFlags,
		buffer,
		storageBufferSize
	);

	// Copy to staging buffer
	VkCommandBuffer copyCmd = device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
	VkBufferCopy copyRegion = {};
	copyRegion.size = storageBufferSize;
	vkCmdCopyBuffer(copyCmd, staging.buffer, buffer.buffer, 1, &copyRegion);
	device->flushCommandBuffer(copyCmd, queue, true);
	staging.destroy();

	size_t index = buffers.size();
	buffers.push_back( std::move(buffer) );
	return index;
}
void ext::vulkan::Graphic::updateBuffer( void* data, VkDeviceSize length, size_t index, bool stage ) {
	Buffer& buffer = buffers.at(index);
	return updateBuffer( data, length, buffer, stage );
}
void ext::vulkan::Graphic::updateBuffer( void* data, VkDeviceSize length, Buffer& buffer, bool stage ) {
	if ( !stage ) {
		VK_CHECK_RESULT(buffer.map());
		memcpy(buffer.mapped, data, length);
		buffer.unmap();
		return;
	}
	VkQueue queue = device->graphicsQueue;
	Buffer staging;
	device->createBuffer(
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		staging,
		length,
		data
	);

	// Copy to staging buffer
	VkCommandBuffer copyCmd = device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
	VkBufferCopy copyRegion = {};
	copyRegion.size = length;
	vkCmdCopyBuffer(copyCmd, staging.buffer, buffer.buffer, 1, &copyRegion);
	device->flushCommandBuffer(copyCmd, queue, true);
	staging.destroy();
}
// Set shaders
void ext::vulkan::Graphic::initializeShaders( const std::vector<std::pair<std::string, VkShaderStageFlagBits>>& layout ) {
	if ( !device ) device = &ext::vulkan::device;
	shader.stages.clear();
	shader.modules.clear();
	shader.stages.reserve( layout.size() );
	for ( auto& pair : layout ) {
		shader.stages.push_back(loadShader(pair.first, pair.second, device->logicalDevice, shader.modules));
	}
}
// Set Descriptor Layout
void ext::vulkan::Graphic::initializeDescriptorLayout( const std::vector<VkDescriptorSetLayoutBinding>& setLayoutBindings, std::size_t pushConstantSize ) {
	VkDescriptorSetLayoutCreateInfo descriptorLayout = ext::vulkan::initializers::descriptorSetLayoutCreateInfo(
		setLayoutBindings.data(),
		static_cast<uint32_t>(setLayoutBindings.size())
	);

	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(*device, &descriptorLayout, nullptr, &descriptorSetLayout));

	VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = ext::vulkan::initializers::pipelineLayoutCreateInfo(
		&descriptorSetLayout,
		1
	);

	VkPushConstantRange pushConstantRange = {};
	if ( pushConstantSize > 0 ) {
		pushConstantRange = ext::vulkan::initializers::pushConstantRange(
			VK_SHADER_STAGE_VERTEX_BIT,
			pushConstantSize,
			0
		);
		// Push constant ranges are part of the pipeline layout
		pPipelineLayoutCreateInfo.pushConstantRangeCount = 1;
		pPipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;
	}

	VK_CHECK_RESULT(vkCreatePipelineLayout(*device, &pPipelineLayoutCreateInfo, nullptr, &pipelineLayout));
}
// Create pipeline
void ext::vulkan::Graphic::initializePipeline( VkGraphicsPipelineCreateInfo pipelineCreateInfo ) {
/*
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = ext::vulkan::initializers::pipelineInputAssemblyStateCreateInfo(
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		0,
		VK_FALSE
	);

	VkPipelineRasterizationStateCreateInfo rasterizationState = ext::vulkan::initializers::pipelineRasterizationStateCreateInfo(
		VK_POLYGON_MODE_FILL,
		VK_CULL_MODE_NONE,
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
		VK_TRUE,
		VK_TRUE,
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

	// Binding description
	vertices.bindingDescriptions.resize(1);
	vertices.bindingDescriptions[0] = ext::vulkan::initializers::vertexInputBindingDescription(
		VERTEX_BUFFER_BIND_ID, 
		sizeof(Vertex), 
		VK_VERTEX_INPUT_RATE_VERTEX
	);
	// Attribute descriptions
	// Describes memory layout and shader positions
	vertices.attributeDescriptions.resize(3);
	// Location 0 : Position
	vertices.attributeDescriptions[0] = ext::vulkan::initializers::vertexInputAttributeDescription(
		VERTEX_BUFFER_BIND_ID,
		0,
		VK_FORMAT_R32G32B32_SFLOAT,
		offsetof(Vertex, position)
	);			
	// Location 1 : Texture coordinates
	vertices.attributeDescriptions[1] = ext::vulkan::initializers::vertexInputAttributeDescription(
		VERTEX_BUFFER_BIND_ID,
		1,
		VK_FORMAT_R32G32_SFLOAT,
		offsetof(Vertex, uv)
	);
	// Location 1 : Vertex normal
	vertices.attributeDescriptions[2] = ext::vulkan::initializers::vertexInputAttributeDescription(
		VERTEX_BUFFER_BIND_ID,
		2,
		VK_FORMAT_R32G32B32_SFLOAT,
		offsetof(Vertex, normal)
	);

	vertices.inputState = ext::vulkan::initializers::pipelineVertexInputStateCreateInfo();
	vertices.inputState.vertexBindingDescriptionCount = static_cast<uint32_t>(vertices.bindingDescriptions.size());
	vertices.inputState.pVertexBindingDescriptions = vertices.bindingDescriptions.data();
	vertices.inputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertices.attributeDescriptions.size());
	vertices.inputState.pVertexAttributeDescriptions = vertices.attributeDescriptions.data();

	// Load shaders
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages = this->loadShaders();

	VkGraphicsPipelineCreateInfo pipelineCreateInfo = ext::vulkan::initializers::pipelineCreateInfo(
		pipelineLayout,
		swapchain.renderPass,
		0
	);
	pipelineCreateInfo.pVertexInputState = &vertices.inputState;
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
	pipelineCreateInfo.pRasterizationState = &rasterizationState;
	pipelineCreateInfo.pColorBlendState = &colorBlendState;
	pipelineCreateInfo.pMultisampleState = &multisampleState;
	pipelineCreateInfo.pViewportState = &viewportState;
	pipelineCreateInfo.pDepthStencilState = &depthStencilState;
	pipelineCreateInfo.pDynamicState = &dynamicState;
	pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
	pipelineCreateInfo.pStages = shaderStages.data();
*/

	ext::vulkan::mutex.lock();
	VK_CHECK_RESULT(vkCreateGraphicsPipelines(*device, swapchain->pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipeline));
	ext::vulkan::mutex.unlock();
}
// Set descriptor pool
void ext::vulkan::Graphic::initializeDescriptorPool( const std::vector<VkDescriptorPoolSize>& poolSizes, size_t length ) {
	VkDescriptorPoolCreateInfo descriptorPoolInfo = ext::vulkan::initializers::descriptorPoolCreateInfo(
		static_cast<uint32_t>(poolSizes.size()),
		const_cast<VkDescriptorPoolSize*>(poolSizes.data()),
		length
	);

	VK_CHECK_RESULT(vkCreateDescriptorPool(*device, &descriptorPoolInfo, nullptr, &descriptorPool));
	
	VkDescriptorSetAllocateInfo allocInfo = ext::vulkan::initializers::descriptorSetAllocateInfo(
		descriptorPool,
		&descriptorSetLayout,
		1
	);

	VK_CHECK_RESULT(vkAllocateDescriptorSets(*device, &allocInfo, &descriptorSet));
}
// Set descriptor set
void ext::vulkan::Graphic::initializeDescriptorSet( const std::vector<VkWriteDescriptorSet>& writeDescriptorSets ) {
	vkUpdateDescriptorSets(
		*device,
		static_cast<uint32_t>(writeDescriptorSets.size()),
		writeDescriptorSets.data(),
		0,
		NULL
	);
}

bool ext::vulkan::Graphic::autoAssignable() const {
	return false;
}
void ext::vulkan::Graphic::autoAssign() {
	if ( !autoAssigned ) {
		if ( this->swapchain ) this->swapchain->rebuild = true;
		ext::vulkan::graphics->push_back(this);
	}
	autoAssigned = true;
}
std::string ext::vulkan::Graphic::name() const {
	return "Graphic";
}

void ext::vulkan::Graphic::initialize( Device& device, Swapchain& swapchain ) {
	this->device = &device;
	this->swapchain = &swapchain;
	this->initialized = true;
	if ( autoAssignable() ) autoAssign();
}
ext::vulkan::Graphic::~Graphic() {
	this->destroy();
}
void ext::vulkan::Graphic::destroy() {
	for (auto& buffer : buffers) {
		buffer.destroy();
	}
	if ( autoAssigned ) {
		ext::vulkan::graphics->erase(
			std::remove(ext::vulkan::graphics->begin(), ext::vulkan::graphics->end(), this),
			ext::vulkan::graphics->end()
		);
		if ( this->swapchain ) this->swapchain->rebuild = true;
		autoAssigned = false;
	}
	initialized = false;
	if ( !device || device == VK_NULL_HANDLE ) return;
	if ( descriptorPool != VK_NULL_HANDLE) {
		vkDestroyDescriptorPool( *device, descriptorPool, nullptr );
		descriptorPool = VK_NULL_HANDLE;
	}
	if ( pipelineLayout != VK_NULL_HANDLE ) {
		vkDestroyPipelineLayout( *device, pipelineLayout, nullptr );
		pipelineLayout = VK_NULL_HANDLE;
	}
	if ( pipeline != VK_NULL_HANDLE ) {
		vkDestroyPipeline( *device, pipeline, nullptr );
		pipeline = VK_NULL_HANDLE;
	}
	if ( descriptorSetLayout != VK_NULL_HANDLE ) {
		vkDestroyDescriptorSetLayout( *device, descriptorSetLayout, nullptr );
		descriptorSetLayout = VK_NULL_HANDLE;
	}
	for ( auto& module : shader.modules ) {
		if ( module != VK_NULL_HANDLE ) {
			vkDestroyShaderModule( *device, module, nullptr );
			module = VK_NULL_HANDLE;
		}
	}
	shader.modules.clear();
	shader.stages.clear();
	buffers.clear();
}
/*
bool ext::vulkan::Graphic::autoAssignable() {
	return false;
}
*/
void ext::vulkan::Graphic::render() {
}
void ext::vulkan::Graphic::createCommandBuffer( VkCommandBuffer commandBuffer ) {
}
void ext::vulkan::Graphic::createImageMemoryBarrier( VkCommandBuffer commandBuffer ) {
}
void ext::vulkan::Graphic::updateDynamicUniformBuffer( bool force ) {
}