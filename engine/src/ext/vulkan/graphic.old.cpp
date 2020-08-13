#include <uf/ext/vulkan/graphic.old.h>
#include <uf/ext/vulkan/initializers.h>
#include <uf/ext/vulkan/vulkan.h>
#include <uf/ext/openvr/openvr.h>

VkFrontFace ext::vulkan::GraphicOld::DEFAULT_WINDING_ORDER = VK_FRONT_FACE_CLOCKWISE;

// Buffers
size_t ext::vulkan::GraphicOld::initializeBuffer( void* data, VkDeviceSize length, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, bool stage ) {
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
void ext::vulkan::GraphicOld::updateBuffer( void* data, VkDeviceSize length, size_t index, bool stage ) {
	Buffer& buffer = buffers.at(index);
	return updateBuffer( data, length, buffer, stage );
}
void ext::vulkan::GraphicOld::updateBuffer( void* data, VkDeviceSize length, Buffer& buffer, bool stage ) {
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
void ext::vulkan::GraphicOld::initializeShaders( const std::vector<std::pair<std::string, VkShaderStageFlagBits>>& layout ) {
	if ( !device ) device = &ext::vulkan::device;
	shader.stages.clear();
	shader.modules.clear();
	shader.stages.reserve( layout.size() );
	for ( auto& pair : layout ) {
		shader.stages.push_back(loadShader(pair.first, pair.second, device->logicalDevice, shader.modules));
	}
}
// Set Descriptor Layout
void ext::vulkan::GraphicOld::initializeDescriptorLayout( const std::vector<VkDescriptorSetLayoutBinding>& setLayoutBindings, std::size_t pushConstantSize ) {
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
void ext::vulkan::GraphicOld::initializePipeline( VkGraphicsPipelineCreateInfo pipelineCreateInfo ) {
	// ext::vulkan::mutex.lock();

	pipelineCreateInfo.subpass = this->subpass;
	VK_CHECK_RESULT(vkCreateGraphicsPipelines(*device, device->pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipeline));

	// ext::vulkan::mutex.unlock();
}
// Set descriptor pool
void ext::vulkan::GraphicOld::initializeDescriptorPool( const std::vector<VkDescriptorPoolSize>& poolSizes, size_t length ) {
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
void ext::vulkan::GraphicOld::initializeDescriptorSet( const std::vector<VkWriteDescriptorSet>& writeDescriptorSets ) {
	vkUpdateDescriptorSets(
		*device,
		static_cast<uint32_t>(writeDescriptorSets.size()),
		writeDescriptorSets.data(),
		0,
		NULL
	);
}

bool ext::vulkan::GraphicOld::autoAssignable() const {
	return false;
}
void ext::vulkan::GraphicOld::autoAssign() {
	if ( !autoAssigned ) {
		ext::vulkan::rebuild = true;
/*
		ext::vulkan::graphicOlds->push_back(this);
*/
	}
	autoAssigned = true;
}
std::string ext::vulkan::GraphicOld::name() const {
	return "Graphic";
}

void ext::vulkan::GraphicOld::initialize( const std::string& renderMode ) {
	return initialize(this->device ? *device : ext::vulkan::device, ext::vulkan::getRenderMode(renderMode));
}
void ext::vulkan::GraphicOld::initialize( Device& device, RenderMode& renderMode ) {
	this->device = &device;
	this->renderMode = &renderMode;
	this->initialized = true;
	if ( autoAssignable() ) autoAssign();
}
ext::vulkan::GraphicOld::~GraphicOld() {
	this->destroy();
}
void ext::vulkan::GraphicOld::destroy() {
	for (auto& buffer : buffers) {
		buffer.destroy();
	}
	if ( autoAssigned ) {
/*
		ext::vulkan::graphicOlds->erase(
			std::remove(ext::vulkan::graphicOlds->begin(), ext::vulkan::graphicOlds->end(), this),
			ext::vulkan::graphicOlds->end()
		);
*/
		autoAssigned = false;
		ext::vulkan::rebuild = true;
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
bool ext::vulkan::GraphicOld::autoAssignable() {
	return false;
}
*/
void ext::vulkan::GraphicOld::render() {
}
void ext::vulkan::GraphicOld::createCommandBuffer( VkCommandBuffer commandBuffer ) {
}
void ext::vulkan::GraphicOld::createImageMemoryBarrier( VkCommandBuffer commandBuffer ) {
}
void ext::vulkan::GraphicOld::updateDynamicUniformBuffer( bool force ) {
}