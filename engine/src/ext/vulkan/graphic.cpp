#include <uf/ext/vulkan/graphic.h>
#include <uf/ext/vulkan/initializers.h>
#include <uf/ext/vulkan/vulkan.h>

#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_glsl.hpp>
#include <fstream>

namespace {
	uint32_t VERTEX_BUFFER_BIND_ID = 0;
}
/*
ext::vulkan::Shader::~Shader() {
	if ( !aliased ) destroy();
}
*/
void ext::vulkan::Shader::initialize( ext::vulkan::Device& device, const std::string& filename, VkShaderStageFlagBits stage ) {
	ext::vulkan::Buffers::initialize( device );
	aliased = false;
	this->device = &device;
	
	std::string spirv;
	{
		std::ifstream is(this->filename = filename, std::ios::binary | std::ios::in | std::ios::ate);
		if ( !is.is_open() ) {
			uf::iostream << "Error: Could not open shader file \"" << filename << "\"" << "\n";
			return;
		}
		is.seekg(0, std::ios::end); spirv.reserve(is.tellg()); is.seekg(0, std::ios::beg);
		spirv.assign((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());
	
		assert(spirv.size() > 0);
	}
	{
	
		VkShaderModuleCreateInfo moduleCreateInfo = {};
		moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		moduleCreateInfo.codeSize = spirv.size();
		moduleCreateInfo.pCode = (uint32_t*) spirv.data();

		VK_CHECK_RESULT(vkCreateShaderModule(device, &moduleCreateInfo, NULL, &module));
	}
	{
		descriptor.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		descriptor.stage = stage;
		descriptor.module = module;
		descriptor.pName = "main";

		assert(descriptor.module != VK_NULL_HANDLE);
	}
	// do reflection
	{
		spirv_cross::Compiler comp( (uint32_t*) &spirv[0], spirv.size() / 4 );
		spirv_cross::ShaderResources res = comp.get_shader_resources();

		auto parseResource = [&]( const spirv_cross::Resource& resource, VkDescriptorType type ) {
			// comp.get_decoration(resource.id, spv::DecorationDescriptorSet);
			switch ( type ) {
				case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT: {
				} break;
				case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: {
					const auto& type = comp.get_type(resource.base_type_id);
    				size_t size = comp.get_declared_struct_size(type);
    				auto& uniform = uniforms.emplace_back();
    				uniform.create( size );
				} break;
			}
			descriptorSetLayoutBindings.push_back(ext::vulkan::initializers::descriptorSetLayoutBinding( type, stage, comp.get_decoration(resource.id, spv::DecorationBinding) ) );
		};

	// std::cout << "Found resource: "#type " with binding: " << comp.get_decoration(resource.id, spv::DecorationBinding) << std::endl;
	#define LOOP_RESOURCES( key, type ) for ( const auto& resource : res.key ) {\
			parseResource( resource, type );\
		}
		LOOP_RESOURCES( sampled_images, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER );
		LOOP_RESOURCES( separate_images, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE );
		LOOP_RESOURCES( storage_images, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE );
		LOOP_RESOURCES( separate_samplers, VK_DESCRIPTOR_TYPE_SAMPLER );
		LOOP_RESOURCES( uniform_buffers, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER );
		LOOP_RESOURCES( subpass_inputs, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT );
		LOOP_RESOURCES( storage_buffers, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER );
	#undef LOOP_RESOURCES
		
		for ( const auto& resource : res.push_constant_buffers ) {
			auto& pushConstant = pushConstants.emplace_back();
			const auto& type = comp.get_type(resource.base_type_id);
    		size_t size = comp.get_declared_struct_size(type);
			pushConstant.create( size );
		}
	
		size_t specializationSize = 0;
		for ( const auto& constant : comp.get_specialization_constants() ) {
			const auto& value = comp.get_constant(constant.id);
			const auto& type = comp.get_type(value.constant_type);
			size_t size = comp.get_declared_struct_size(type);
			// default: value.scalar_i32()

			VkSpecializationMapEntry specializationMapEntry;
			specializationMapEntry.constantID = constant.constant_id;
			specializationMapEntry.size = size;
			specializationMapEntry.offset = specializationSize;
			specializationMapEntries.push_back(specializationMapEntry);
			specializationSize += size;
		}
		if ( specializationSize > 0 ) {
			specializationConstants.create( specializationSize );

			specializationInfo = {};
			specializationInfo.dataSize = specializationSize;
			specializationInfo.mapEntryCount = specializationMapEntries.size();
			specializationInfo.pMapEntries = specializationMapEntries.data();
			specializationInfo.pData = (void*) specializationConstants;
			descriptor.pSpecializationInfo = &specializationInfo;
		}	
	/*
	*/
	//	LOOP_RESOURCES( stage_inputs, VkVertexInputAttributeDescription );
	//	LOOP_RESOURCES( stage_outputs, VkPipelineColorBlendAttachmentState );
	}
	// update uniform buffers
	for ( auto& uniform : uniforms ) {
		pod::Userdata& userdata = uniform.data();
		initializeBuffer(
			(void*) userdata.data,
			userdata.len,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			false
		);
	}
}

