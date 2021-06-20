#if UF_USE_VULKAN

#include <uf/ext/vulkan/graphic.h>
#include <uf/ext/vulkan/initializers.h>
#include <uf/ext/vulkan/vulkan.h>
#include <uf/ext/openvr/openvr.h>
#include <uf/utils/camera/camera.h>

#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_glsl.hpp>
#include <fstream>
#include <regex>

#define VK_DEBUG_VALIDATION_MESSAGE(x)\
//	VK_VALIDATION_MESSAGE(x);

namespace {
	uint32_t VERTEX_BUFFER_BIND_ID = 0;
}

void ext::vulkan::Pipeline::initialize( Graphic& graphic ) {
	return this->initialize( graphic, graphic.descriptor );
}
void ext::vulkan::Pipeline::initialize( Graphic& graphic, GraphicDescriptor& descriptor ) {
	this->device = graphic.device;
	this->descriptor = descriptor;
	this->metadata.type = descriptor.pipeline;
	Device& device = *graphic.device;

	// VK_VALIDATION_MESSAGE(&graphic << ": Shaders: " << graphic.material.shaders.size() << " Textures: " << graphic.material.textures.size());
	auto shaders = getShaders( graphic.material.shaders );
	assert( shaders.size() > 0 );

	RenderMode& renderMode = ext::vulkan::getRenderMode( descriptor.renderMode, true);
	auto& renderTarget = renderMode.getRenderTarget(  descriptor.renderTarget );
	{
		std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings;
		std::vector<VkPushConstantRange> pushConstantRanges;
		std::vector<VkDescriptorPoolSize> poolSizes;
		std::unordered_map<VkDescriptorType, uint32_t> descriptorTypes;

	//	for ( auto& shader : graphic.material.shaders ) {
		for ( auto* shaderPointer : shaders ) {
			auto& shader = *shaderPointer;
			descriptorSetLayoutBindings.insert( descriptorSetLayoutBindings.begin(), shader.descriptorSetLayoutBindings.begin(), shader.descriptorSetLayoutBindings.end() );

			std::size_t offset = 0;
			for ( auto& pushConstant : shader.pushConstants ) {
				size_t len = pushConstant.data().len;
				if ( len <= 0 || len > device.properties.limits.maxPushConstantsSize ) {
					VK_DEBUG_VALIDATION_MESSAGE("Invalid push constant length of " << len << " for shader " << shader.filename);
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

		std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentStates;

		VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
		if ( renderMode.getType() != "Swapchain" ) {
			auto& subpass = renderTarget.passes[descriptor.subpass];
			for ( auto& color : subpass.colors ) {
				auto& attachment = renderTarget.attachments[color.attachment];
				blendAttachmentStates.push_back(attachment.blendState);
				samples = std::max(samples, ext::vulkan::sampleCount( attachment.descriptor.samples ));
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
			descriptor.depth.test,
			descriptor.depth.write,
			descriptor.depth.operation //VK_COMPARE_OP_LESS_OR_EQUAL
		);
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
				descriptor.geometry.attributes.vertex.size, 
				VK_VERTEX_INPUT_RATE_VERTEX
			)
		};
		// Attribute descriptions
		// Describes memory layout and shader positions
		std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions = {};
		for ( auto& attribute : descriptor.geometry.attributes.descriptor ) {
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
		for ( auto* shaderPointer : shaders ) {
			auto& shader = *shaderPointer;

			void* s = (void*) shader.specializationConstants;
			size_t len = shader.specializationConstants.data().len;
			bool invalidated = true;
			for ( size_t i = 0; i < len / 4; ++i ) {
				auto& payload = shader.metadata.json["specializationConstants"][i];
				std::string type = payload["type"].as<std::string>();
				if ( type == "int32_t" ) {
					int32_t& v = ((int32_t*) s)[i];
					// failsafe, because for some reason things break
					if ( payload["validate"].as<bool>() && v == 0 ) {
						VK_DEBUG_VALIDATION_MESSAGE("Specialization constant of 0 for `" << payload.dump() << "` for shader `" << shader.filename << "`");
						v = payload["value"].is<int32_t>() ? payload["value"].as<int32_t>() : payload["default"].as<int32_t>();
						invalidated = true;
					}
					payload["value"] = v;
				} else if ( type == "uint32_t" ) {
					uint32_t& v = ((uint32_t*) s)[i];
					// failsafe, because for some reason things break
					if ( payload["validate"].as<bool>() && v == 0 ) {
						VK_DEBUG_VALIDATION_MESSAGE("Specialization constant of 0 for `" << payload.dump() << "` for shader `" << shader.filename << "`");
						v = payload["value"].is<uint32_t>() ? payload["value"].as<uint32_t>() : payload["default"].as<uint32_t>();
						invalidated = true;
					}
					payload["value"] = v;
				} else if ( type == "float" ) {
					float& v = ((float*) s)[i];
					// failsafe, because for some reason things break
					if ( payload["validate"].as<bool>() && v == 0 ) {
						VK_DEBUG_VALIDATION_MESSAGE("Specialization constant of 0 for `" << payload.dump() << "` for shader `" << shader.filename << "`");
						v = payload["value"].is<float>() ? payload["value"].as<float>() : payload["default"].as<float>();
						invalidated = true;
					}
					payload["value"] = v;
				}
			}
			VK_DEBUG_VALIDATION_MESSAGE("Specialization constants for shader `" << shader.filename << "`: " << shader.metadata.json["specializationConstants"].dump(1, '\t'));
			if ( invalidated ) {
			//	shader.specializationInfo = {};
			//	shader.specializationInfo.mapEntryCount = shader.specializationMapEntries.size();
				shader.specializationInfo.pMapEntries = shader.specializationMapEntries.data();
				shader.specializationInfo.pData = (void*) shader.specializationConstants;
			//	shader.specializationInfo.dataSize = shader.specializationConstants.data().len;
				shader.descriptor.pSpecializationInfo = &shader.specializationInfo;
			}
			
			VK_DEBUG_VALIDATION_MESSAGE("Specialization constants for shader `" << shader.filename << "`: " << shader.specializationInfo.dataSize );
			
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
		VK_DEBUG_VALIDATION_MESSAGE("Created graphics pipeline");
	}

	graphic.process = true;
	return;
PIPELINE_INITIALIZATION_INVALID:
	graphic.process = false;
	VK_DEBUG_VALIDATION_MESSAGE("Pipeline initialization invalid, updating next tick...");
	uf::thread::add( uf::thread::get("Main"), [&]() -> int {
		this->initialize( graphic, descriptor );
	return 0;}, true );
	return;
}
void ext::vulkan::Pipeline::record( Graphic& graphic, VkCommandBuffer commandBuffer, size_t pass, size_t draw ) {
	auto bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	auto shaders = getShaders( graphic.material.shaders );
	for ( auto* shaderPointer : shaders ) {
		auto& shader = *shaderPointer;

		if ( shader.descriptor.stage == VK_SHADER_STAGE_COMPUTE_BIT ) {
			bindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
		}
		size_t offset = 0;
		for ( auto& pushConstant : shader.pushConstants ) {
			// 
			if ( ext::json::isObject( shader.metadata.json["definitions"]["pushConstants"]["PushConstant"] ) ) {
				if ( shader.descriptor.stage == VK_SHADER_STAGE_VERTEX_BIT ) {
					struct PushConstant {
						uint32_t pass;
						uint32_t draw;
					} pushConstant = { pass, draw };
					vkCmdPushConstants( commandBuffer, pipelineLayout, shader.descriptor.stage, 0, sizeof(pushConstant), &pushConstant );
				}
			} else {
				size_t len = pushConstant.data().len;
				void* pointer = pushConstant.data().data;
				if ( len > 0 && pointer ) {
					vkCmdPushConstants( commandBuffer, pipelineLayout, shader.descriptor.stage, 0, len, pointer );
				}
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
//	return this->update( graphic, graphic.descriptor );
	return this->update( graphic, descriptor );
}
void ext::vulkan::Pipeline::update( Graphic& graphic, GraphicDescriptor& descriptor ) {
	//
	if ( descriptorSet == VK_NULL_HANDLE ) return;
	this->descriptor = descriptor;

	//descriptor = d;
	RenderMode& renderMode = ext::vulkan::getRenderMode(descriptor.renderMode, true);
	auto& renderTarget = renderMode.getRenderTarget(descriptor.renderTarget );

	std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings;
	auto shaders = getShaders( graphic.material.shaders );
	for ( auto* shaderPointer : shaders ) {
		auto& shader = *shaderPointer;
		descriptorSetLayoutBindings.insert( descriptorSetLayoutBindings.begin(), shader.descriptorSetLayoutBindings.begin(), shader.descriptorSetLayoutBindings.end() );
	}

	struct {
		std::vector<VkDescriptorBufferInfo> uniform;
		std::vector<VkDescriptorBufferInfo> storage;
		
		std::vector<VkDescriptorImageInfo> image;
		std::vector<VkDescriptorImageInfo> image2D;
		std::vector<VkDescriptorImageInfo> imageCube;
		std::vector<VkDescriptorImageInfo> image3D;
		std::vector<VkDescriptorImageInfo> imageUnknown;

		std::vector<VkDescriptorImageInfo> sampler;
		std::vector<VkDescriptorImageInfo> input;
	} infos;

	if ( descriptor.subpass < renderTarget.passes.size() ) {
		auto& subpass = renderTarget.passes[descriptor.subpass];
		for ( auto& input : subpass.inputs ) {
			infos.input.push_back(ext::vulkan::initializers::descriptorImageInfo( 
				renderTarget.attachments[input.attachment].views[subpass.layer],
				input.layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : input.layout
			));
		}
	}

	{
		for ( auto& texture : graphic.material.textures ) {
			infos.image.emplace_back(texture.descriptor);
			switch ( texture.viewType ) {
				case VK_IMAGE_VIEW_TYPE_2D:
					infos.image2D.emplace_back(texture.descriptor);
				break;
				case VK_IMAGE_VIEW_TYPE_CUBE:
					infos.imageCube.emplace_back(texture.descriptor);
				break;
				case VK_IMAGE_VIEW_TYPE_3D:
					infos.image3D.emplace_back(texture.descriptor);
				break;
				default:
					infos.imageUnknown.emplace_back(texture.descriptor);
				break;
			}
		}
		for ( auto& sampler : graphic.material.samplers ) infos.sampler.emplace_back(sampler.descriptor.info);
	}

	size_t consumes = 0;
	std::vector<std::string> types;
	for ( auto* shaderPointer : shaders ) {
		auto& shader = *shaderPointer;

		#define PARSE_BUFFER( buffers ) for ( auto& buffer : buffers ) {\
			if ( buffer.usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT ) {\
				infos.uniform.emplace_back(buffer.descriptor);\
			}\
			if ( buffer.usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT ) {\
				infos.storage.emplace_back(buffer.descriptor);\
			}\
		}

		PARSE_BUFFER(shader.buffers)
		PARSE_BUFFER(graphic.buffers)

		// check if we can even consume that many infos
		for ( auto& layout : shader.descriptorSetLayoutBindings ) {
			switch ( layout.descriptorType ) {
				// consume an texture image info
				case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
				case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
				case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE: {
					consumes += layout.descriptorCount;
					std::string binding = std::to_string(layout.binding);
					std::string imageType = shader.metadata.json["definitions"]["textures"][binding]["type"].as<std::string>();					
					types.reserve(consumes);
					for ( size_t i = 0; i < layout.descriptorCount; ++i ) types.emplace_back(imageType);
				} break;
			}
		}
	}
	
	size_t maxTextures2D = 0;
	size_t maxTextures3D = 0;
	size_t maxTexturesCube = 0;
	size_t maxTexturesUnknown = 0;
	for ( auto& type : types ) {
		if ( type == "3D" ) ++maxTextures3D;
		else if ( type == "Cube" ) ++maxTexturesCube;
		else if ( type == "2D" ) ++maxTextures2D;
		else ++maxTexturesUnknown;
	}

	while ( infos.image2D.size() < maxTextures2D ) infos.image2D.push_back(Texture2D::empty.descriptor);
	while ( infos.imageCube.size() < maxTexturesCube ) infos.imageCube.push_back(TextureCube::empty.descriptor);
	while ( infos.image3D.size() < maxTextures3D ) infos.image3D.push_back(Texture3D::empty.descriptor);
	while ( infos.imageUnknown.size() < maxTexturesUnknown ) infos.imageUnknown.push_back(Texture2D::empty.descriptor);

	for ( size_t i = infos.image.size(); i < consumes; ++i ) {
		std::string type = i < types.size() ? types[i] : "";
		if ( type == "3D" ) infos.image.push_back(Texture3D::empty.descriptor);
		else if ( type == "Cube" ) infos.image.push_back(TextureCube::empty.descriptor);
		else if ( type == "2D" ) infos.image.push_back(Texture2D::empty.descriptor);
		else infos.image.push_back(Texture2D::empty.descriptor);
	}
	
	auto uniformBufferInfo = infos.uniform.begin();
	auto storageBufferInfo = infos.storage.begin();
	
	auto imageInfo = infos.image.begin();
	auto image2DInfo = infos.image2D.begin();
	auto imageCubeInfo = infos.imageCube.begin();
	auto image3DInfo = infos.image3D.begin();
	auto imageUnknownInfo = infos.imageUnknown.begin();

	auto samplerInfo = infos.sampler.begin();
	auto inputInfo = infos.input.begin();


	std::vector<VkWriteDescriptorSet> writeDescriptorSets;
	for ( auto* shaderPointer : shaders ) {
		auto& shader = *shaderPointer;

	//	UF_MSG_DEBUG(shader.filename << ": ");
	//	UF_MSG_DEBUG("\tAVAILABLE UNIFORM BUFFERS: " << infos.uniform.size());
	//	UF_MSG_DEBUG("\tAVAILABLE STORAGE BUFFERS: " << infos.storage.size());
	//	UF_MSG_DEBUG("\tCONSUMING : " << shader.descriptorSetLayoutBindings.size());

		for ( auto& layout : shader.descriptorSetLayoutBindings ) {
	//		VK_VALIDATION_MESSAGE(shader.filename << "\tType: " << layout.descriptorType << "\tConsuming: " << layout.descriptorCount);
			switch ( layout.descriptorType ) {
				// consume an texture image info
				case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
				case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
				case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE: {
				//	UF_MSG_DEBUG("\t["<< layout.binding << "] INSERTING " << layout.descriptorCount << " IMAGE");
				//	if ( layout.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ) UF_MSG_DEBUG("\t\tCOMBINED_IMAGE_SAMPLER");
				//	if ( layout.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE ) UF_MSG_DEBUG("\t\tSAMPLED_IMAGE");
				//	if ( layout.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE ) UF_MSG_DEBUG("\t\tSTORAGE_IMAGE");
				//	if ( layout.descriptorCount == 1 ) UF_MSG_DEBUG(i.imageView << "\t" << (*imageInfo).imageLayout);

				#if 1
					std::string binding = std::to_string(layout.binding);
					std::string imageType = shader.metadata.json["definitions"]["textures"][binding]["type"].as<std::string>();
					if ( imageType == "2D" ) {
						UF_ASSERT_BREAK_MSG( image2DInfo != infos.image2D.end(), "Filename: " << shader.filename << "\tCount: " << layout.descriptorCount )
						writeDescriptorSets.push_back(ext::vulkan::initializers::writeDescriptorSet(
							descriptorSet,
							layout.descriptorType,
							layout.binding,
							&(*image2DInfo),
							layout.descriptorCount
						));
						image2DInfo += layout.descriptorCount;
					} else if ( imageType == "Cube" ) {
						UF_ASSERT_BREAK_MSG( imageCubeInfo != infos.imageCube.end(), "Filename: " << shader.filename << "\tCount: " << layout.descriptorCount )
						writeDescriptorSets.push_back(ext::vulkan::initializers::writeDescriptorSet(
							descriptorSet,
							layout.descriptorType,
							layout.binding,
							&(*imageCubeInfo),
							layout.descriptorCount
						));
						imageCubeInfo += layout.descriptorCount;
					} else if ( imageType == "3D" ) {
						UF_ASSERT_BREAK_MSG( image3DInfo != infos.image3D.end(), "Filename: " << shader.filename << "\tCount: " << layout.descriptorCount )
						writeDescriptorSets.push_back(ext::vulkan::initializers::writeDescriptorSet(
							descriptorSet,
							layout.descriptorType,
							layout.binding,
							&(*image3DInfo),
							layout.descriptorCount
						));
						image3DInfo += layout.descriptorCount;
					} else {
						UF_ASSERT_BREAK_MSG( imageUnknownInfo != infos.imageUnknown.end(), "Filename: " << shader.filename << "\tCount: " << layout.descriptorCount )
						writeDescriptorSets.push_back(ext::vulkan::initializers::writeDescriptorSet(
							descriptorSet,
							layout.descriptorType,
							layout.binding,
							&(*imageUnknownInfo),
							layout.descriptorCount
						));
						imageUnknownInfo += layout.descriptorCount;
					}
				#else
					UF_ASSERT_BREAK_MSG( imageInfo != infos.image.end(), "Filename: " << shader.filename << "\tCount: " << layout.descriptorCount )
					writeDescriptorSets.push_back(ext::vulkan::initializers::writeDescriptorSet(
						descriptorSet,
						layout.descriptorType,
						layout.binding,
						&(*imageInfo),
						layout.descriptorCount
					));
					imageInfo += layout.descriptorCount;
				#endif
				} break;
				case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT: {
				//	UF_MSG_DEBUG("\t["<< layout.binding << "] INSERTING " << layout.descriptorCount << " INPUT_ATTACHMENT");
					UF_ASSERT_BREAK_MSG( inputInfo != infos.input.end(), "Filename: " << shader.filename << "\tCount: " << layout.descriptorCount )
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
				//	UF_MSG_DEBUG("\t["<< layout.binding << "] INSERTING " << layout.descriptorCount << " SAMPLER");
					UF_ASSERT_BREAK_MSG( samplerInfo != infos.sampler.end(), "Filename: " << shader.filename << "\tCount: " << layout.descriptorCount )
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
				//	UF_MSG_DEBUG("\t["<< layout.binding << "] INSERTING " << layout.descriptorCount << " UNIFORM_BUFFER");
					UF_ASSERT_BREAK_MSG( uniformBufferInfo != infos.uniform.end(), "Filename: " << shader.filename << "\tCount: " << layout.descriptorCount )
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
				//	UF_MSG_DEBUG("\t["<< layout.binding << "] INSERTING " << layout.descriptorCount << " STORAGE_BUFFER");
					UF_ASSERT_BREAK_MSG( storageBufferInfo != infos.storage.end(), "Filename: " << shader.filename << "\tCount: " << layout.descriptorCount )
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
				//	if ( descriptor.pImageInfo[i].imageView == VK_NULL_HANDLE ) {
						VK_DEBUG_VALIDATION_MESSAGE("Null image view or layout is undefined, replacing with fallback texture...");
						auto pointer = const_cast<VkDescriptorImageInfo*>(&descriptor.pImageInfo[i]);
						std::string binding = std::to_string(descriptor.dstBinding);
						std::string imageType = "";
						for ( auto* shaderPointer : shaders ) {
							auto& shader = *shaderPointer;

							auto& info = shader.metadata.json["definitions"]["textures"][binding];
							if ( ext::json::isNull(info) ) continue;
							imageType = info["type"].as<std::string>();
							break;
						}
						if ( imageType == "3D" ) {
							*pointer = Texture3D::empty.descriptor;
						} else if ( imageType == "Cube" ) {
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
				if ( 	descriptor.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER ||
						descriptor.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
				) {
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
	VK_DEBUG_VALIDATION_MESSAGE("Pipeline update invalid, updating next tick...");
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
std::vector<ext::vulkan::Shader*> ext::vulkan::Pipeline::getShaders( std::vector<ext::vulkan::Shader>& shaders ) {
	std::unordered_map<std::string, ext::vulkan::Shader*> map;
	std::vector<ext::vulkan::Shader*> res;

/*
	std::string pipelineName = metadata["name"].as<std::string>();
	for ( auto& shader : shaders ) {
		std::string target = shader.metadata.json["pipeline"].as<std::string>();
		std::string type = shader.metadata.json["type"].as<std::string>();
		if ( target != "" && target != pipelineName ) continue;
		map[type] = &shader;
	}
*/
	for ( auto& shader : shaders ) {
		if ( shader.metadata.pipeline != "" && shader.metadata.pipeline != metadata.type ) continue;
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
void ext::vulkan::Material::attachShader( const std::string& filename, VkShaderStageFlagBits stage, const std::string& pipeline ) {
	auto& shader = shaders.emplace_back();
	shader.initialize( *device, filename, stage );

	// check how many samplers requested
	for ( auto& layout : shader.descriptorSetLayoutBindings ) {
		if ( layout.descriptorType != VK_DESCRIPTOR_TYPE_SAMPLER ) continue;
		Sampler& sampler = samplers.emplace_back();
		sampler.initialize( *device );
	}
	
	std::string type = "unknown";
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
	metadata.json["shaders"][pipeline][type]["index"] = shaders.size() - 1;
}
void ext::vulkan::Material::initializeShaders( const std::vector<std::pair<std::string, VkShaderStageFlagBits>>& layout, const std::string& pipeline ) {
	shaders.clear(); shaders.reserve( layout.size() );
	for ( auto& request : layout ) {
		attachShader( request.first, request.second, pipeline );
	}
}
bool ext::vulkan::Material::hasShader( const std::string& type, const std::string& pipeline ) {
	return !ext::json::isNull( metadata.json["shaders"][pipeline][type] );
}
ext::vulkan::Shader& ext::vulkan::Material::getShader( const std::string& type, const std::string& pipeline ) {
	if ( !hasShader(type, pipeline) ) {
		static ext::vulkan::Shader null;
		return null;
	}
	size_t index = metadata.json["shaders"][pipeline][type]["index"].as<size_t>();
	return shaders.at(index);
}
bool ext::vulkan::Material::validate() {
	bool was = true;
	for ( auto& shader : shaders ) {
		if ( !shader.validate() ) was = false;
	}
	return was;
}
ext::vulkan::Graphic::~Graphic() {
	this->destroy();
}
void ext::vulkan::Graphic::initialize( const std::string& renderModeName ) {
	RenderMode& renderMode = ext::vulkan::getRenderMode(renderModeName, true);

//	if ( !uf::Camera::USE_REVERSE_INFINITE_PROJECTION && descriptor.depth.operation == ext::vulkan::enums::Compare::GREATER_OR_EQUAL ) {
//		descriptor.depth.operation = ext::vulkan::enums::Compare::LESS_OR_EQUAL;
//	}

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
	material.validate();

	return pipeline;
}
void ext::vulkan::Graphic::initializeGeometry( const uf::BaseGeometry& mesh ) {
	// already generated, check if we can just update
	if ( descriptor.indices > 0 ) {
		if ( descriptor.geometry.attributes.vertex.size == mesh.attributes.vertex.size && descriptor.geometry.attributes.index.size == mesh.attributes.index.size && descriptor.indices == mesh.attributes.index.length ) {
			// too lazy to check if this equals, only matters in pipeline creation anyways
			descriptor.geometry = mesh;

			int32_t vertexBuffer = -1;
			int32_t indexBuffer = -1;
			for ( size_t i = 0; i < buffers.size(); ++i ) {
				if ( buffers[i].usage & uf::renderer::enums::Buffer::VERTEX ) vertexBuffer = i;
				if ( buffers[i].usage & uf::renderer::enums::Buffer::INDEX ) indexBuffer = i;
			}

			if ( vertexBuffer > 0 && indexBuffer > 0 ) {
				updateBuffer(
					(void*) mesh.attributes.vertex.pointer,
					mesh.attributes.vertex.size * mesh.attributes.vertex.length,
					vertexBuffer
				);
				updateBuffer(
					(void*) mesh.attributes.index.pointer,
					mesh.attributes.index.size * mesh.attributes.index.length,
					indexBuffer
				);
				return;
			}
		}
	}

	descriptor.geometry = mesh;
	descriptor.indices = mesh.attributes.index.length;

	initializeBuffer(
		(void*) mesh.attributes.vertex.pointer,
		mesh.attributes.vertex.size * mesh.attributes.vertex.length,
		uf::renderer::enums::Buffer::VERTEX
	);
	initializeBuffer(
		(void*) mesh.attributes.index.pointer,
		mesh.attributes.index.size * mesh.attributes.index.length,
		uf::renderer::enums::Buffer::INDEX
	);
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
void ext::vulkan::Graphic::record( VkCommandBuffer commandBuffer, size_t pass, size_t draw ) {
	return this->record( commandBuffer, descriptor, pass, draw );
}
void ext::vulkan::Graphic::record( VkCommandBuffer commandBuffer, GraphicDescriptor& descriptor, size_t pass, size_t draw ) {
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

	Buffer* vertexBuffer = NULL;
	Buffer* indexBuffer = NULL;
	for ( auto& buffer : buffers ) {
		if ( buffer.usage & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT ) vertexBuffer = &buffer;
		if ( buffer.usage & VK_BUFFER_USAGE_INDEX_BUFFER_BIT ) indexBuffer = &buffer;
	}
	assert( vertexBuffer && indexBuffer );

	pipeline.record(*this, commandBuffer, pass, draw);
	// Bind triangle vertex buffer (contains position and colors)
	VkDeviceSize offsets[1] = { descriptor.offsets.vertex };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer->buffer, offsets);
	// Bind triangle index buffer
	VkIndexType indicesType = VK_INDEX_TYPE_UINT32;
	switch ( descriptor.geometry.attributes.index.size * 8 ) {
		case  8: indicesType = VK_INDEX_TYPE_UINT8_EXT; break;
		case 16: indicesType = VK_INDEX_TYPE_UINT16; break;
		case 32: indicesType = VK_INDEX_TYPE_UINT32; break;
		default:
			UF_EXCEPTION("invalid indices size of " + std::to_string((int) descriptor.geometry.attributes.index.size));
		break;
	}
	vkCmdBindIndexBuffer(commandBuffer, indexBuffer->buffer, descriptor.offsets.index, indicesType);
	// Draw indexed triangle
	vkCmdDrawIndexed(commandBuffer, descriptor.indices, 1, 0, 0, 1);
	//std::cout << (int) descriptor.indices << "\t" << (int) descriptor.offsets.index << std::endl;
}
void ext::vulkan::Graphic::destroy() {
	for ( auto& pair : pipelines ) pair.second.destroy();
	pipelines.clear();

	material.destroy();

	ext::vulkan::Buffers::destroy();

	ext::vulkan::states::rebuild = true;
}

bool ext::vulkan::Graphic::hasStorage( const std::string& name ) {
	for ( auto& shader : material.shaders ) {
		if ( shader.hasStorage(name) ) return true;
	}
	return false;
}
ext::vulkan::Buffer* ext::vulkan::Graphic::getStorageBuffer( const std::string& name ) {
	size_t storageIndex = -1;
	for ( auto& shader : material.shaders ) {
		if ( !shader.hasStorage(name) ) continue;
		storageIndex = shader.metadata.json["definitions"]["storage"][name]["index"].as<size_t>();
		break;
	}
	for ( size_t bufferIndex = 0, storageCounter = 0; bufferIndex < buffers.size(); ++bufferIndex ) {
		if ( !(buffers[bufferIndex].usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) ) continue;
		if ( storageCounter++ != storageIndex ) continue;
		return &buffers[bufferIndex];
	}
	return NULL;
}
uf::Serializer ext::vulkan::Graphic::getStorageJson( const std::string& name, bool cache ) {
	for ( auto& shader : material.shaders ) {
		if ( !shader.hasStorage(name) ) continue;
		return shader.getStorageJson(name, cache);
	}
	return ext::json::null();
}
ext::vulkan::userdata_t ext::vulkan::Graphic::getStorageUserdata( const std::string& name, const ext::json::Value& payload ) {
	for ( auto& shader : material.shaders ) {
		if ( !shader.hasStorage(name) ) continue;
		return shader.getStorageUserdata(name, payload);
	}
	return ext::vulkan::userdata_t();
}

#include <uf/utils/string/hash.h>
void ext::vulkan::GraphicDescriptor::parse( ext::json::Value& metadata ) {
	if ( metadata["front face"].is<std::string>() ) {
		if ( metadata["front face"].as<std::string>() == "ccw" ) {
			frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		} else if ( metadata["front face"].as<std::string>() == "cw" ) {
			frontFace = VK_FRONT_FACE_CLOCKWISE;
		}
	}
	if ( metadata["cull mode"].is<std::string>() ) {
		if ( metadata["cull mode"].as<std::string>() == "back" ) {
			cullMode = VK_CULL_MODE_BACK_BIT;
		} else if ( metadata["cull mode"].as<std::string>() == "front" ) {
			cullMode = VK_CULL_MODE_FRONT_BIT;
		} else if ( metadata["cull mode"].as<std::string>() == "none" ) {
			cullMode = VK_CULL_MODE_NONE;
		} else if ( metadata["cull mode"].as<std::string>() == "both" ) {
			cullMode = VK_CULL_MODE_FRONT_AND_BACK;
		}
	}
	if ( metadata["indices"].is<size_t>() ) {
		indices = metadata["indices"].as<size_t>();
	}
	if ( ext::json::isObject( metadata["offsets"] ) ) {
		offsets.vertex = metadata["offsets"]["vertex"].as<size_t>();
		offsets.index = metadata["offsets"]["index"].as<size_t>();
	}
	if ( ext::json::isObject(metadata["depth bias"]) ) {
		depth.bias.enable = VK_TRUE;
		depth.bias.constant = metadata["depth bias"]["constant"].as<float>();
		depth.bias.slope = metadata["depth bias"]["slope"].as<float>();
		depth.bias.clamp = metadata["depth bias"]["clamp"].as<float>();
	}
}
ext::vulkan::GraphicDescriptor::hash_t ext::vulkan::GraphicDescriptor::hash() const {
#if UF_GRAPHIC_DESCRIPTOR_USE_STRING
	uf::Serializer serializer;

	serializer["subpass"] = subpass;
	if ( settings::experimental::individualPipelines )
		serializer["renderMode"] = renderMode;

	serializer["renderTarget"] = renderTarget;
	serializer["geometry"]["sizes"]["vertex"] = geometry.attributes.vertex.size;
	serializer["geometry"]["sizes"]["indices"] = geometry.attributes.index.size;
	serializer["indices"] = indices;
	serializer["offsets"]["vertex"] = offsets.vertex;
	serializer["offsets"]["index"] = offsets.index;

	for ( uint8_t i = 0; i < geometry.attributes.descriptor.size(); ++i ) {
		serializer["geometry"]["attributes"][i]["format"] = geometry.attributes.descriptor[i].format;
		serializer["geometry"]["attributes"][i]["offset"] = geometry.attributes.descriptor[i].offset;
	}

	serializer["topology"] = topology;
	serializer["cullMode"] = cullMode;
	serializer["fill"] = fill;
	serializer["lineWidth"] = lineWidth;
	serializer["frontFace"] = frontFace;
	serializer["depth"]["test"] = depth.test;
	serializer["depth"]["write"] = depth.write;
	serializer["depth"]["operation"] = depth.operation;
	serializer["depth"]["bias"]["enable"] = depth.bias.enable;
	serializer["depth"]["bias"]["constant"] = depth.bias.constant;
	serializer["depth"]["bias"]["slope"] = depth.bias.slope;
	serializer["depth"]["bias"]["clamp"] = depth.bias.clamp;

	return uf::string::sha256( serializer.serialize() );
//	return serializer.dump();
#else
	size_t hash{};

	hash += std::hash<decltype(subpass)>{}(subpass);
	if ( settings::experimental::individualPipelines )
		hash += std::hash<decltype(renderMode)>{}(renderMode);
	
	hash += std::hash<decltype(renderTarget)>{}(renderTarget);
	hash += std::hash<decltype(geometry.attributes.vertex.size)>{}(geometry.attributes.vertex.size);
	hash += std::hash<decltype(geometry.attributes.index.size)>{}(geometry.attributes.index.size);
	hash += std::hash<decltype(indices)>{}(indices);
	hash += std::hash<decltype(offsets.vertex)>{}(offsets.vertex);
	hash += std::hash<decltype(offsets.index)>{}(offsets.index);

	for ( uint8_t i = 0; i < geometry.attributes.descriptor.size(); ++i ) {
		hash += std::hash<decltype(geometry.attributes.descriptor[i].format)>{}(geometry.attributes.descriptor[i].format);
		hash += std::hash<decltype(geometry.attributes.descriptor[i].offset)>{}(geometry.attributes.descriptor[i].offset);
	}

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
#endif
}

#endif