#if UF_USE_VULKAN

#include <uf/ext/vulkan/graphic.h>
#include <uf/ext/vulkan/initializers.h>
#include <uf/ext/vulkan/vulkan.h>
#include <uf/ext/openvr/openvr.h>
#include <uf/utils/camera/camera.h>
#include <uf/engine/graph/graph.h>

#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_glsl.hpp>
#include <fstream>
#include <regex>

#define VK_DEBUG_VALIDATION_MESSAGE(x)\
//	VK_VALIDATION_MESSAGE(x);

namespace {
	uint32_t VERTEX_BUFFER_BIND_ID = 0;
}

void ext::vulkan::Pipeline::initialize( const Graphic& graphic ) {
	return this->initialize( graphic, graphic.descriptor );
}
void ext::vulkan::Pipeline::initialize( const Graphic& graphic, const GraphicDescriptor& descriptor ) {
	this->device = graphic.device;
	this->descriptor = descriptor;
	this->metadata.type = descriptor.pipeline;
	Device& device = *graphic.device;

	auto shaders = getShaders( graphic.material.shaders );
	assert( shaders.size() > 0 );

	uint32_t subpass = descriptor.subpass;

	RenderMode& renderMode = ext::vulkan::getRenderMode( descriptor.renderMode, true );
	auto& renderTarget = renderMode.getRenderTarget( descriptor.renderTarget );
	
	uf::stl::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings;
	uf::stl::vector<VkPushConstantRange> pushConstantRanges;
	uf::stl::vector<VkDescriptorPoolSize> poolSizes;
	uf::stl::unordered_map<VkDescriptorType, uint32_t> descriptorTypes;

	uf::stl::vector<VkPipelineColorBlendAttachmentState> blendAttachmentStates;
	uf::stl::vector<VkPipelineShaderStageCreateInfo> shaderDescriptors;
	uf::stl::vector<VkSpecializationInfo> shaderSpecializationInfos;
	uf::stl::vector<VkVertexInputBindingDescription> inputBindingDescriptions;
	uf::stl::vector<VkVertexInputAttributeDescription> attributeDescriptions;
	{

		for ( auto* shaderPointer : shaders ) {
			auto& shader = *shaderPointer;
			descriptorSetLayoutBindings.insert( descriptorSetLayoutBindings.end(), shader.descriptorSetLayoutBindings.begin(), shader.descriptorSetLayoutBindings.end() );

			size_t offset = 0;
			for ( auto& pushConstant : shader.pushConstants ) {
				size_t len = pushConstant.data().len;
				if ( len <= 0 || len > device.properties.limits.maxPushConstantsSize ) {
					VK_DEBUG_VALIDATION_MESSAGE("Invalid push constant length of " << len << " for shader " << shader.filename);
				//	goto PIPELINE_INITIALIZATION_INVALID;
					len = device.properties.limits.maxPushConstantsSize;
				}
				pushConstantRanges.emplace_back(ext::vulkan::initializers::pushConstantRange(
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
			poolSizes.emplace_back(ext::vulkan::initializers::descriptorPoolSize(pair.first, pair.second));
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
	for ( auto* shaderPointer : shaders ) {
		auto& shader = *shaderPointer;
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
		rasterizationState.depthBiasEnable = graphic.descriptor.depth.bias.enable;
		rasterizationState.depthBiasConstantFactor = graphic.descriptor.depth.bias.constant;
		rasterizationState.depthBiasSlopeFactor = graphic.descriptor.depth.bias.slope;
		rasterizationState.depthBiasClamp = graphic.descriptor.depth.bias.clamp;

		VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
		if ( renderMode.getType() != "Swapchain" ) {
			auto& subpass = renderTarget.passes[descriptor.subpass];
			for ( auto& color : subpass.colors ) {
				auto& attachment = renderTarget.attachments[color.attachment];
				blendAttachmentStates.emplace_back(attachment.blendState);
				samples = std::max(samples, ext::vulkan::sampleCount( attachment.descriptor.samples ));
			}
			// require blending if independentBlend is not an enabled feature
			if ( device.enabledFeatures.independentBlend == VK_FALSE ) {
				for ( size_t i = 1; i < blendAttachmentStates.size(); ++i ) {
					blendAttachmentStates[i] = blendAttachmentStates[0];
				}
			}
		} else {
			subpass = 0;
			VkBool32 blendEnabled = VK_TRUE;
			VkColorComponentFlags writeMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

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
			blendAttachmentStates.emplace_back(blendAttachmentState);
		}

		VkPipelineColorBlendStateCreateInfo colorBlendState = ext::vulkan::initializers::pipelineColorBlendStateCreateInfo(
			blendAttachmentStates.size(),
			blendAttachmentStates.data()
		);
		VkPipelineDepthStencilStateCreateInfo depthStencilState = ext::vulkan::initializers::pipelineDepthStencilStateCreateInfo(
			descriptor.depth.test,
			descriptor.depth.write,
			descriptor.depth.operation
		);
		if ( uf::matrix::reverseInfiniteProjection ) {
			depthStencilState.depthCompareOp = ext::vulkan::enums::Compare::GREATER_OR_EQUAL;
			depthStencilState.minDepthBounds = 1.0f;
			depthStencilState.maxDepthBounds = 0.0f;
		} else {
			depthStencilState.depthCompareOp = ext::vulkan::enums::Compare::LESS;
			depthStencilState.minDepthBounds = 0.0f;
			depthStencilState.maxDepthBounds = 1.0f;
		}
		VkPipelineViewportStateCreateInfo viewportState = ext::vulkan::initializers::pipelineViewportStateCreateInfo(
			1, 1, 0
		);
		VkPipelineMultisampleStateCreateInfo multisampleState = ext::vulkan::initializers::pipelineMultisampleStateCreateInfo(
			samples,
			0
		);
		if ( samples > 1 && device.features.sampleRateShading ) {
			multisampleState.sampleShadingEnable = VK_TRUE;
			multisampleState.minSampleShading = 0.25f;
		}

		uf::stl::vector<VkDynamicState> dynamicStateEnables = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineDynamicStateCreateInfo dynamicState = ext::vulkan::initializers::pipelineDynamicStateCreateInfo(
			dynamicStateEnables.data(),
			dynamicStateEnables.size(),
			0
		);

		// Vertex binding description
		size_t vertexBindID = 0;
		size_t vertexLocationID = 0;
		if ( !descriptor.inputs.vertex.attributes.empty() ) {
			if ( 0 <= descriptor.inputs.vertex.interleaved ) {
				inputBindingDescriptions.emplace_back(ext::vulkan::initializers::vertexInputBindingDescription(
					vertexBindID, // descriptor.inputs.vertex.interleaved, 
					descriptor.inputs.vertex.size, 
					VK_VERTEX_INPUT_RATE_VERTEX
				));
				for ( auto& attribute : descriptor.inputs.vertex.attributes ) {
					attributeDescriptions.emplace_back(ext::vulkan::initializers::vertexInputAttributeDescription(
						vertexBindID,
						vertexLocationID++,
						attribute.descriptor.format,
						attribute.descriptor.offset
					));
				}
				++vertexBindID;
			} else for ( auto& attribute : descriptor.inputs.vertex.attributes ) {
				inputBindingDescriptions.emplace_back(ext::vulkan::initializers::vertexInputBindingDescription(
					vertexBindID, // attribute.buffer, 
					attribute.descriptor.size, 
					VK_VERTEX_INPUT_RATE_VERTEX
				));
				attributeDescriptions.emplace_back(ext::vulkan::initializers::vertexInputAttributeDescription(
					vertexBindID,
					vertexLocationID++,
					attribute.descriptor.format,
					0 // attribute.descriptor.offset
				));
				++vertexBindID;
			}
		}

		VkPipelineVertexInputStateCreateInfo vertexInputState = ext::vulkan::initializers::pipelineVertexInputStateCreateInfo();
		vertexInputState.vertexBindingDescriptionCount = inputBindingDescriptions.size();
		vertexInputState.pVertexBindingDescriptions = inputBindingDescriptions.data();
		vertexInputState.vertexAttributeDescriptionCount = attributeDescriptions.size();
		vertexInputState.pVertexAttributeDescriptions = attributeDescriptions.data();

		for ( auto* shader : shaders ) shaderDescriptors.emplace_back(shader->descriptor);

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
	return;

PIPELINE_INITIALIZATION_INVALID:
	VK_DEBUG_VALIDATION_MESSAGE("Pipeline initialization invalid, updating next tick...");
	uf::thread::queue( uf::thread::get("Main"), [&]{
		this->initialize( graphic, descriptor );
	});
	return;
}
void ext::vulkan::Pipeline::record( const Graphic& graphic, VkCommandBuffer commandBuffer, size_t pass, size_t draw ) const {
	return record( graphic, descriptor, commandBuffer, pass, draw );
}
void ext::vulkan::Pipeline::record( const Graphic& graphic, const GraphicDescriptor& descriptor, VkCommandBuffer commandBuffer, size_t pass, size_t draw ) const {
	auto bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	auto shaders = getShaders( graphic.material.shaders );
	for ( auto* shader : shaders ) {
		if ( shader->descriptor.stage == VK_SHADER_STAGE_COMPUTE_BIT ) bindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
	#if 1
		if ( shader->metadata.definitions.pushConstants.count("PushConstant") > 0 ) {
			if ( shader->descriptor.stage == VK_SHADER_STAGE_VERTEX_BIT || shader->descriptor.stage == VK_SHADER_STAGE_COMPUTE_BIT ) {
				struct PushConstant {
					uint32_t pass;
					uint32_t draw;
				} pushConstant = { pass, draw };
				vkCmdPushConstants( commandBuffer, pipelineLayout, shader->descriptor.stage, 0, sizeof(pushConstant), &pushConstant );
			}
		} else
	#endif
		if ( !shader->pushConstants.empty() ) {
			auto& pushConstant = shader->pushConstants.front();
			size_t size = pushConstant.size();
			void* data = pushConstant.data().data;
			if ( size && data ) {
				vkCmdPushConstants( commandBuffer, pipelineLayout, shader->descriptor.stage, 0, size, data );
			}
		}
	/*
		size_t offset = 0;
		for ( auto& pushConstant : shader->pushConstants ) {
			if ( shader->metadata.definitions.pushConstants.count("PushConstant") > 0 ) {
				if ( shader->descriptor.stage == VK_SHADER_STAGE_VERTEX_BIT ) {
					struct PushConstant {
						uint32_t pass;
						uint32_t draw;
					} pushConstant = { pass, draw };
						( commandBuffer, pipelineLayout, shader->descriptor.stage, 0, sizeof(pushConstant), &pushConstant );
				}
			} else {
				size_t len = pushConstant.data().len;
				void* pointer = pushConstant.data().data;
				if ( len > 0 && pointer ) {
					vkCmdPushConstants( commandBuffer, pipelineLayout, shader->descriptor.stage, 0, len, pointer );
				}
			}
		}
	*/
	}
	// Bind descriptor sets describing shader binding points
	vkCmdBindDescriptorSets(commandBuffer, bindPoint, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
	// Bind the rendering pipeline
	// The pipeline (state object) contains all states of the rendering pipeline, binding it will set all the states specified at pipeline creation time
	vkCmdBindPipeline(commandBuffer, bindPoint, pipeline);

	if ( bindPoint == VK_PIPELINE_BIND_POINT_COMPUTE && descriptor.inputs.dispatch.x != 0 && descriptor.inputs.dispatch.y != 0 && descriptor.inputs.dispatch.z != 0 ) {
		vkCmdDispatch(commandBuffer, descriptor.inputs.dispatch.x, descriptor.inputs.dispatch.y, descriptor.inputs.dispatch.z);
	}
}
void ext::vulkan::Pipeline::update( const Graphic& graphic ) {
	return this->update( graphic, descriptor );
}
void ext::vulkan::Pipeline::update( const Graphic& graphic, const GraphicDescriptor& descriptor ) {
	//
	if ( descriptorSet == VK_NULL_HANDLE ) return;
	this->descriptor = descriptor;

	RenderMode& renderMode = ext::vulkan::getRenderMode(descriptor.renderMode, true);
	auto& renderTarget = renderMode.getRenderTarget(descriptor.renderTarget );


	auto shaders = getShaders( graphic.material.shaders );
	uf::stl::vector<VkWriteDescriptorSet> writeDescriptorSets;

	struct Infos {
		uf::stl::vector<VkDescriptorBufferInfo> uniform;
		uf::stl::vector<VkDescriptorBufferInfo> storage;
		
		uf::stl::vector<VkDescriptorImageInfo> image;
		uf::stl::vector<VkDescriptorImageInfo> image2D;
		uf::stl::vector<VkDescriptorImageInfo> image2DA;
		uf::stl::vector<VkDescriptorImageInfo> imageCube;
		uf::stl::vector<VkDescriptorImageInfo> image3D;
		uf::stl::vector<VkDescriptorImageInfo> imageUnknown;

		uf::stl::vector<VkDescriptorImageInfo> sampler;
		uf::stl::vector<VkDescriptorImageInfo> input;
	};
	uf::stl::vector<Infos> INFOS; INFOS.reserve( shaders.size() );

	for ( auto* shader : shaders ) {
		auto& infos = INFOS.emplace_back();
		uf::stl::vector<ext::vulkan::enums::Image::viewType_t> types;

		// add per-rendermode buffers
		for ( auto& buffer : renderMode.buffers ) {
			if ( buffer.usage & uf::renderer::enums::Buffer::UNIFORM ) infos.uniform.emplace_back(buffer.descriptor);
			if ( buffer.usage & uf::renderer::enums::Buffer::STORAGE ) infos.storage.emplace_back(buffer.descriptor);
		}
		// add per-shader buffers
		for ( auto& buffer : shader->buffers ) {
			if ( buffer.usage & uf::renderer::enums::Buffer::UNIFORM ) infos.uniform.emplace_back(buffer.descriptor);
			if ( buffer.usage & uf::renderer::enums::Buffer::STORAGE ) infos.storage.emplace_back(buffer.descriptor);
		}
		// add per-pipeline buffers
		for ( auto& buffer : this->buffers ) {
			if ( buffer.usage & uf::renderer::enums::Buffer::UNIFORM ) infos.uniform.emplace_back(buffer.descriptor);
			if ( buffer.usage & uf::renderer::enums::Buffer::STORAGE ) infos.storage.emplace_back(buffer.descriptor);
		}

		if ( descriptor.subpass < renderTarget.passes.size() ) {
			auto& subpass = renderTarget.passes[descriptor.subpass];
			for ( auto& input : subpass.inputs ) {
				infos.input.emplace_back(ext::vulkan::initializers::descriptorImageInfo( 
					renderTarget.attachments[input.attachment].views[subpass.layer],
					input.layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : input.layout
				));
			}
		}

		auto& textures = !shader->textures.empty() ? shader->textures : graphic.material.textures;
		for ( auto& texture : textures ) {
			infos.image.emplace_back(texture.descriptor);
			switch ( texture.viewType ) {
				case VK_IMAGE_VIEW_TYPE_2D: infos.image2D.emplace_back(texture.descriptor); break;
				case VK_IMAGE_VIEW_TYPE_2D_ARRAY: infos.image2DA.emplace_back(texture.descriptor); break;
				case VK_IMAGE_VIEW_TYPE_CUBE: infos.imageCube.emplace_back(texture.descriptor); break;
				case VK_IMAGE_VIEW_TYPE_3D: infos.image3D.emplace_back(texture.descriptor); break;
				default: infos.imageUnknown.emplace_back(texture.descriptor); break;
			}
		}
		for ( auto& sampler : graphic.material.samplers ) {
			infos.sampler.emplace_back(sampler.descriptor.info);
		}

		// check if we can even consume that many infos
		size_t consumes = 0;
		for ( auto& layout : shader->descriptorSetLayoutBindings ) {
			switch ( layout.descriptorType ) {
				// consume an texture image info
				case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
				case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
				case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE: {
					consumes += layout.descriptorCount;				
					ext::vulkan::enums::Image::viewType_t imageType = shader->metadata.definitions.textures.at(layout.binding).type;
					types.reserve(consumes);
					for ( size_t i = 0; i < layout.descriptorCount; ++i ) types.emplace_back(imageType);
				} break;
			}
		}
		
		size_t maxTextures2D = 0;
		size_t maxTextures2DA = 0;
		size_t maxTextures3D = 0;
		size_t maxTexturesCube = 0;
		size_t maxTexturesUnknown = 0;
		for ( auto& type : types ) {
			if ( type == ext::vulkan::enums::Image::VIEW_TYPE_3D ) ++maxTextures3D;
			else if ( type == ext::vulkan::enums::Image::VIEW_TYPE_CUBE ) ++maxTexturesCube;
			else if ( type == ext::vulkan::enums::Image::VIEW_TYPE_2D ) ++maxTextures2D;
			else if ( type == ext::vulkan::enums::Image::VIEW_TYPE_2D_ARRAY ) ++maxTextures2DA;
			else ++maxTexturesUnknown;
		}

		while ( infos.image2D.size() < maxTextures2D ) infos.image2D.emplace_back(Texture2D::empty.descriptor);
		while ( infos.image2DA.size() < maxTextures2DA ) infos.image2DA.emplace_back(Texture2D::empty.descriptor);
		while ( infos.imageCube.size() < maxTexturesCube ) infos.imageCube.emplace_back(TextureCube::empty.descriptor);
		while ( infos.image3D.size() < maxTextures3D ) infos.image3D.emplace_back(Texture3D::empty.descriptor);
		while ( infos.imageUnknown.size() < maxTexturesUnknown ) infos.imageUnknown.emplace_back(Texture2D::empty.descriptor);

		for ( size_t i = infos.image.size(); i < consumes; ++i ) {
			ext::vulkan::enums::Image::viewType_t type = i < types.size() ? types[i] : ext::vulkan::enums::Image::viewType_t{};
			if ( type == ext::vulkan::enums::Image::VIEW_TYPE_3D ) infos.image.emplace_back(Texture3D::empty.descriptor);
			else if ( type == ext::vulkan::enums::Image::VIEW_TYPE_CUBE ) infos.image.emplace_back(TextureCube::empty.descriptor);
			else if ( type == ext::vulkan::enums::Image::VIEW_TYPE_2D ) infos.image.emplace_back(Texture2D::empty.descriptor);
			else infos.image.emplace_back(Texture2D::empty.descriptor);
		}
		
		auto uniformBufferInfo = infos.uniform.begin();
		auto storageBufferInfo = infos.storage.begin();
		
		auto imageInfo = infos.image.begin();
		auto image2DInfo = infos.image2D.begin();
		auto image2DAInfo = infos.image2DA.begin();
		auto imageCubeInfo = infos.imageCube.begin();
		auto image3DInfo = infos.image3D.begin();
		auto imageUnknownInfo = infos.imageUnknown.begin();

		auto samplerInfo = infos.sampler.begin();
		auto inputInfo = infos.input.begin();

		for ( auto& layout : shader->descriptorSetLayoutBindings ) {
			switch ( layout.descriptorType ) {
				// consume an texture image info
				case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
				case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
				case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE: {
					ext::vulkan::enums::Image::viewType_t imageType = shader->metadata.definitions.textures.at(layout.binding).type;
					if ( imageType == ext::vulkan::enums::Image::VIEW_TYPE_2D ) {
						UF_ASSERT_BREAK_MSG( image2DInfo != infos.image2D.end(), "Filename: " << shader->filename << "\tCount: " << layout.descriptorCount )
						writeDescriptorSets.emplace_back(ext::vulkan::initializers::writeDescriptorSet(
							descriptorSet,
							layout.descriptorType,
							layout.binding,
							&(*image2DInfo),
							layout.descriptorCount
						));
						image2DInfo += layout.descriptorCount;
					} else if ( imageType == ext::vulkan::enums::Image::VIEW_TYPE_2D_ARRAY ) {
						UF_ASSERT_BREAK_MSG( image2DAInfo != infos.image2DA.end(), "Filename: " << shader->filename << "\tCount: " << layout.descriptorCount )
						writeDescriptorSets.emplace_back(ext::vulkan::initializers::writeDescriptorSet(
							descriptorSet,
							layout.descriptorType,
							layout.binding,
							&(*image2DAInfo),
							layout.descriptorCount
						));
						image2DAInfo += layout.descriptorCount;
					} else if ( imageType == ext::vulkan::enums::Image::VIEW_TYPE_CUBE ) {
						UF_ASSERT_BREAK_MSG( imageCubeInfo != infos.imageCube.end(), "Filename: " << shader->filename << "\tCount: " << layout.descriptorCount )
						writeDescriptorSets.emplace_back(ext::vulkan::initializers::writeDescriptorSet(
							descriptorSet,
							layout.descriptorType,
							layout.binding,
							&(*imageCubeInfo),
							layout.descriptorCount
						));
						imageCubeInfo += layout.descriptorCount;
					} else if ( imageType == ext::vulkan::enums::Image::VIEW_TYPE_3D ) {
						UF_ASSERT_BREAK_MSG( image3DInfo != infos.image3D.end(), "Filename: " << shader->filename << "\tCount: " << layout.descriptorCount )
						writeDescriptorSets.emplace_back(ext::vulkan::initializers::writeDescriptorSet(
							descriptorSet,
							layout.descriptorType,
							layout.binding,
							&(*image3DInfo),
							layout.descriptorCount
						));
						image3DInfo += layout.descriptorCount;
					} else {
						UF_ASSERT_BREAK_MSG( imageUnknownInfo != infos.imageUnknown.end(), "Filename: " << shader->filename << "\tCount: " << layout.descriptorCount )
						writeDescriptorSets.emplace_back(ext::vulkan::initializers::writeDescriptorSet(
							descriptorSet,
							layout.descriptorType,
							layout.binding,
							&(*imageUnknownInfo),
							layout.descriptorCount
						));
						imageUnknownInfo += layout.descriptorCount;
					}
				} break;
				case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT: {
					UF_ASSERT_BREAK_MSG( inputInfo != infos.input.end(), "Filename: " << shader->filename << "\tCount: " << layout.descriptorCount )
					writeDescriptorSets.emplace_back(ext::vulkan::initializers::writeDescriptorSet(
						descriptorSet,
						layout.descriptorType,
						layout.binding,
						&(*inputInfo),
						layout.descriptorCount
					));
					inputInfo += layout.descriptorCount;
				} break;
				case VK_DESCRIPTOR_TYPE_SAMPLER: {
					UF_ASSERT_BREAK_MSG( samplerInfo != infos.sampler.end(), "Filename: " << shader->filename << "\tCount: " << layout.descriptorCount )
					writeDescriptorSets.emplace_back(ext::vulkan::initializers::writeDescriptorSet(
						descriptorSet,
						layout.descriptorType,
						layout.binding,
						&(*samplerInfo),
						layout.descriptorCount
					));
					samplerInfo += layout.descriptorCount;
				} break;
				case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: {
					UF_ASSERT_BREAK_MSG( uniformBufferInfo != infos.uniform.end(), "Filename: " << shader->filename << "\tCount: " << layout.descriptorCount )
					writeDescriptorSets.emplace_back(ext::vulkan::initializers::writeDescriptorSet(
						descriptorSet,
						layout.descriptorType,
						layout.binding,
						&(*uniformBufferInfo),
						layout.descriptorCount
					));
					uniformBufferInfo += layout.descriptorCount;
				} break;
				case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER: {
					UF_ASSERT_BREAK_MSG( storageBufferInfo != infos.storage.end(), "Filename: " << shader->filename << "\tCount: " << layout.descriptorCount )
					writeDescriptorSets.emplace_back(ext::vulkan::initializers::writeDescriptorSet(
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

	// validate writeDescriptorSets
	for ( auto& descriptor : writeDescriptorSets ) {
		for ( size_t i = 0; i < descriptor.descriptorCount; ++i ) {
			if ( descriptor.pBufferInfo ) {
				if ( descriptor.pBufferInfo[i].offset % device->properties.limits.minUniformBufferOffsetAlignment != 0 ) {
					VK_DEBUG_VALIDATION_MESSAGE("Invalid descriptor for buffer: " << descriptor.pBufferInfo[i].buffer << " (Offset: " << descriptor.pBufferInfo[i].offset << ", Range: " << descriptor.pBufferInfo[i].range << "), invalidating...");
					goto PIPELINE_UPDATE_INVALID;
				}
			}
			if ( descriptor.pImageInfo ) {
				if (  	descriptor.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ||
						descriptor.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE ||
						descriptor.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE ||
						descriptor.descriptorType == VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT
				 ) {
					if ( descriptor.pImageInfo[i].imageView == VK_NULL_HANDLE || descriptor.pImageInfo[i].imageLayout == VK_IMAGE_LAYOUT_UNDEFINED ) {
						VK_DEBUG_VALIDATION_MESSAGE("Null image view or layout is undefined, replacing with fallback texture...");
						auto pointer = const_cast<VkDescriptorImageInfo*>(&descriptor.pImageInfo[i]);
						ext::vulkan::enums::Image::viewType_t imageType{};
						for ( auto* shader : shaders ) {
							if ( shader->metadata.definitions.textures.count(descriptor.dstBinding) == 0  ) continue;
							imageType = shader->metadata.definitions.textures.at(descriptor.dstBinding).type;
							break;
						}
						if ( imageType == ext::vulkan::enums::Image::VIEW_TYPE_3D ) {
							*pointer = Texture3D::empty.descriptor;
						} else if ( imageType == ext::vulkan::enums::Image::VIEW_TYPE_CUBE ) {
							*pointer = TextureCube::empty.descriptor;
						} else {
							*pointer = Texture2D::empty.descriptor;
						}
					}
					if ( descriptor.pImageInfo[i].imageLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ) {
						VK_DEBUG_VALIDATION_MESSAGE("Image layout is VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, fixing to VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL");
						auto pointer = const_cast<VkDescriptorImageInfo*>(&descriptor.pImageInfo[i]);
						pointer->imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
					}
				}
				if ( descriptor.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER || descriptor.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ) {
					if ( !descriptor.pImageInfo[i].sampler ) {
						VK_DEBUG_VALIDATION_MESSAGE("Image descriptor type is VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, yet lacks a sampler, adding default sampler...");
						auto pointer = const_cast<VkDescriptorImageInfo*>(&descriptor.pImageInfo[i]);
						pointer->sampler = Texture2D::empty.sampler.sampler;
					}
				}
			}
		}
	}

	renderMode.rebuild = true;

	vkUpdateDescriptorSets(
		*device,
		writeDescriptorSets.size(),
		writeDescriptorSets.data(),
		0,
		NULL
	);
	return;

PIPELINE_UPDATE_INVALID:
//	graphic.process = false;
	VK_DEBUG_VALIDATION_MESSAGE("Pipeline update invalid, updating next tick...");
	uf::thread::queue( uf::thread::get("Main"), [&]{
		this->update( graphic, descriptor );
	});
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

	if ( settings::experimental::dedicatedThread ) ext::vulkan::states::rebuild = true;
}
uf::stl::vector<ext::vulkan::Shader*> ext::vulkan::Pipeline::getShaders( uf::stl::vector<ext::vulkan::Shader>& shaders ) {
	uf::stl::unordered_map<uf::stl::string, ext::vulkan::Shader*> map;
	uf::stl::vector<ext::vulkan::Shader*> res;
	bool isCompute = false;
	for ( auto& shader : shaders ) {
		if ( shader.metadata.pipeline != "" && shader.metadata.pipeline != metadata.type ) continue;
		if ( shader.descriptor.stage == VK_SHADER_STAGE_COMPUTE_BIT ) isCompute = true;
	}
	for ( auto& shader : shaders ) {
		if ( shader.metadata.pipeline != "" && shader.metadata.pipeline != metadata.type ) continue;
		if ( isCompute && shader.descriptor.stage != VK_SHADER_STAGE_COMPUTE_BIT ) continue;
		map[shader.metadata.type] = &shader;
	}
	for ( auto pair : map ) res.insert( res.begin(), pair.second);
	return res;
}
uf::stl::vector<const ext::vulkan::Shader*> ext::vulkan::Pipeline::getShaders( const uf::stl::vector<ext::vulkan::Shader>& shaders ) const {
	uf::stl::unordered_map<uf::stl::string, const ext::vulkan::Shader*> map;
	uf::stl::vector<const ext::vulkan::Shader*> res;
	bool isCompute = false;
	for ( auto& shader : shaders ) {
		if ( shader.metadata.pipeline != "" && shader.metadata.pipeline != metadata.type ) continue;
		if ( shader.descriptor.stage == VK_SHADER_STAGE_COMPUTE_BIT ) isCompute = true;
	}
	for ( auto& shader : shaders ) {
		if ( shader.metadata.pipeline != "" && shader.metadata.pipeline != metadata.type ) continue;
		if ( isCompute && shader.descriptor.stage != VK_SHADER_STAGE_COMPUTE_BIT ) continue;
		map[shader.metadata.type] = &shader;
	}
	for ( auto pair : map ) res.insert( res.begin(), pair.second);
	return res;
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
void ext::vulkan::Material::attachShader( const uf::stl::string& filename, VkShaderStageFlagBits stage, const uf::stl::string& pipeline ) {
	auto& shader = shaders.emplace_back();
	shader.metadata.json = metadata.json["shader"];
	shader.metadata.autoInitializeUniforms = metadata.autoInitializeUniforms;
	shader.initialize( *device, filename, stage );

	// repoint our specialization info descriptor because our shaders will change memory locations when attaching one by one

	for ( auto& shader : shaders ) {
		shader.specializationInfo.mapEntryCount = shader.specializationMapEntries.size();
		shader.specializationInfo.pMapEntries = shader.specializationMapEntries.data();
		shader.specializationInfo.dataSize = shader.specializationConstants.size();
		shader.specializationInfo.pData = (void*) shader.specializationConstants;
		shader.descriptor.pSpecializationInfo = &shader.specializationInfo;
	}

	// check how many samplers requested
	for ( auto& layout : shader.descriptorSetLayoutBindings ) {
		if ( layout.descriptorType != VK_DESCRIPTOR_TYPE_SAMPLER ) continue;
		Sampler& sampler = samplers.emplace_back();
		sampler.initialize( *device );
	}
	
	uf::stl::string type = "unknown";
	switch ( stage ) {
		case VK_SHADER_STAGE_VERTEX_BIT: type = "vertex"; break;
		case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT: type = "tessellation_control"; break;
		case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT: type = "tessellation_evaluation"; break;
		case VK_SHADER_STAGE_GEOMETRY_BIT: type = "geometry"; break;
		case VK_SHADER_STAGE_FRAGMENT_BIT: type = "fragment"; break;
		case VK_SHADER_STAGE_COMPUTE_BIT: type = "compute"; break;
		case VK_SHADER_STAGE_ALL_GRAPHICS: type = "all_graphics"; break;
		case VK_SHADER_STAGE_ALL: type = "all"; break;
		case VK_SHADER_STAGE_RAYGEN_BIT_KHR: type = "raygen"; break;
		case VK_SHADER_STAGE_ANY_HIT_BIT_KHR: type = "any_hit"; break;
		case VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR: type = "closest_hit"; break;
		case VK_SHADER_STAGE_MISS_BIT_KHR: type = "miss"; break;
		case VK_SHADER_STAGE_INTERSECTION_BIT_KHR: type = "intersection"; break;
		case VK_SHADER_STAGE_CALLABLE_BIT_KHR: type = "callable"; break;
	}
	shader.metadata.pipeline = pipeline;
	shader.metadata.type = type;

	metadata.json["shaders"][pipeline][type]["index"] = shaders.size() - 1;
	metadata.json["shaders"][pipeline][type]["filename"] = filename;
	metadata.shaders[pipeline+":"+type] = shaders.size() - 1;
}
void ext::vulkan::Material::initializeShaders( const uf::stl::vector<std::pair<uf::stl::string, VkShaderStageFlagBits>>& layout, const uf::stl::string& pipeline ) {
	shaders.clear(); shaders.reserve( layout.size() );
	for ( auto& request : layout ) {
		attachShader( request.first, request.second, pipeline );
	}
}
bool ext::vulkan::Material::hasShader( const uf::stl::string& type, const uf::stl::string& pipeline ) const {
	return metadata.shaders.count(pipeline+":"+type) > 0;
}
ext::vulkan::Shader& ext::vulkan::Material::getShader( const uf::stl::string& type, const uf::stl::string& pipeline ) {
	UF_ASSERT( hasShader(type, pipeline) );
	return shaders.at( metadata.shaders[pipeline+":"+type] );
}
const ext::vulkan::Shader& ext::vulkan::Material::getShader( const uf::stl::string& type, const uf::stl::string& pipeline ) const {
	UF_ASSERT( hasShader(type, pipeline) );
	return shaders.at( metadata.shaders.at(pipeline+":"+type) );
}
bool ext::vulkan::Material::validate() {
	bool was = true;
	for ( auto& shader : shaders ) if ( !shader.validate() ) was = false;
	return was;
}
ext::vulkan::Graphic::~Graphic() {
	this->destroy();
}
void ext::vulkan::Graphic::initialize( const uf::stl::string& renderModeName ) {
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
ext::vulkan::Pipeline& ext::vulkan::Graphic::initializePipeline( const GraphicDescriptor& descriptor, bool update ) {
	auto& pipeline = pipelines[descriptor.hash()];

	pipeline.initialize(*this, descriptor);
	pipeline.update(*this, descriptor);

	initialized = true;
	material.validate();

	return pipeline;
}
void ext::vulkan::Graphic::initializeMesh( uf::Mesh& mesh, bool buffer ) {
	// generate indices if not found
//	if ( mesh.index.count == 0 ) mesh.generateIndices();
	// generate indirect data if not found
//	if ( mesh.indirect.count == 0  ) mesh.generateIndirect();
	// ensure our descriptors are proper
	mesh.updateDescriptor();

	// copy descriptors
	descriptor.inputs.vertex = mesh.vertex;
	descriptor.inputs.index = mesh.index;
	descriptor.inputs.instance = mesh.instance;
	descriptor.inputs.indirect = mesh.indirect;

	// create buffer if not set and requested
	if ( buffer ) {
		// ensures each buffer index reflects nicely
		struct Queue {
			void* data;
			size_t size;
			VkBufferUsageFlags usage;
		};
		uf::stl::vector<Queue> queue;
		descriptor.inputs.bufferOffset = buffers.empty() ? 0 : buffers.size() - 1;

		#define PARSE_ATTRIBUTE(i, usage) {\
			auto& buffer = mesh.buffers[i];\
			if ( queue.size() <= i ) queue.resize( i );\
			if ( !buffer.empty() ) queue.emplace_back(Queue{ (void*) buffer.data(), buffer.size(), usage });\
		}
		#define PARSE_INPUT(name, usage){\
			if ( mesh.isInterleaved( mesh.name.interleaved ) ) PARSE_ATTRIBUTE(descriptor.inputs.name.interleaved, usage)\
			else for ( auto& attribute : descriptor.inputs.name.attributes ) PARSE_ATTRIBUTE(attribute.buffer, usage)\
		}

		PARSE_INPUT(vertex, uf::renderer::enums::Buffer::VERTEX)
		PARSE_INPUT(index, uf::renderer::enums::Buffer::INDEX)
		PARSE_INPUT(instance, uf::renderer::enums::Buffer::VERTEX)
		PARSE_INPUT(indirect, uf::renderer::enums::Buffer::INDIRECT | uf::renderer::enums::Buffer::STORAGE)

		// allocate buffers
		for ( auto i = 0; i < queue.size(); ++i ) {
			auto& q = queue[i];
			initializeBuffer( q.data, q.size, q.usage );
		}
	}

	if ( mesh.instance.count == 0 && mesh.instance.attributes.empty() ) {
		descriptor.inputs.instance.count = 1;
	}
}
bool ext::vulkan::Graphic::updateMesh( uf::Mesh& mesh ) {
	// generate indices if not found
//	if ( mesh.index.count == 0 ) mesh.generateIndices();
	// generate indirect data if not found
//	if ( mesh.indirect.count == 0  ) mesh.generateIndirect();
	// ensure our descriptors are proper
	mesh.updateDescriptor();

	// copy descriptors
	descriptor.inputs.vertex = mesh.vertex;
	descriptor.inputs.index = mesh.index;
	descriptor.inputs.instance = mesh.instance;
	descriptor.inputs.indirect = mesh.indirect;

	// create buffer if not set and requested
	// ensures each buffer index reflects nicely
	struct Queue {
		void* data;
		size_t size;
		uf::renderer::enums::Buffer::type_t usage;
	};
	uf::stl::vector<Queue> queue;

	#define PARSE_ATTRIBUTE(i, usage) {\
		auto& buffer = mesh.buffers[i];\
		if ( queue.size() <= i ) queue.resize( i );\
		if ( !buffer.empty() ) queue.emplace_back(Queue{ (void*) buffer.data(), buffer.size(), usage });\
	}
	#define PARSE_INPUT(name, usage){\
		if ( mesh.isInterleaved( mesh.name.interleaved ) ) PARSE_ATTRIBUTE(descriptor.inputs.name.interleaved, usage)\
		else for ( auto& attribute : descriptor.inputs.name.attributes ) PARSE_ATTRIBUTE(attribute.buffer, usage)\
	}

	PARSE_INPUT(vertex, uf::renderer::enums::Buffer::VERTEX)
	PARSE_INPUT(index, uf::renderer::enums::Buffer::INDEX)
	PARSE_INPUT(instance, uf::renderer::enums::Buffer::VERTEX)
	PARSE_INPUT(indirect, uf::renderer::enums::Buffer::INDIRECT)

	// allocate buffers
	bool rebuild = false;
	for ( auto i = 0; i < queue.size(); ++i ) {
		auto& q = queue[i];
		rebuild = rebuild || updateBuffer( q.data, q.size, descriptor.inputs.bufferOffset + i );
	}

	if ( mesh.instance.count == 0 && mesh.instance.attributes.empty() ) {
		descriptor.inputs.instance.count = 1;
	}
	return rebuild;
}
bool ext::vulkan::Graphic::hasPipeline( const GraphicDescriptor& descriptor ) const {
	return pipelines.count( descriptor.hash() ) > 0;
}
ext::vulkan::Pipeline& ext::vulkan::Graphic::getPipeline() {
	return getPipeline( descriptor );
}
const ext::vulkan::Pipeline& ext::vulkan::Graphic::getPipeline() const {
	return getPipeline( descriptor );
}
ext::vulkan::Pipeline& ext::vulkan::Graphic::getPipeline( const GraphicDescriptor& descriptor ) {
	if ( !hasPipeline(descriptor) ) return initializePipeline( descriptor );
	return pipelines[descriptor.hash()];
}
const ext::vulkan::Pipeline& ext::vulkan::Graphic::getPipeline( const GraphicDescriptor& descriptor ) const {
	if ( !hasPipeline(descriptor) ) UF_EXCEPTION("does not have pipeline");
	return pipelines.at(descriptor.hash());
}
void ext::vulkan::Graphic::updatePipelines() {
	for ( auto pair : this->pipelines ) pair.second.update( *this );
}
void ext::vulkan::Graphic::record( VkCommandBuffer commandBuffer, size_t pass, size_t draw ) const {
	return this->record( commandBuffer, descriptor, pass, draw );
}
void ext::vulkan::Graphic::record( VkCommandBuffer commandBuffer, const GraphicDescriptor& descriptor, size_t pass, size_t draw ) const {
	if ( !process ) return;
	if ( !this->hasPipeline( descriptor ) ) {
		VK_DEBUG_VALIDATION_MESSAGE(this << ": has no valid pipeline");
		return;
	}

	auto& pipeline = this->getPipeline( descriptor );
	if ( pipeline.descriptorSet == VK_NULL_HANDLE ) {
		VK_DEBUG_VALIDATION_MESSAGE(this << ": has no valid pipeline descriptor set");
		return;
	}
	if ( !pipeline.metadata.process ) return;
	pipeline.record(*this, descriptor, commandBuffer, pass, draw);

	auto shaders = pipeline.getShaders( material.shaders );
	for ( auto* shader : shaders ) if ( shader->descriptor.stage == VK_SHADER_STAGE_COMPUTE_BIT ) return;

	struct {
		uf::stl::vector<VkBuffer> buffer;
		uf::stl::vector<VkDeviceSize> offset;
	} vertexInstance;

	for ( auto& attribute : descriptor.inputs.vertex.attributes ) {
		vertexInstance.buffer.emplace_back( buffers.at((0 <= descriptor.inputs.vertex.interleaved ? descriptor.inputs.vertex.interleaved : attribute.buffer) + descriptor.inputs.bufferOffset).buffer );
		vertexInstance.offset.emplace_back( 0 <= descriptor.inputs.vertex.interleaved ? descriptor.inputs.vertex.offset : attribute.offset );
	}
	for ( auto& attribute : descriptor.inputs.instance.attributes ) {
		vertexInstance.buffer.emplace_back( buffers.at((0 <= descriptor.inputs.instance.interleaved ? descriptor.inputs.instance.interleaved : attribute.buffer) + descriptor.inputs.bufferOffset).buffer );
		vertexInstance.offset.emplace_back( 0 <= descriptor.inputs.instance.interleaved ? descriptor.inputs.instance.offset : attribute.offset );
	}

	struct {
		VkBuffer buffer = NULL;
		VkDeviceSize offset = 0;
		size_t binding = 0;
		size_t index = 0;
	} index, indirect;

	if ( descriptor.inputs.index.count && !descriptor.inputs.index.attributes.empty() ) {
		auto& attribute = descriptor.inputs.index.attributes.front();
		index.buffer = buffers.at((0 <= descriptor.inputs.index.interleaved ? descriptor.inputs.index.interleaved : attribute.buffer) + descriptor.inputs.bufferOffset).buffer;
		index.offset = 0 <= descriptor.inputs.index.interleaved ? descriptor.inputs.index.offset : attribute.offset;
	}
	if ( descriptor.inputs.indirect.count && !descriptor.inputs.indirect.attributes.empty() ) {
		auto& attribute = descriptor.inputs.indirect.attributes.front();
		indirect.buffer = buffers.at((0 <= descriptor.inputs.indirect.interleaved ? descriptor.inputs.indirect.interleaved : attribute.buffer) + descriptor.inputs.bufferOffset).buffer;
		indirect.offset = 0 <= descriptor.inputs.indirect.interleaved ? descriptor.inputs.indirect.offset : attribute.offset;

	/*
		.indices = indices.size(),
		.instances = 1,
		.indexID = mesh.index.count,
		.vertexID = mesh.vertex.count,

		.instanceID = mesh.instance.count,
		.materialID = p.material,
		.objectID = 0,
		.vertices = vertices.size(),

		if ( attribute.descriptor.pointer ) {
			pod::DrawCommand* drawCommands = (pod::DrawCommand*) attribute.descriptor.pointer;
			for ( auto i = 0; i < descriptor.inputs.indirect.count; ++i ) {
				auto& drawCommand = drawCommands[i];
			//	UF_MSG_DEBUG( "DrawCommand[" << i << "]: " << drawCommand.indices << " " << drawCommand.instances << " " << drawCommand.indexID << " " << drawCommand.vertexID << " " << drawCommand.instanceID << " " << drawCommand.materialID << " " << drawCommand.objectID << " " << drawCommand.vertices );
			}
		}
	*/
	}

	for ( auto& buffer : buffers ) {
		if ( !index.buffer && buffer.usage & uf::renderer::enums::Buffer::INDEX ) index.buffer = buffer.buffer;
		if ( !indirect.buffer && buffer.usage & uf::renderer::enums::Buffer::INDIRECT ) indirect.buffer = buffer.buffer;
	}

	if ( !vertexInstance.buffer.empty() ) {
		vkCmdBindVertexBuffers( commandBuffer, 0, vertexInstance.buffer.size(), vertexInstance.buffer.data(), vertexInstance.offset.data() );
	}

	if ( index.buffer ) {
		VkIndexType indicesType = VK_INDEX_TYPE_UINT32;
		switch ( descriptor.inputs.index.size ) {
			case 1: indicesType = VK_INDEX_TYPE_UINT8_EXT; break;
			case 2: indicesType = VK_INDEX_TYPE_UINT16; break;
			case 4: indicesType = VK_INDEX_TYPE_UINT32; break;
			default:
				UF_EXCEPTION("invalid indices size of " << (int) descriptor.inputs.index.size);
			break;
		}
		vkCmdBindIndexBuffer(commandBuffer, index.buffer, index.offset, indicesType);
	}
	const bool cpuSideIndirection = false;
	if ( index.buffer && indirect.buffer && cpuSideIndirection ) {
		auto& indirectAttribute = descriptor.inputs.indirect.attributes.front();
		const pod::DrawCommand* drawCommands = (const pod::DrawCommand*) indirectAttribute.pointer;
		for ( auto i = 0; i < descriptor.inputs.indirect.count; ++i ) {
			auto& drawCommand = drawCommands[i];
			vkCmdDrawIndexed(commandBuffer,
				drawCommand.indices, // indexCount
				drawCommand.instances, // instanceCount
				drawCommand.indexID, // firstIndex
				drawCommand.vertexID, // vertexOffset
				drawCommand.instanceID // firstInstance
			);
		}
	} else if ( index.buffer && indirect.buffer ) {
		if ( device->enabledFeatures.multiDrawIndirect || descriptor.inputs.indirect.count <= 1 ) {
			vkCmdDrawIndexedIndirect(commandBuffer, indirect.buffer,
				descriptor.inputs.indirect.offset, // offset
				descriptor.inputs.indirect.count, // drawCount
				descriptor.inputs.indirect.size // stride
			);
		} else {
			for ( auto i = 0; i < descriptor.inputs.indirect.count; ++i ) {
				vkCmdDrawIndexedIndirect(commandBuffer, indirect.buffer,
					descriptor.inputs.indirect.offset + i * descriptor.inputs.indirect.size, // offset
					1, // drawCount
					descriptor.inputs.indirect.size // stride
				);
			}
		}
	} else if ( index.buffer && !indirect.buffer ) {
		vkCmdDrawIndexed(commandBuffer,
			descriptor.inputs.index.count, // indexCount
			descriptor.inputs.instance.count, // instanceCount
			descriptor.inputs.index.first, // firstIndex
			descriptor.inputs.vertex.first, // vertexOffset
			descriptor.inputs.instance.first // firstInstance
		);
	} else if ( !vertexInstance.buffer.empty() && indirect.buffer ) {
		vkCmdDrawIndexedIndirect(commandBuffer, indirect.buffer,
			descriptor.inputs.indirect.offset, // offset
			descriptor.inputs.indirect.count, // drawCount
			descriptor.inputs.indirect.size // stride
		);
	} else/* if ( !vertexInstance.buffer.empty() && !indirect.buffer ) */{
		vkCmdDraw(commandBuffer,
			descriptor.inputs.vertex.count, // vertexCount
			descriptor.inputs.instance.count, // instanceCount
			descriptor.inputs.vertex.first, // firstVertex
			descriptor.inputs.instance.first // firstInstance
		);
	}
}
void ext::vulkan::Graphic::destroy() {
	for ( auto& pair : pipelines ) pair.second.destroy();
	pipelines.clear();
	material.destroy();
	ext::vulkan::Buffers::destroy();
	ext::vulkan::states::rebuild = true;
}

#include <uf/utils/string/hash.h>
void ext::vulkan::GraphicDescriptor::parse( ext::json::Value& metadata ) {
	if ( metadata["front face"].is<uf::stl::string>() ) {
		if ( metadata["front face"].as<uf::stl::string>() == "ccw" ) frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		else if ( metadata["front face"].as<uf::stl::string>() == "cw" ) frontFace = VK_FRONT_FACE_CLOCKWISE;
	}
	if ( metadata["cull mode"].is<uf::stl::string>() ) {
		if ( metadata["cull mode"].as<uf::stl::string>() == "back" ) cullMode = VK_CULL_MODE_BACK_BIT;
		else if ( metadata["cull mode"].as<uf::stl::string>() == "front" ) cullMode = VK_CULL_MODE_FRONT_BIT;
		else if ( metadata["cull mode"].as<uf::stl::string>() == "none" ) cullMode = VK_CULL_MODE_NONE;
		else if ( metadata["cull mode"].as<uf::stl::string>() == "both" ) cullMode = VK_CULL_MODE_FRONT_AND_BACK;
	}

	if ( ext::json::isObject(metadata["depth bias"]) ) {
		depth.bias.enable = VK_TRUE;
		depth.bias.constant = metadata["depth bias"]["constant"].as<float>();
		depth.bias.slope = metadata["depth bias"]["slope"].as<float>();
		depth.bias.clamp = metadata["depth bias"]["clamp"].as<float>();
	}
}
ext::vulkan::GraphicDescriptor::hash_t ext::vulkan::GraphicDescriptor::hash() const {
	size_t hash{};

	hash += std::hash<decltype(subpass)>{}(subpass);
	if ( settings::invariant::individualPipelines )
		hash += std::hash<decltype(renderMode)>{}(renderMode);
	
	hash += std::hash<decltype(renderTarget)>{}(renderTarget);
	hash += std::hash<decltype(pipeline)>{}(pipeline);

	for ( auto i = 0; i < inputs.vertex.attributes.size(); ++i ) {
		hash += std::hash<decltype(inputs.vertex.attributes[i].descriptor.format)>{}(inputs.vertex.attributes[i].descriptor.format);
		hash += std::hash<decltype(inputs.vertex.attributes[i].descriptor.offset)>{}(inputs.vertex.attributes[i].descriptor.offset);
	}
	for ( auto i = 0; i < inputs.index.attributes.size(); ++i ) {
		hash += std::hash<decltype(inputs.index.attributes[i].descriptor.format)>{}(inputs.index.attributes[i].descriptor.format);
		hash += std::hash<decltype(inputs.index.attributes[i].descriptor.offset)>{}(inputs.index.attributes[i].descriptor.offset);
	}
	for ( auto i = 0; i < inputs.instance.attributes.size(); ++i ) {
		hash += std::hash<decltype(inputs.instance.attributes[i].descriptor.format)>{}(inputs.instance.attributes[i].descriptor.format);
		hash += std::hash<decltype(inputs.instance.attributes[i].descriptor.offset)>{}(inputs.instance.attributes[i].descriptor.offset);
	}
/*
	for ( auto i = 0; i < inputs.indirect.attributes.size(); ++i ) {
		hash += std::hash<decltype(inputs.indirect.attributes[i].descriptor.format)>{}(inputs.indirect.attributes[i].descriptor.format);
		hash += std::hash<decltype(inputs.indirect.attributes[i].descriptor.offset)>{}(inputs.indirect.attributes[i].descriptor.offset);
	}
*/

	hash += std::hash<decltype(topology)>{}(topology);
	hash += std::hash<decltype(cullMode)>{}(cullMode);
	hash += std::hash<decltype(fill)>{}(fill);
	hash += std::hash<decltype(lineWidth)>{}(lineWidth);
	hash += std::hash<decltype(frontFace)>{}(frontFace);
	hash += std::hash<decltype(depth.test)>{}(depth.test);
	hash += std::hash<decltype(depth.write)>{}(depth.write);
	hash += std::hash<decltype(depth.operation)>{}(depth.operation);
	hash += std::hash<decltype(depth.bias.enable)>{}(depth.bias.enable);
	hash += std::hash<decltype(depth.bias.constant)>{}(depth.bias.constant);
	hash += std::hash<decltype(depth.bias.slope)>{}(depth.bias.slope);
	hash += std::hash<decltype(depth.bias.clamp)>{}(depth.bias.clamp);

	return hash;
}

#endif