void ext::vulkan::Shader::destroy() {
	if ( aliased ) return;
	if ( !device ) return;

	ext::vulkan::Buffers::destroy();

	if ( module != VK_NULL_HANDLE ) {
		vkDestroyShaderModule( *device, module, nullptr );
		module = VK_NULL_HANDLE;
		descriptor = {};
	}
}

void ext::vulkan::Pipeline::initialize( Graphic& graphic ) {
	this->device = graphic.device;
	Device& device = *graphic.device;

	assert( graphic.material.shaders.size() > 0 );

	RenderMode& renderMode = ext::vulkan::getRenderMode(graphic.descriptor.renderMode, true);

	{
		std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings;
		std::vector<VkPushConstantRange> pushConstantRanges;
		std::vector<VkDescriptorPoolSize> poolSizes;
		std::unordered_map<VkDescriptorType, uint32_t> descriptorTypes;

		for ( auto& shader : graphic.material.shaders ) {
			descriptorSetLayoutBindings.insert( descriptorSetLayoutBindings.begin(), shader.descriptorSetLayoutBindings.begin(), shader.descriptorSetLayoutBindings.end() );

			std::size_t offset = 0;
			for ( auto& pushConstant : shader.pushConstants ) {
				pushConstantRanges.push_back(ext::vulkan::initializers::pushConstantRange(
					shader.descriptor.stage,
					pushConstant.data().len,
					offset
				));
				offset += pushConstant.data().len;
			}
		}
		for ( auto& descriptor : descriptorSetLayoutBindings ) {
			if ( descriptorTypes.count( descriptor.descriptorType ) < 0 ) descriptorTypes[descriptor.descriptorType] = 0;
			descriptorTypes[descriptor.descriptorType] += descriptor.descriptorCount;
		}
		for ( auto pair : descriptorTypes ) {
			poolSizes.push_back(ext::vulkan::initializers::descriptorPoolSize(pair.first, pair.second));
		}
		VkDescriptorPoolCreateInfo descriptorPoolInfo = ext::vulkan::initializers::descriptorPoolCreateInfo(
			poolSizes.size(),
			poolSizes.data(),
			1
		);
		
		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));


		VkDescriptorSetLayoutCreateInfo descriptorLayout = ext::vulkan::initializers::descriptorSetLayoutCreateInfo(
			descriptorSetLayoutBindings.data(),
			descriptorSetLayoutBindings.size()
		);

		VK_CHECK_RESULT(vkCreateDescriptorSetLayout( device, &descriptorLayout, nullptr, &descriptorSetLayout ));

		VkDescriptorSetAllocateInfo allocInfo = ext::vulkan::initializers::descriptorSetAllocateInfo(
			descriptorPool,
			&descriptorSetLayout,
			1
		);
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet));

		VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = ext::vulkan::initializers::pipelineLayoutCreateInfo(
			&descriptorSetLayout,
			1
		);
		pPipelineLayoutCreateInfo.pushConstantRangeCount = pushConstantRanges.size();
		pPipelineLayoutCreateInfo.pPushConstantRanges = pushConstantRanges.data();

		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr, &pipelineLayout));
	}
	{
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = ext::vulkan::initializers::pipelineInputAssemblyStateCreateInfo(
			graphic.descriptor.topology,
			0,
			VK_FALSE
		);
		VkPipelineRasterizationStateCreateInfo rasterizationState = ext::vulkan::initializers::pipelineRasterizationStateCreateInfo(
			graphic.descriptor.fill,
			VK_CULL_MODE_BACK_BIT, //	VK_CULL_MODE_NONE,
			graphic.descriptor.frontFace,
			0
		);
		rasterizationState.lineWidth = graphic.descriptor.lineWidth;

		std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentStates;
		auto& subpass = renderMode.renderTarget.passes[graphic.descriptor.subpass];
		for ( auto& color : subpass.colors ) {
			VkPipelineColorBlendAttachmentState blendAttachmentState = ext::vulkan::initializers::pipelineColorBlendAttachmentState(
				0xf,
				VK_FALSE
			);
			blendAttachmentStates.push_back(blendAttachmentState);
		}
		VkPipelineColorBlendStateCreateInfo colorBlendState = ext::vulkan::initializers::pipelineColorBlendStateCreateInfo(
			blendAttachmentStates.size(),
			blendAttachmentStates.data()
		);
		VkPipelineDepthStencilStateCreateInfo depthStencilState = ext::vulkan::initializers::pipelineDepthStencilStateCreateInfo(
			graphic.descriptor.depthTest.test,
			graphic.descriptor.depthTest.write,
			graphic.descriptor.depthTest.operation //VK_COMPARE_OP_LESS_OR_EQUAL
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
			dynamicStateEnables.size(),
			0
		);

		// Binding description
		std::vector<VkVertexInputBindingDescription> vertexBindingDescriptions = {
			ext::vulkan::initializers::vertexInputBindingDescription(
				VERTEX_BUFFER_BIND_ID, 
				graphic.descriptor.size, 
				VK_VERTEX_INPUT_RATE_VERTEX
			)
		};
		// Attribute descriptions
		// Describes memory layout and shader positions
		std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions = {};
		for ( auto& attribute : graphic.descriptor.attributes ) {
			auto d = ext::vulkan::initializers::vertexInputAttributeDescription(
				VERTEX_BUFFER_BIND_ID,
				vertexAttributeDescriptions.size(),
				attribute.format,
				attribute.offset
			);
			vertexAttributeDescriptions.push_back(d);
		}

		VkPipelineVertexInputStateCreateInfo vertexInputState = ext::vulkan::initializers::pipelineVertexInputStateCreateInfo();
		vertexInputState.vertexBindingDescriptionCount = vertexBindingDescriptions.size();
		vertexInputState.pVertexBindingDescriptions = vertexBindingDescriptions.data();
		vertexInputState.vertexAttributeDescriptionCount = vertexAttributeDescriptions.size();
		vertexInputState.pVertexAttributeDescriptions = vertexAttributeDescriptions.data();

		std::vector<VkPipelineShaderStageCreateInfo> shaderDescriptors;
		for ( auto& shader : graphic.material.shaders ) {
			shaderDescriptors.push_back(shader.descriptor);
		}

		VkGraphicsPipelineCreateInfo pipelineCreateInfo = ext::vulkan::initializers::pipelineCreateInfo(
			pipelineLayout,
			renderMode.renderTarget.renderPass,
			0
		);
		pipelineCreateInfo.pVertexInputState = &vertexInputState;
		pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
		pipelineCreateInfo.pRasterizationState = &rasterizationState;
		pipelineCreateInfo.pColorBlendState = &colorBlendState;
		pipelineCreateInfo.pMultisampleState = &multisampleState;
		pipelineCreateInfo.pViewportState = &viewportState;
		pipelineCreateInfo.pDepthStencilState = &depthStencilState;
		pipelineCreateInfo.pDynamicState = &dynamicState;
		pipelineCreateInfo.stageCount = shaderDescriptors.size();
		pipelineCreateInfo.pStages = shaderDescriptors.data();
		pipelineCreateInfo.subpass = graphic.descriptor.subpass;

		VK_CHECK_RESULT(vkCreateGraphicsPipelines( device, device.pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipeline));
	}
}
void ext::vulkan::Pipeline::record( Graphic& graphic, VkCommandBuffer commandBuffer ) {
	for ( auto& shader : graphic.material.shaders ) {
		size_t offset = 0;
		for ( auto& pushConstant : shader.pushConstants ) {
			pod::Userdata& userdata = pushConstant.data();
			vkCmdPushConstants( commandBuffer, pipelineLayout, shader.descriptor.stage, offset, userdata.len, userdata.data );
			offset += userdata.len;
		}
	}
	// Bind descriptor sets describing shader binding points
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
	// Bind the rendering pipeline
	// The pipeline (state object) contains all states of the rendering pipeline, binding it will set all the states specified at pipeline creation time
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
}
void ext::vulkan::Pipeline::update( Graphic& graphic ) {
	std::vector<VkWriteDescriptorSet> writeDescriptorSets;
	std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings;
	std::vector<VkDescriptorImageInfo> inputDescriptors;

	for ( auto& shader : graphic.material.shaders ) {
		descriptorSetLayoutBindings.insert( descriptorSetLayoutBindings.begin(), shader.descriptorSetLayoutBindings.begin(), shader.descriptorSetLayoutBindings.end() );
	}

	RenderMode& renderMode = ext::vulkan::getRenderMode(graphic.descriptor.renderMode, true);
	auto& subpass = renderMode.renderTarget.passes[graphic.descriptor.subpass];
	for ( auto& input : subpass.inputs ) {
		inputDescriptors.push_back(ext::vulkan::initializers::descriptorImageInfo( 
			renderMode.renderTarget.attachments[input.attachment].view,
			input.layout
		));
	}
	{
		for ( auto& shader : graphic.material.shaders ) {
			auto textures = graphic.material.textures.begin();
			auto samplers = graphic.material.samplers.begin();
			auto buffers = shader.buffers.begin();
			auto attachments = inputDescriptors.begin();

			for ( auto& layout : shader.descriptorSetLayoutBindings ) {
				VkDescriptorBufferInfo* bufferInfo = NULL;
				VkDescriptorImageInfo* imageInfo = NULL;
				switch ( layout.descriptorType ) {
					case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
					case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE: {
						imageInfo = &((textures++)->descriptor);
					} break;
					case VK_DESCRIPTOR_TYPE_SAMPLER: {
						imageInfo = &((samplers++)->descriptor);
					} break;
					case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: {
						bufferInfo = &((buffers++)->descriptor);
					} break;
					case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT: {
						imageInfo = &(*(attachments++));
					} break;
				}
				if ( !bufferInfo && !imageInfo ) continue;
				if ( bufferInfo )
					writeDescriptorSets.push_back(ext::vulkan::initializers::writeDescriptorSet(
						descriptorSet,
						layout.descriptorType,
						layout.binding,
						bufferInfo
					));
				else if ( imageInfo ) 
					writeDescriptorSets.push_back(ext::vulkan::initializers::writeDescriptorSet(
						descriptorSet,
						layout.descriptorType,
						layout.binding,
						imageInfo
					));
			}
		}
	}
	
	vkUpdateDescriptorSets(
		*device,
		writeDescriptorSets.size(),
		writeDescriptorSets.data(),
		0,
		NULL
	);
}
void ext::vulkan::Pipeline::destroy() {
	if ( aliased ) return;

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
}

