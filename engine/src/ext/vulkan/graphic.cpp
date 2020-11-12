#include <uf/ext/vulkan/graphic.h>
#include <uf/ext/vulkan/initializers.h>
#include <uf/ext/vulkan/vulkan.h>
#include <uf/ext/openvr/openvr.h>

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
	this->device = &device;
	ext::vulkan::Buffers::initialize( device );
	aliased = false;
	
	std::string spirv;
	
	{
		std::ifstream is(this->filename = filename, std::ios::binary | std::ios::in | std::ios::ate);
		if ( !is.is_open() ) {
			VK_VALIDATION_MESSAGE("Error: Could not open shader file \"" << filename << "\"");
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

		auto parseResource = [&]( const spirv_cross::Resource& resource, VkDescriptorType descriptorType ) {			
			const auto& type = comp.get_type(resource.type_id);
			const auto& base_type = comp.get_type(resource.base_type_id);
			
			switch ( descriptorType ) {
				case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: {
					size_t size = comp.get_declared_struct_size(base_type);
					if ( size <= 0 ) break;
					if ( size > device.properties.limits.maxUniformBufferRange ) {
						VK_VALIDATION_MESSAGE("Invalid uniform buffer length of " << size << " for shader " << filename);
						size = device.properties.limits.maxUniformBufferRange;
					}
					size_t misalignment = size % device.properties.limits.minStorageBufferOffsetAlignment;
					if ( misalignment != 0 ) {
						VK_VALIDATION_MESSAGE("Invalid uniform buffer alignmnet of " << misalignment << " for shader " << filename << ", correcting...");
						size += misalignment;
					}
    				auto& uniform = uniforms.emplace_back();
    				uniform.create( size );
				} break;
			/*
				case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER: {
    				auto& uniform = uniforms.emplace_back();
					// const auto& base_type = comp.get_type(resource.base_type_id);
    				// uniform.create( comp.get_declared_struct_size(base_type) );
				} break;
			*/
			}

			size_t size = 1;
			if ( !type.array.empty() ) {
				size = type.array[0];
			/*
				uf::iostream << "ARRAY: " << filename << ":\t";
				for ( auto v : type.array ) 
					uf::iostream << v << " ";
				uf::iostream << "\n";
				uf::iostream << "ARRAY: " << filename << ":\t";
				for ( auto v : type.array_size_literal ) 
					uf::iostream << v << " ";
				uf::iostream << "\n";
			*/
			}
			descriptorSetLayoutBindings.push_back( ext::vulkan::initializers::descriptorSetLayoutBinding( descriptorType, stage, comp.get_decoration(resource.id, spv::DecorationBinding), size ) );
		};
		
		//	uf::iostream << "["<<filename<<"] Found resource: "#type " with binding: " << comp.get_decoration(resource.id, spv::DecorationBinding) << "\n";\

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
			const auto& type = comp.get_type(resource.base_type_id);
    		size_t size = comp.get_declared_struct_size(type);
    		if ( size <= 0 ) continue;
    		if ( size > device.properties.limits.maxPushConstantsSize ) {
				VK_VALIDATION_MESSAGE("Invalid push constant length of " << size << " for shader " << filename);
				size = device.properties.limits.maxPushConstantsSize;
			}
			auto& pushConstant = pushConstants.emplace_back();
			pushConstant.create( size );
		}
		
		size_t specializationSize = 0;
		for ( const auto& constant : comp.get_specialization_constants() ) {
			const auto& value = comp.get_constant(constant.id);
			const auto& type = comp.get_type(value.constant_type);
			size_t size = 4; //comp.get_declared_struct_size(type);

			VkSpecializationMapEntry specializationMapEntry;
			specializationMapEntry.constantID = constant.constant_id;
			specializationMapEntry.size = size;
			specializationMapEntry.offset = specializationSize;
			specializationMapEntries.push_back(specializationMapEntry);
			specializationSize += size;
		}
		if ( specializationSize > 0 ) {
		//	specializationConstants.create( specializationSize );
			specializationConstants.resize( specializationSize, 0 );

			uint8_t* s = (uint8_t*) &specializationConstants[0];
			size_t offset = 0;
			for ( const auto& constant : comp.get_specialization_constants() ) {
				const auto& value = comp.get_constant(constant.id);
				const auto& type = comp.get_type(value.constant_type);
				size_t size = 4;
				uint8_t buffer[size];
				switch ( type.basetype ) {
					case spirv_cross::SPIRType::UInt: {
						auto v = value.scalar();
						memcpy( &buffer[0], &v, sizeof(v) );
					} break;
					case spirv_cross::SPIRType::Int: {
						auto v = value.scalar_i32();
						memcpy( &buffer[0], &v, sizeof(v) );
					} break;
					case spirv_cross::SPIRType::Float: {
						auto v = value.scalar_f32();
						memcpy( &buffer[0], &v, sizeof(v) );
					} break;
					case spirv_cross::SPIRType::Boolean: {
						auto v = value.scalar()!=0;
						memcpy( &buffer[0], &v, sizeof(v) );
					} break;
				}

				memcpy( &s[offset], &buffer, size );
				offset += size;
			}

			{
				specializationInfo = {};
				specializationInfo.dataSize = specializationSize;
				specializationInfo.mapEntryCount = specializationMapEntries.size();
				specializationInfo.pMapEntries = specializationMapEntries.data();
				specializationInfo.pData = s;
				descriptor.pSpecializationInfo = &specializationInfo;
			}
		}
	/*
	*/
	//	LOOP_RESOURCES( stage_inputs, VkVertexInputAttributeDescription );
	//	LOOP_RESOURCES( stage_outputs, VkPipelineColorBlendAttachmentState );
	}

	// organize layouts
	{
		std::sort( descriptorSetLayoutBindings.begin(), descriptorSetLayoutBindings.end(), [&]( const VkDescriptorSetLayoutBinding& l, const VkDescriptorSetLayoutBinding& r ){
			return l.binding < r.binding;
		} );
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
	return this->initialize( graphic, graphic.descriptor );
}
void ext::vulkan::Pipeline::initialize( Graphic& graphic, GraphicDescriptor& descriptor ) {
	this->device = graphic.device;
	Device& device = *graphic.device;

	// VK_VALIDATION_MESSAGE(&graphic << ": Shaders: " << graphic.material.shaders.size() << " Textures: " << graphic.material.textures.size());
	assert( graphic.material.shaders.size() > 0 );

	RenderMode& renderMode = ext::vulkan::getRenderMode( descriptor.renderMode, true);
	auto& renderTarget = renderMode.getRenderTarget(  descriptor.renderTarget );
	{
		std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings;
		std::vector<VkPushConstantRange> pushConstantRanges;
		std::vector<VkDescriptorPoolSize> poolSizes;
		std::unordered_map<VkDescriptorType, uint32_t> descriptorTypes;

		for ( auto& shader : graphic.material.shaders ) {
			descriptorSetLayoutBindings.insert( descriptorSetLayoutBindings.begin(), shader.descriptorSetLayoutBindings.begin(), shader.descriptorSetLayoutBindings.end() );

			std::size_t offset = 0;
			for ( auto& pushConstant : shader.pushConstants ) {
				size_t len = pushConstant.data().len;
				if ( len <= 0 || len > device.properties.limits.maxPushConstantsSize ) {
					VK_VALIDATION_MESSAGE("Invalid push constent length of " << len << " for shader " << shader.filename);
				//	goto PIPELINE_INITIALIZATION_INVALID;
					len = device.properties.limits.maxPushConstantsSize;
				}
				pushConstantRanges.push_back(ext::vulkan::initializers::pushConstantRange(
					shader.descriptor.stage,
					len,
					offset
				));
				offset += len;
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
	// Compute
	for ( auto& shader : graphic.material.shaders ) {
		if ( shader.descriptor.stage != VK_SHADER_STAGE_COMPUTE_BIT ) continue;

		// Create compute shader pipelines
		VkComputePipelineCreateInfo computePipelineCreateInfo = ext::vulkan::initializers::computePipelineCreateInfo(
			pipelineLayout,
			0
		);
		computePipelineCreateInfo.stage = shader.descriptor;
		VK_CHECK_RESULT(vkCreateComputePipelines(device, device.pipelineCache, 1, &computePipelineCreateInfo, nullptr, &pipeline));
	
		return;
	}
	// Graphic
	{
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = ext::vulkan::initializers::pipelineInputAssemblyStateCreateInfo(
			descriptor.topology,
			0,
			VK_FALSE
		);
		VkPipelineRasterizationStateCreateInfo rasterizationState = ext::vulkan::initializers::pipelineRasterizationStateCreateInfo(
			descriptor.fill,
			descriptor.cullMode, //	VK_CULL_MODE_NONE,
			descriptor.frontFace,
			0
		);
		rasterizationState.lineWidth = graphic.descriptor.lineWidth;

		std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentStates;

		if ( renderMode.getType() != "Swapchain" ) {
			auto& subpass = renderTarget.passes[descriptor.subpass];
			for ( auto& color : subpass.colors ) {
				blendAttachmentStates.push_back(renderTarget.attachments[color.attachment].blendState);
			}
			// require blending if independentBlend is not an enabled feature
			if ( device.enabledFeatures.independentBlend == VK_FALSE ) {
				for ( size_t i = 1; i < blendAttachmentStates.size(); ++i ) {
					blendAttachmentStates[i] = blendAttachmentStates[0];
				}
			}
		} else {
			descriptor.subpass = 0;

			VkBool32 blendEnabled = VK_FALSE;
			VkColorComponentFlags writeMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT;

			if ( true ) {
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
			blendAttachmentStates.push_back(blendAttachmentState);
		}

		VkPipelineColorBlendStateCreateInfo colorBlendState = ext::vulkan::initializers::pipelineColorBlendStateCreateInfo(
			blendAttachmentStates.size(),
			blendAttachmentStates.data()
		);
		VkPipelineDepthStencilStateCreateInfo depthStencilState = ext::vulkan::initializers::pipelineDepthStencilStateCreateInfo(
			descriptor.depthTest.test,
			descriptor.depthTest.write,
			descriptor.depthTest.operation //VK_COMPARE_OP_LESS_OR_EQUAL
		);
		VkPipelineViewportStateCreateInfo viewportState = ext::vulkan::initializers::pipelineViewportStateCreateInfo(
			1, 1, 0
		);
		VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
		switch ( ext::vulkan::settings::msaa ) {
			case 64: samples = VK_SAMPLE_COUNT_64_BIT; break;
			case 32: samples = VK_SAMPLE_COUNT_32_BIT; break;
			case 16: samples = VK_SAMPLE_COUNT_16_BIT; break;
			case  8: samples =  VK_SAMPLE_COUNT_8_BIT; break;
			case  4: samples =  VK_SAMPLE_COUNT_4_BIT; break;
			case  2: samples =  VK_SAMPLE_COUNT_2_BIT; break;
			default: samples =  VK_SAMPLE_COUNT_1_BIT; break;
		}
		VkPipelineMultisampleStateCreateInfo multisampleState = ext::vulkan::initializers::pipelineMultisampleStateCreateInfo(
			samples,
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
				descriptor.geometry.sizes.vertex, 
				VK_VERTEX_INPUT_RATE_VERTEX
			)
		};
		// Attribute descriptions
		// Describes memory layout and shader positions
		std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions = {};
		for ( auto& attribute : descriptor.geometry.attributes ) {
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
			renderTarget.renderPass,
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
		pipelineCreateInfo.subpass = descriptor.subpass;

		VK_CHECK_RESULT(vkCreateGraphicsPipelines( device, device.pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipeline));
	}

	graphic.process = true;
	return;
PIPELINE_INITIALIZATION_INVALID:
	graphic.process = false;
	VK_VALIDATION_MESSAGE("Pipeline initialization invalid, updating next tick...");
	uf::thread::add( uf::thread::get("Main"), [&]() -> int {
		this->initialize( graphic, descriptor );
	return 0;}, true );
	return;
}
void ext::vulkan::Pipeline::record( Graphic& graphic, VkCommandBuffer commandBuffer ) {
	auto bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	for ( auto& shader : graphic.material.shaders ) {
		if ( shader.descriptor.stage == VK_SHADER_STAGE_COMPUTE_BIT ) {
			bindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
		}
		size_t offset = 0;
		for ( auto& pushConstant : shader.pushConstants ) {
			size_t len = 0;
			void* pointer = NULL;
			if ( bindPoint == VK_PIPELINE_BIND_POINT_COMPUTE ) {
				len = pushConstant.data().len;
				pointer = pushConstant.data().data;
			} else {
				struct Stereo {
					uint32_t pass;
				};
				static Stereo stereo;
				stereo.pass = ext::openvr::renderPass;

				len = sizeof(stereo);
				pointer = &stereo;
			}
			if ( len > 0 && pointer ) {
				vkCmdPushConstants( commandBuffer, pipelineLayout, shader.descriptor.stage, 0, len, pointer );
			}
		}
	}
	// Bind descriptor sets describing shader binding points
	vkCmdBindDescriptorSets(commandBuffer, bindPoint, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
	// Bind the rendering pipeline
	// The pipeline (state object) contains all states of the rendering pipeline, binding it will set all the states specified at pipeline creation time
	vkCmdBindPipeline(commandBuffer, bindPoint, pipeline);
}
void ext::vulkan::Pipeline::update( Graphic& graphic ) {
	return this->update( graphic, graphic.descriptor );
}
void ext::vulkan::Pipeline::update( Graphic& graphic, GraphicDescriptor& descriptor ) {
	//
	if ( descriptorSet == VK_NULL_HANDLE ) return;
	// generate fallback empty texture
	auto& emptyTexture = Texture2D::empty;

	RenderMode& renderMode = ext::vulkan::getRenderMode(descriptor.renderMode, true);
	auto& renderTarget = renderMode.getRenderTarget(descriptor.renderTarget );

	std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings;
	for ( auto& shader : graphic.material.shaders ) {
		descriptorSetLayoutBindings.insert( descriptorSetLayoutBindings.begin(), shader.descriptorSetLayoutBindings.begin(), shader.descriptorSetLayoutBindings.end() );
	}

	struct {
		std::vector<VkDescriptorBufferInfo> uniform;
		std::vector<VkDescriptorBufferInfo> storage;
		std::vector<VkDescriptorImageInfo> image;
		std::vector<VkDescriptorImageInfo> sampler;
		std::vector<VkDescriptorImageInfo> input;
	} infos;

	if ( descriptor.subpass < renderTarget.passes.size() ) {
		auto& subpass = renderTarget.passes[descriptor.subpass];
		for ( auto& input : subpass.inputs ) {
			infos.input.push_back(ext::vulkan::initializers::descriptorImageInfo( 
				renderTarget.attachments[input.attachment].view,
			//	input.layout
				input.layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : input.layout
			));
		}
	}

	{
		for ( auto& texture : graphic.material.textures ) {
			infos.image.emplace_back(texture.descriptor);
		}
		for ( auto& sampler : graphic.material.samplers ) {
			infos.sampler.emplace_back(sampler.descriptor.info);
		}
	}

	for ( auto& shader : graphic.material.shaders ) {
		#define PARSE_BUFFER( buffers ) for ( auto& buffer : buffers ) {\
			if ( buffer.usageFlags & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT ) {\
				infos.uniform.emplace_back(buffer.descriptor);\
			}\
			if ( buffer.usageFlags & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT ) {\
				infos.storage.emplace_back(buffer.descriptor);\
			}\
		}

		PARSE_BUFFER(shader.buffers)
		PARSE_BUFFER(graphic.buffers)

		// check if we can even consume that many infos
		size_t consumes = 0;
		for ( auto& layout : shader.descriptorSetLayoutBindings ) {
			switch ( layout.descriptorType ) {
				// consume an texture image info
				case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
				case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
				case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE: {
					consumes += layout.descriptorCount;
				} break;
			}
		}
		for ( size_t i = infos.image.size(); i < consumes; ++i ) {
			 infos.image.push_back(emptyTexture.descriptor);
		}
	}

	auto uniformBufferInfo = infos.uniform.begin();
	auto storageBufferInfo = infos.storage.begin();
	auto imageInfo = infos.image.begin();
	auto samplerInfo = infos.sampler.begin();
	auto inputInfo = infos.input.begin();

	#define BREAK_ASSERT(condition, ...) if ( condition ) { VK_VALIDATION_MESSAGE(#condition << "\t" << __VA_ARGS__); break; }

	std::vector<VkWriteDescriptorSet> writeDescriptorSets;
	for ( auto& shader : graphic.material.shaders ) {
		for ( auto& layout : shader.descriptorSetLayoutBindings ) {
		//	VK_VALIDATION_MESSAGE(shader.filename << "\tType: " << layout.descriptorType << "\tConsuming: " << layout.descriptorCount);
			switch ( layout.descriptorType ) {
				// consume an texture image info
				case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
				case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
				case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE: {
					BREAK_ASSERT( imageInfo == infos.image.end(), "Filename: " << shader.filename << "\tCount: " << layout.descriptorCount )
					writeDescriptorSets.push_back(ext::vulkan::initializers::writeDescriptorSet(
						descriptorSet,
						layout.descriptorType,
						layout.binding,
						&(*imageInfo),
						layout.descriptorCount
					));
					imageInfo += layout.descriptorCount;
				} break;
				case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT: {
					BREAK_ASSERT( inputInfo == infos.input.end(), "Filename: " << shader.filename << "\tCount: " << layout.descriptorCount )
					writeDescriptorSets.push_back(ext::vulkan::initializers::writeDescriptorSet(
						descriptorSet,
						layout.descriptorType,
						layout.binding,
						&(*inputInfo),
						layout.descriptorCount
					));
					inputInfo += layout.descriptorCount;
				} break;
				case VK_DESCRIPTOR_TYPE_SAMPLER: {
					BREAK_ASSERT( samplerInfo == infos.sampler.end(), "Filename: " << shader.filename << "\tCount: " << layout.descriptorCount )
					writeDescriptorSets.push_back(ext::vulkan::initializers::writeDescriptorSet(
						descriptorSet,
						layout.descriptorType,
						layout.binding,
						&(*samplerInfo),
						layout.descriptorCount
					));
					samplerInfo += layout.descriptorCount;
				} break;
				case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: {
					BREAK_ASSERT( uniformBufferInfo == infos.uniform.end(), "Filename: " << shader.filename << "\tCount: " << layout.descriptorCount )
					writeDescriptorSets.push_back(ext::vulkan::initializers::writeDescriptorSet(
						descriptorSet,
						layout.descriptorType,
						layout.binding,
						&(*uniformBufferInfo),
						layout.descriptorCount
					));
					uniformBufferInfo += layout.descriptorCount;
				} break;
				case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER: {
					BREAK_ASSERT( storageBufferInfo == infos.storage.end(), "Filename: " << shader.filename << "\tCount: " << layout.descriptorCount )
					writeDescriptorSets.push_back(ext::vulkan::initializers::writeDescriptorSet(
						descriptorSet,
						layout.descriptorType,
						layout.binding,
						&(*storageBufferInfo),
						layout.descriptorCount
					));
					storageBufferInfo += layout.descriptorCount;
				} break;
			}

		}
	}

	for ( auto& descriptor : writeDescriptorSets ) {
		for ( size_t i = 0; i < descriptor.descriptorCount; ++i ) {
			if ( descriptor.pBufferInfo ) {
				if ( descriptor.pBufferInfo[i].offset % device->properties.limits.minUniformBufferOffsetAlignment != 0 ) {
					VK_VALIDATION_MESSAGE("Invalid descriptor for buffer: " << descriptor.pBufferInfo[i].buffer << " (Offset: " << descriptor.pBufferInfo[i].offset << ", Range: " << descriptor.pBufferInfo[i].range << "), invalidating...");
					goto PIPELINE_UPDATE_INVALID;
				}
			}
			if ( descriptor.pImageInfo ) {
				if ( descriptor.pImageInfo[i].imageLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ) {
					VK_VALIDATION_MESSAGE("Image layout is VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, fixing to VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL");
					auto pointer = const_cast<VkDescriptorImageInfo*>(&descriptor.pImageInfo[i]);
					pointer->imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
				}
				if ( descriptor.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER && !descriptor.pImageInfo[i].sampler ) {
					VK_VALIDATION_MESSAGE("Image descriptor type is VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, yet lacks a sampler, adding default sampler...");
					auto pointer = const_cast<VkDescriptorImageInfo*>(&descriptor.pImageInfo[i]);
					pointer->sampler = emptyTexture.sampler.sampler;
				}
			}
		}
	}
	renderMode.rebuild = true;
	graphic.process = true;

	vkUpdateDescriptorSets(
		*device,
		writeDescriptorSets.size(),
		writeDescriptorSets.data(),
		0,
		NULL
	);
	return;

PIPELINE_UPDATE_INVALID:
	graphic.process = false;
	VK_VALIDATION_MESSAGE("Pipeline update invalid, updating next tick...");
	uf::thread::add( uf::thread::get("Main"), [&]() -> int {
		this->update( graphic, descriptor );
	return 0;}, true );
	return;
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

	// check how many samplers requested
	for ( auto& layout : shader.descriptorSetLayoutBindings ) {
		if ( layout.descriptorType != VK_DESCRIPTOR_TYPE_SAMPLER ) continue;
		Sampler& sampler = samplers.emplace_back();
		sampler.initialize( *device );
	}
}
void ext::vulkan::Material::initializeShaders( const std::vector<std::pair<std::string, VkShaderStageFlagBits>>& layout ) {
	shaders.clear(); shaders.reserve( layout.size() );
	for ( auto& request : layout ) {
		attachShader( request.first, request.second );
	}
}
ext::vulkan::Graphic::~Graphic() {
	this->destroy();
}
void ext::vulkan::Graphic::initialize( const std::string& renderModeName ) {
	RenderMode& renderMode = ext::vulkan::getRenderMode(renderModeName, true);

	this->descriptor.renderMode = renderModeName;
	auto* device = renderMode.device;
	if ( !device ) device = &ext::vulkan::device;

	material.initialize( *device );

	ext::vulkan::Buffers::initialize( *device );
}
void ext::vulkan::Graphic::initializePipeline() {
	initializePipeline( this->descriptor, false );
}
ext::vulkan::Pipeline& ext::vulkan::Graphic::initializePipeline( GraphicDescriptor& descriptor, bool update ) {
	auto& pipeline = pipelines[descriptor.hash()];

//	auto previous = this->descriptor;
//	this->descriptor = descriptor;
	pipeline.initialize(*this, descriptor);
	pipeline.update(*this, descriptor);
//	if ( !update ) this->descriptor = previous;

	initialized = true;

	return pipeline;
}
bool ext::vulkan::Graphic::hasPipeline( GraphicDescriptor& descriptor ) {
	return pipelines.count( descriptor.hash() ) > 0;
}
ext::vulkan::Pipeline& ext::vulkan::Graphic::getPipeline() {
	return getPipeline( descriptor );
}
ext::vulkan::Pipeline& ext::vulkan::Graphic::getPipeline( GraphicDescriptor& descriptor ) {
	if ( !hasPipeline(descriptor) ) return initializePipeline( descriptor );
	return pipelines[descriptor.hash()];
}
void ext::vulkan::Graphic::updatePipelines() {
	for ( auto pair : this->pipelines ) {
		pair.second.update( *this );
	}
}
void ext::vulkan::Graphic::record( VkCommandBuffer commandBuffer ) {
	return this->record( commandBuffer, descriptor );
}
void ext::vulkan::Graphic::record( VkCommandBuffer commandBuffer, GraphicDescriptor& descriptor ) {
	if ( !process ) return;
	if ( !this->hasPipeline( descriptor ) ) {
		VK_VALIDATION_MESSAGE(this << ": has no valid pipeline");
		return;
	}

	auto& pipeline = this->getPipeline( descriptor );
	if ( pipeline.descriptorSet == VK_NULL_HANDLE ) return;
	assert( buffers.size() >= 2 );
	Buffer& vertexBuffer = buffers.at(0);
	Buffer& indexBuffer = buffers.at(1);

	pipeline.record(*this, commandBuffer);
	// Bind triangle vertex buffer (contains position and colors)
	VkDeviceSize offsets[1] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer.buffer, offsets);
	// Bind triangle index buffer
	VkIndexType indicesType = VK_INDEX_TYPE_UINT32;
	switch ( descriptor.geometry.sizes.indices * 8 ) {
		case  8: indicesType = VK_INDEX_TYPE_UINT8_EXT; break;
		case 16: indicesType = VK_INDEX_TYPE_UINT16; break;
		case 32: indicesType = VK_INDEX_TYPE_UINT32; break;
		default:
			throw std::runtime_error("invalid indices size of " + std::to_string((int) descriptor.geometry.sizes.indices));
		break;
	}
	vkCmdBindIndexBuffer(commandBuffer, indexBuffer.buffer, 0, indicesType);
	// Draw indexed triangle
	vkCmdDrawIndexed(commandBuffer, descriptor.indices, 1, 0, 0, 1);
}
void ext::vulkan::Graphic::destroy() {
	for ( auto& pair : pipelines ) pair.second.destroy();
	pipelines.clear();

	material.destroy();

	ext::vulkan::Buffers::destroy();

	ext::vulkan::states::rebuild = true;
}

#include <uf/utils/string/hash.h>
std::string ext::vulkan::GraphicDescriptor::hash() const {
	uf::Serializer serializer;

	serializer["subpass"] = subpass;
	if ( settings::experimental::individualPipelines ) serializer["renderMode"] = renderMode;
	serializer["renderTarget"] = renderTarget;
	serializer["geometry"]["sizes"]["vertex"] = geometry.sizes.vertex;
	serializer["geometry"]["sizes"]["indices"] = geometry.sizes.indices;

	{
		int i = 0;
		for ( auto& attribute : geometry.attributes ) {
			serializer["geometry"]["attributes"][i]["format"] = attribute.format;
			serializer["geometry"]["attributes"][i]["offset"] = attribute.offset;
			++i;
		}
	}
	serializer["topology"] = topology;
	serializer["cullMode"] = cullMode;
	serializer["fill"] = fill;
	serializer["lineWidth"] = lineWidth;
	serializer["frontFace"] = frontFace;
	serializer["depthTest"]["test"] = depthTest.test;
	serializer["depthTest"]["write"] = depthTest.write;
	serializer["depthTest"]["operation"] = depthTest.operation;

//	if ( renderMode != "Gui" ) uf::iostream << this << ": " << indices << ": " << renderMode << ": " << subpass << ": " << serializer << "\n";
	return uf::string::sha256( serializer.serialize() );
}