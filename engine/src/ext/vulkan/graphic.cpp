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
void ext::vulkan::Shader::initialize( ext::vulkan::Device& _device, const std::string& filename, VkShaderStageFlagBits stage ) {
	auto& device = &_device ? _device : ext::vulkan::device;
	this->device = &device;
	ext::vulkan::Buffers::initialize( device );
	aliased = false;
	
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

		auto parseResource = [&]( const spirv_cross::Resource& resource, VkDescriptorType descriptorType ) {			
			const auto& type = comp.get_type(resource.type_id);
			const auto& base_type = comp.get_type(resource.base_type_id);
			
			switch ( descriptorType ) {
				case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: {
					size_t size = comp.get_declared_struct_size(base_type);
					if ( size <= 0 ) break;
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
				std::cout << "ARRAY: " << filename << ":\t";
				for ( auto v : type.array ) 
					std::cout << v << " ";
				std::cout << std::endl;
				std::cout << "ARRAY: " << filename << ":\t";
				for ( auto v : type.array_size_literal ) 
					std::cout << v << " ";
				std::cout << std::endl;
			*/
			}
			descriptorSetLayoutBindings.push_back( ext::vulkan::initializers::descriptorSetLayoutBinding( descriptorType, stage, comp.get_decoration(resource.id, spv::DecorationBinding), size ) );
		};
		
		//	std::cout << "["<<filename<<"] Found resource: "#type " with binding: " << comp.get_decoration(resource.id, spv::DecorationBinding) << std::endl;\

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
	this->device = graphic.device;
	Device& device = *graphic.device;

	// std::cout << &graphic << ": Shaders: " << graphic.material.shaders.size() << " Textures: " << graphic.material.textures.size() << std::endl;
	assert( graphic.material.shaders.size() > 0 );

	RenderMode& renderMode = ext::vulkan::getRenderMode(graphic.descriptor.renderMode, true);
	auto& renderTarget = renderMode.getRenderTarget( graphic.descriptor.renderTarget );
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
					std::cout << "INVALID PUSH CONSTANT LEN OF : " << len << " FOR " << shader.filename << std::endl;
					len = 4;
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
			graphic.descriptor.topology,
			0,
			VK_FALSE
		);
		VkPipelineRasterizationStateCreateInfo rasterizationState = ext::vulkan::initializers::pipelineRasterizationStateCreateInfo(
			graphic.descriptor.fill,
			graphic.descriptor.cullMode, //	VK_CULL_MODE_NONE,
			graphic.descriptor.frontFace,
			0
		);
		rasterizationState.lineWidth = graphic.descriptor.lineWidth;

		std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentStates;

		if ( renderMode.getType() != "Swapchain" ) {
			auto& subpass = renderTarget.passes[graphic.descriptor.subpass];
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
			graphic.descriptor.subpass = 0;

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
				blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
				blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
				blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
			}
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
		VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
		switch ( ext::vulkan::msaa ) {
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
				graphic.descriptor.geometry.sizes.vertex, 
				VK_VERTEX_INPUT_RATE_VERTEX
			)
		};
		// Attribute descriptions
		// Describes memory layout and shader positions
		std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions = {};
		for ( auto& attribute : graphic.descriptor.geometry.attributes ) {
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
		pipelineCreateInfo.subpass = graphic.descriptor.subpass;

		VK_CHECK_RESULT(vkCreateGraphicsPipelines( device, device.pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipeline));
	}
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
	// generate fallback empty texture
	auto& emptyTexture = Texture2D::empty;

	RenderMode& renderMode = ext::vulkan::getRenderMode(graphic.descriptor.renderMode, true);
	auto& renderTarget = renderMode.getRenderTarget(graphic.descriptor.renderTarget );

	std::vector<VkWriteDescriptorSet> writeDescriptorSets;
	std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings;
	std::vector<VkDescriptorImageInfo> inputDescriptors;
	std::vector<VkDescriptorImageInfo> imageInfos;

	for ( auto& shader : graphic.material.shaders ) {
		descriptorSetLayoutBindings.insert( descriptorSetLayoutBindings.begin(), shader.descriptorSetLayoutBindings.begin(), shader.descriptorSetLayoutBindings.end() );
	}

	if ( graphic.descriptor.subpass < renderTarget.passes.size() ) {
		auto& subpass = renderTarget.passes[graphic.descriptor.subpass];
		for ( auto& input : subpass.inputs ) {
			inputDescriptors.push_back(ext::vulkan::initializers::descriptorImageInfo( 
				renderTarget.attachments[input.attachment].view,
			//	input.layout
				input.layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : input.layout
			));
		}
	}
	{
		for ( auto& shader : graphic.material.shaders ) {
			std::vector<VkDescriptorBufferInfo> buffersStorageVector;
			std::vector<VkDescriptorBufferInfo> buffersUniformsVector;
			for ( auto& buffer : shader.buffers ) {
				if ( buffer.usageFlags & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT ) {
					auto& descriptor = buffersStorageVector.emplace_back(buffer.descriptor);
					if ( descriptor.offset % device->properties.limits.minStorageBufferOffsetAlignment != 0 ) {
					//	std::cout << "Unaligned offset! Expecting " << device->properties.limits.minUniformBufferOffsetAlignment << ", got " << descriptor.pBufferInfo->offset << std::endl;
						descriptor.offset = 0;
					}
				}
				if ( buffer.usageFlags & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT ) {
					auto& descriptor = buffersUniformsVector.emplace_back(buffer.descriptor);
					if ( descriptor.offset % device->properties.limits.minUniformBufferOffsetAlignment != 0 ) {
					//	std::cout << "Unaligned offset! Expecting " << device->properties.limits.minUniformBufferOffsetAlignment << ", got " << descriptor.pBufferInfo->offset << std::endl;
						descriptor.offset = 0;
					}
				}
			}
			for ( auto& buffer : graphic.buffers ) {
				if ( buffer.usageFlags & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT ) {
					auto& descriptor = buffersStorageVector.emplace_back(buffer.descriptor);
					if ( descriptor.offset % device->properties.limits.minStorageBufferOffsetAlignment != 0 ) {
						std::cout << __LINE__ << ": Unaligned offset! Expecting " << device->properties.limits.minUniformBufferOffsetAlignment << ", got " << descriptor.offset << std::endl;
						descriptor.offset = 0;
					}
				}
				if ( buffer.usageFlags & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT ) {
					auto& descriptor = buffersUniformsVector.emplace_back(buffer.descriptor);
					if ( descriptor.offset % device->properties.limits.minUniformBufferOffsetAlignment != 0 ) {
						std::cout << __LINE__ << ": Unaligned offset! Expecting " << device->properties.limits.minUniformBufferOffsetAlignment << ", got " << descriptor.offset << std::endl;
						descriptor.offset = 0;
					}
				}
			}

			auto textures = graphic.material.textures.begin();
			auto samplers = graphic.material.samplers.begin();
			auto attachments = inputDescriptors.begin();
			auto buffersStorage = buffersStorageVector.begin();
			auto buffersUniforms = buffersUniformsVector.begin();

			for ( auto& layout : shader.descriptorSetLayoutBindings ) {

				if ( layout.descriptorCount > 1 ) {
					switch ( layout.descriptorType ) {
						case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
						case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
						case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE: {
							size_t imageInfosStart = imageInfos.size();
							// assume we have a texture, and fill it in the slots as defaults
							size_t target = layout.descriptorCount;
							for ( size_t i = 0; i < target; ++i ) {
								VkDescriptorImageInfo d = emptyTexture.descriptor;
								if ( textures != graphic.material.textures.end() ) {
									d = (textures++)->descriptor;
									if ( layout.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER && !d.sampler )
										d.sampler = emptyTexture.sampler.sampler;

									if ( d.imageLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL )
										d.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
								} else {
								//	std::cout << "textures == graphic.material.textures.end()" << std::endl;
								}
								imageInfos.push_back( d );
							}
							
							size_t len = imageInfos.size() - imageInfosStart;

							writeDescriptorSets.push_back(ext::vulkan::initializers::writeDescriptorSet(
								descriptorSet,
								layout.descriptorType,
								layout.binding,
								&imageInfos[imageInfosStart],
								len
							));
						} break;
					}
					continue;
				}

				VkDescriptorImageInfo* imageInfo = NULL;
				switch ( layout.descriptorType ) {
					case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
					case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
					case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE: {
						if ( textures == graphic.material.textures.end() ) {
							imageInfo = &emptyTexture.descriptor;
							break;
						}
						imageInfo = &((textures++)->descriptor);
					} break;
					case VK_DESCRIPTOR_TYPE_SAMPLER: {
						if ( samplers == graphic.material.samplers.end() ) {
							std::cout << "samplers == graphic.material.samplers.end()" << std::endl;
							break;
						}
						imageInfo = &((samplers++)->descriptor.info);
					} break;
					case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: {
						if ( buffersUniforms == buffersUniformsVector.end() ) {
							std::cout << "buffersUniforms == buffersUniformsVector.end()" << std::endl;
							break;
						}
						auto* descriptor = &(*(buffersUniforms++));
						if ( descriptor->offset % device->properties.limits.minUniformBufferOffsetAlignment != 0 ) {
							std::cout << __LINE__ << ": Unaligned offset! Expecting " << device->properties.limits.minUniformBufferOffsetAlignment << ", got " << descriptor->offset << std::endl;
							descriptor->offset = 0;
						}
						writeDescriptorSets.push_back(ext::vulkan::initializers::writeDescriptorSet(
							descriptorSet,
							layout.descriptorType,
							layout.binding,
							descriptor
						));
					} break;
					case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER: {
						if ( buffersStorage == buffersStorageVector.end() ) {
							std::cout << "buffersStorage == buffersStorageVector.end()" << std::endl;
							break;
						}
						auto* descriptor = &(*(buffersStorage++));
						if ( descriptor->offset % device->properties.limits.minStorageBufferOffsetAlignment != 0 ) {
							std::cout << __LINE__ << ": Unaligned offset! Expecting " << device->properties.limits.minUniformBufferOffsetAlignment << ", got " << descriptor->offset << std::endl;
							descriptor->offset = 0;
						}

						writeDescriptorSets.push_back(ext::vulkan::initializers::writeDescriptorSet(
							descriptorSet,
							layout.descriptorType,
							layout.binding,
							descriptor
						));
					} break;
					case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT: {
						if ( attachments == inputDescriptors.end() ) {
							std::cout << "attachments == inputDescriptors.end()" << std::endl;
							break;
						}
						imageInfo = &(*(attachments++));
					} break;
				}

				if ( imageInfo ) {
					if ( layout.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER || layout.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE ) {
						if ( imageInfo->imageView == VK_NULL_HANDLE ) {
							imageInfo = &emptyTexture.descriptor;
						}
					}
					if ( layout.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER || layout.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER ) {
						if ( imageInfo->sampler == VK_NULL_HANDLE ) {
							imageInfo->sampler = emptyTexture.sampler.sampler;
						}
					}
					writeDescriptorSets.push_back(ext::vulkan::initializers::writeDescriptorSet(
						descriptorSet,
						layout.descriptorType,
						layout.binding,
						imageInfo
					));
				}
			}
		}
	}

	for ( auto& descriptor : writeDescriptorSets ) {
		if ( descriptor.pBufferInfo ) {
		//	std::cout << descriptor.pBufferInfo->offset << std::endl;
			for ( size_t i = 0; i < descriptor.descriptorCount; ++i ) {
				if ( descriptor.pBufferInfo[i].offset % device->properties.limits.minUniformBufferOffsetAlignment != 0 ) {
				//	std::cout << "Unaligned offset! Expecting " << device->properties.limits.minUniformBufferOffsetAlignment << ", got " << descriptor.pBufferInfo[i].offset << std::endl;
					std::cout << "Invalid descriptor for buffer: " << descriptor.pBufferInfo[i].buffer << " " << descriptor.pBufferInfo[i].offset << " " << descriptor.pBufferInfo[i].range << ", invalidating..." << std::endl;
					auto pointer = const_cast<VkDescriptorBufferInfo*>(&descriptor.pBufferInfo[i]);
					pointer->offset = 0;
					pointer->range = 0;
					pointer->buffer = VK_NULL_HANDLE;

					// const_cast<VkDescriptorBufferInfo*>(descriptor.pBufferInfo)->offset = 0;
				}
			}
		}
	}

	// render mode's command buffer is in flight, queue a rebuild
	// ext::vulkan::rebuild = true;
	// renderMode.synchronize();
	renderMode.rebuild = true;