void ext::vulkan::Material::initialize( Device& device ) {
	this->device = &device;
}
void ext::vulkan::Material::destroy() {
	if ( aliased ) return;
	for ( auto& texture : textures ) texture.destroy();
	for ( auto& shader : shaders ) shader.destroy();
	for ( auto& sampler : samplers ) sampler.destroy();

	shaders.clear();
	textures.clear();
	samplers.clear();
}
void ext::vulkan::Material::attachShader( const std::string& filename, VkShaderStageFlagBits stage ) {
	auto& shader = shaders.emplace_back();
	shader.initialize( *device, filename, stage );
/*
	Shader shader;
	shader.initialize( *device, filename, stage );
	shaders.push_back( std::move( shader ) );
*/
}
void ext::vulkan::Material::initializeShaders( const std::vector<std::pair<std::string, VkShaderStageFlagBits>>& layout ) {
	shaders.clear(); shaders.reserve( layout.size() );
	for ( auto& request : layout ) attachShader( request.first, request.second );
}

void ext::vulkan::Graphic::initialize( const std::string& renderModeName ) {
	RenderMode& renderMode = ext::vulkan::getRenderMode(renderModeName, true);
	this->descriptor.renderMode = renderModeName;
	material.initialize( *renderMode.device );

	ext::vulkan::Buffers::initialize( *renderMode.device );
}
void ext::vulkan::Graphic::initializePipeline() {
	initializePipeline( this->descriptor, false );
}
ext::vulkan::Pipeline& ext::vulkan::Graphic::initializePipeline( Descriptor& descriptor, bool update ) {
	auto& pipeline = pipelines[descriptor.hash()];

	Descriptor previous = this->descriptor;
	this->descriptor = descriptor;
	pipeline.initialize(*this);
	pipeline.update(*this);
	if ( !update ) this->descriptor = previous;

	initialized = true;

	return pipeline;
}
bool ext::vulkan::Graphic::hasPipeline( Descriptor& descriptor ) {
	return pipelines.count( descriptor.hash() ) > 0;
}
ext::vulkan::Pipeline& ext::vulkan::Graphic::getPipeline( Descriptor& descriptor ) {
	if ( !hasPipeline(descriptor) ) return initializePipeline( descriptor );
	return pipelines[descriptor.hash()];
}
void ext::vulkan::Graphic::record( VkCommandBuffer commandBuffer ) {
	if ( !this->hasPipeline( descriptor ) ) return;

	auto& pipeline = this->getPipeline( descriptor );

	assert( buffers.size() >= 2 );
	Buffer& vertexBuffer = buffers.at(0);
	Buffer& indexBuffer = buffers.at(1);

	pipeline.record(*this, commandBuffer);
	// Bind triangle vertex buffer (contains position and colors)
	VkDeviceSize offsets[1] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer.buffer, offsets);
	// Bind triangle index buffer
	vkCmdBindIndexBuffer(commandBuffer, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
	// Draw indexed triangle
	vkCmdDrawIndexed(commandBuffer, descriptor.indices, 1, 0, 0, 1);
}
void ext::vulkan::Graphic::destroy() {
	for ( auto& pair : pipelines ) pair.second.destroy();
	pipelines.clear();

	material.destroy();

	ext::vulkan::Buffers::destroy();

	ext::vulkan::rebuild = true;
}

#include <uf/utils/string/hash.h>
std::string ext::vulkan::Graphic::Descriptor::hash() const {
	uf::Serializer serializer;

	serializer["subpass"] = subpass;
	serializer["size"] = size;

	{
		int i = 0;
		for ( auto& attribute : attributes ) {
			serializer["attributes"][i]["format"] = attribute.format;
			serializer["attributes"][i]["offset"] = attribute.offset;
			++i;
		}
	}
	serializer["topology"] = topology;
	serializer["fill"] = fill;
	serializer["lineWidth"] = lineWidth;
	serializer["frontFace"] = frontFace;
	serializer["depthTest"]["test"] = depthTest.test;
	serializer["depthTest"]["write"] = depthTest.write;
	serializer["depthTest"]["operation"] = depthTest.operation;

//	std::cout << this << ": " << indices << ": " << subpass << ": " << serializer << std::endl;

	return uf::string::sha256( serializer.serialize() );
}