/*
	std::cout << this << ": " << renderMode.getName() << ": " << renderMode.getType() << " " << descriptorSet << std::endl;
	for ( auto& descriptor : writeDescriptorSets ) {
		for ( size_t i = 0; i < descriptor.descriptorCount; ++i ) {
			if ( descriptor.pImageInfo ) {
				std::cout << "[" << i << "]: " << descriptor.pImageInfo[i].imageView << std::endl;
			}
		}	
	}
*/
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
ext::vulkan::Pipeline& ext::vulkan::Graphic::getPipeline() {
	return getPipeline( descriptor );
}
ext::vulkan::Pipeline& ext::vulkan::Graphic::getPipeline( Descriptor& descriptor ) {
	if ( !hasPipeline(descriptor) ) return initializePipeline( descriptor );
	return pipelines[descriptor.hash()];
}
void ext::vulkan::Graphic::record( VkCommandBuffer commandBuffer ) {
	return this->record( commandBuffer, descriptor );
}
void ext::vulkan::Graphic::record( VkCommandBuffer commandBuffer, Descriptor& descriptor ) {
	if ( !this->hasPipeline( descriptor ) ) {
		std::cout << this << ": has no valid pipeline" << std::endl;
		return;
	}

	auto& pipeline = this->getPipeline( descriptor );

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

	ext::vulkan::rebuild = true;
}

#include <uf/utils/string/hash.h>
std::string ext::vulkan::Graphic::Descriptor::hash() const {
	uf::Serializer serializer;

	serializer["subpass"] = subpass;
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

//	std::cout << this << ": " << indices << ": " << renderMode << ": " << subpass << ": " << serializer << std::endl;
	return uf::string::sha256( serializer.serialize() );
}