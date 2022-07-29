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

#define VK_DEBUG_VALIDATION_MESSAGE(...)\
//	VK_VALIDATION_MESSAGE(x);

namespace {
	uint32_t VERTEX_BUFFER_BIND_ID = 0;
}

PFN_vkGetBufferDeviceAddressKHR ext::vulkan::vkGetBufferDeviceAddressKHR = NULL; // = reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(vkGetDeviceProcAddr(device, "vkGetBufferDeviceAddressKHR"));
PFN_vkCmdBuildAccelerationStructuresKHR ext::vulkan::vkCmdBuildAccelerationStructuresKHR = NULL; // = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(device, "vkCmdBuildAccelerationStructuresKHR"));
PFN_vkBuildAccelerationStructuresKHR ext::vulkan::vkBuildAccelerationStructuresKHR = NULL; // = reinterpret_cast<PFN_vkBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(device, "vkBuildAccelerationStructuresKHR"));
PFN_vkCreateAccelerationStructureKHR ext::vulkan::vkCreateAccelerationStructureKHR = NULL; // = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(device, "vkCreateAccelerationStructureKHR"));
PFN_vkDestroyAccelerationStructureKHR ext::vulkan::vkDestroyAccelerationStructureKHR = NULL; // = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(vkGetDeviceProcAddr(device, "vkDestroyAccelerationStructureKHR"));
PFN_vkGetAccelerationStructureBuildSizesKHR ext::vulkan::vkGetAccelerationStructureBuildSizesKHR = NULL; // = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(vkGetDeviceProcAddr(device, "vkGetAccelerationStructureBuildSizesKHR"));
PFN_vkGetAccelerationStructureDeviceAddressKHR ext::vulkan::vkGetAccelerationStructureDeviceAddressKHR = NULL; // = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(vkGetDeviceProcAddr(device, "vkGetAccelerationStructureDeviceAddressKHR"));
PFN_vkCmdTraceRaysKHR ext::vulkan::vkCmdTraceRaysKHR = NULL; // = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(device, "vkCmdTraceRaysKHR"));
PFN_vkGetRayTracingShaderGroupHandlesKHR ext::vulkan::vkGetRayTracingShaderGroupHandlesKHR = NULL; // = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(vkGetDeviceProcAddr(device, "vkGetRayTracingShaderGroupHandlesKHR"));
PFN_vkCreateRayTracingPipelinesKHR ext::vulkan::vkCreateRayTracingPipelinesKHR = NULL; // = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(vkGetDeviceProcAddr(device, "vkCreateRayTracingPipelinesKHR"));
PFN_vkCmdWriteAccelerationStructuresPropertiesKHR ext::vulkan::vkCmdWriteAccelerationStructuresPropertiesKHR = NULL; // = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(vkGetDeviceProcAddr(device, "vkCreateRayTracingPipelinesKHR"));
PFN_vkCmdCopyAccelerationStructureKHR ext::vulkan::vkCmdCopyAccelerationStructureKHR = NULL; // = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(vkGetDeviceProcAddr(device, "vkCreateRayTracingPipelinesKHR"));

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
					VK_DEBUG_VALIDATION_MESSAGE("Invalid push constant length of {} for shader {}", len, shader.filename);
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
	// raytrace
	{
		uf::stl::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups;
		for ( auto* shader : shaders ) {
			if (!(
				shader->descriptor.stage == VK_SHADER_STAGE_RAYGEN_BIT_KHR ||
				shader->descriptor.stage == VK_SHADER_STAGE_MISS_BIT_KHR ||
				shader->descriptor.stage == VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR ||
				shader->descriptor.stage == VK_SHADER_STAGE_ANY_HIT_BIT_KHR ||
				shader->descriptor.stage == VK_SHADER_STAGE_INTERSECTION_BIT_KHR
			)) continue;
			
			size_t shaderID = static_cast<uint32_t>(shaderDescriptors.size());
			bool isHit = shader->descriptor.stage & (VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR | VK_SHADER_STAGE_INTERSECTION_BIT_KHR);

			auto& shaderGroup = shaderGroups.emplace_back();
			shaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
			shaderGroup.type = !isHit ? VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR : VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
			shaderGroup.generalShader = !isHit ? shaderID : VK_SHADER_UNUSED_KHR;
			
			shaderGroup.closestHitShader = (shader->descriptor.stage & VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR) ? shaderID : VK_SHADER_UNUSED_KHR;
			shaderGroup.anyHitShader = (shader->descriptor.stage & VK_SHADER_STAGE_ANY_HIT_BIT_KHR) ? shaderID : VK_SHADER_UNUSED_KHR;
			shaderGroup.intersectionShader = (shader->descriptor.stage & VK_SHADER_STAGE_INTERSECTION_BIT_KHR) ? shaderID : VK_SHADER_UNUSED_KHR;
			
			shaderDescriptors.emplace_back(shader->descriptor);
		}

		if ( !shaderGroups.empty() ) {
			VkRayTracingPipelineCreateInfoKHR rayTracingPipelineCI{};
			rayTracingPipelineCI.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
			rayTracingPipelineCI.stageCount = static_cast<uint32_t>(shaderDescriptors.size());
			rayTracingPipelineCI.pStages = shaderDescriptors.data();
			rayTracingPipelineCI.groupCount = static_cast<uint32_t>(shaderGroups.size());
			rayTracingPipelineCI.pGroups = shaderGroups.data();
			rayTracingPipelineCI.maxPipelineRayRecursionDepth = 1;
			rayTracingPipelineCI.layout = pipelineLayout;
			VK_CHECK_RESULT(vkCreateRayTracingPipelinesKHR(device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &rayTracingPipelineCI, nullptr, &pipeline));

			VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingPipelineProperties{};
			rayTracingPipelineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;

			VkPhysicalDeviceProperties2 deviceProperties2{};
			deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
			deviceProperties2.pNext = &rayTracingPipelineProperties;

			vkGetPhysicalDeviceProperties2(device.physicalDevice, &deviceProperties2);

			const uint32_t handleSize = rayTracingPipelineProperties.shaderGroupHandleSize;
			const uint32_t handleSizeAligned = ALIGNED_SIZE(rayTracingPipelineProperties.shaderGroupHandleSize, rayTracingPipelineProperties.shaderGroupHandleAlignment);
			const uint32_t groupCount = static_cast<uint32_t>(shaderGroups.size());
			const uint32_t sbtSize = groupCount * handleSizeAligned;
			const VkBufferUsageFlags bufferUsageFlags = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

			std::vector<uint8_t> shaderHandleStorage(sbtSize);
			VK_CHECK_RESULT(vkGetRayTracingShaderGroupHandlesKHR(device, pipeline, 0, groupCount, sbtSize, shaderHandleStorage.data()));

			requestedAlignment = rayTracingPipelineProperties.shaderGroupBaseAlignment;
			size_t raygenBufferIndex = initializeBuffer((const void*) (shaderHandleStorage.data() + handleSizeAligned * 0), handleSize, bufferUsageFlags);
			size_t raymissBufferIndex = initializeBuffer((const void*) (shaderHandleStorage.data() + handleSizeAligned * 1), handleSize, bufferUsageFlags);
			size_t rayhitBufferIndex = initializeBuffer((const void*) (shaderHandleStorage.data() + handleSizeAligned * 2), handleSize, bufferUsageFlags);
			requestedAlignment = 0;

			Buffer raygenBuffer = buffers[raygenBufferIndex].alias();
			Buffer raymissBuffer = buffers[raymissBufferIndex].alias();
			Buffer rayhitBuffer = buffers[rayhitBufferIndex].alias();

			auto& raygenShaderSbtEntry = sbtEntries.emplace_back();
			raygenShaderSbtEntry.deviceAddress = raygenBuffer.getAddress();
			raygenShaderSbtEntry.stride = handleSizeAligned;
			raygenShaderSbtEntry.size = handleSizeAligned;

			auto& raymissShaderSbtEntry = sbtEntries.emplace_back();
			raymissShaderSbtEntry.deviceAddress = raymissBuffer.getAddress();
			raymissShaderSbtEntry.stride = handleSizeAligned;
			raymissShaderSbtEntry.size = handleSizeAligned;

			auto& rayhitShaderSbtEntry = sbtEntries.emplace_back();
			rayhitShaderSbtEntry.deviceAddress = rayhitBuffer.getAddress();
			rayhitShaderSbtEntry.stride = handleSizeAligned;
			rayhitShaderSbtEntry.size = handleSizeAligned;

			auto& raycallShaderSbtEntry = sbtEntries.emplace_back();
			
			return;
		}
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
		RenderMode& renderMode = ext::vulkan::getRenderMode( descriptor.renderMode, true );
		auto& renderTarget = renderMode.getRenderTarget( descriptor.renderTarget );

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
	uf::thread::queue( uf::thread::get(uf::thread::mainThreadName), [&]{
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
		if (
			shader->descriptor.stage == VK_SHADER_STAGE_RAYGEN_BIT_KHR ||
			shader->descriptor.stage == VK_SHADER_STAGE_MISS_BIT_KHR ||
			shader->descriptor.stage == VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR ||
			shader->descriptor.stage == VK_SHADER_STAGE_ANY_HIT_BIT_KHR ||
			shader->descriptor.stage == VK_SHADER_STAGE_INTERSECTION_BIT_KHR
		) bindPoint = VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR;
	#if 1
		if ( shader->metadata.definitions.pushConstants.count("PushConstant") > 0 ) {
			if ( shader->descriptor.stage == VK_SHADER_STAGE_VERTEX_BIT || shader->descriptor.stage == VK_SHADER_STAGE_COMPUTE_BIT || shader->descriptor.stage == VK_SHADER_STAGE_RAYGEN_BIT_KHR ) {
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
	}
	// Bind descriptor sets describing shader binding points
	vkCmdBindDescriptorSets(commandBuffer, bindPoint, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
	// Bind the rendering pipeline
	// The pipeline (state object) contains all states of the rendering pipeline, binding it will set all the states specified at pipeline creation time
	vkCmdBindPipeline(commandBuffer, bindPoint, pipeline);

	if ( bindPoint == VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR ) {
		uf::renderer::vkCmdTraceRaysKHR(
			commandBuffer,
			&sbtEntries[0],
			&sbtEntries[1],
			&sbtEntries[2],
			&sbtEntries[3],
			descriptor.inputs.width ? descriptor.inputs.width : ext::vulkan::settings::width,
			descriptor.inputs.height ? descriptor.inputs.height : ext::vulkan::settings::height,
			1
		);
	} else if ( bindPoint == VK_PIPELINE_BIND_POINT_COMPUTE && descriptor.inputs.dispatch.x != 0 && descriptor.inputs.dispatch.y != 0 && descriptor.inputs.dispatch.z != 0 ) {
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
		uf::stl::vector<VkDescriptorBufferInfo> accelerationStructure;
		uf::stl::vector<VkWriteDescriptorSetAccelerationStructureKHR> accelerationStructureInfos;

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
		//	if ( buffer.usage & uf::renderer::enums::Buffer::ACCELERATION_STRUCTURE ) infos.accelerationStructure.emplace_back(buffer.descriptor);
		}
		// add per-shader buffers
		for ( auto& buffer : shader->buffers ) {
			if ( buffer.usage & uf::renderer::enums::Buffer::UNIFORM ) infos.uniform.emplace_back(buffer.descriptor);
			if ( buffer.usage & uf::renderer::enums::Buffer::STORAGE ) infos.storage.emplace_back(buffer.descriptor);
		//	if ( buffer.usage & uf::renderer::enums::Buffer::ACCELERATION_STRUCTURE ) infos.accelerationStructure.emplace_back(buffer.descriptor);
		}
		// add per-pipeline buffers
		for ( auto& buffer : this->buffers ) {
			if ( buffer.usage & uf::renderer::enums::Buffer::UNIFORM ) infos.uniform.emplace_back(buffer.descriptor);
			if ( buffer.usage & uf::renderer::enums::Buffer::STORAGE ) infos.storage.emplace_back(buffer.descriptor);
		//	if ( buffer.usage & uf::renderer::enums::Buffer::ACCELERATION_STRUCTURE ) infos.accelerationStructure.emplace_back(buffer.descriptor);
		}
		// add per-graphics buffers
		for ( auto& buffer : graphic.buffers ) {
			if ( buffer.usage & uf::renderer::enums::Buffer::UNIFORM ) infos.uniform.emplace_back(buffer.descriptor);
			if ( buffer.usage & uf::renderer::enums::Buffer::STORAGE ) infos.storage.emplace_back(buffer.descriptor);
		//	if ( buffer.usage & uf::renderer::enums::Buffer::ACCELERATION_STRUCTURE ) infos.accelerationStructure.emplace_back(buffer.descriptor);
		}

		if ( descriptor.subpass < renderTarget.passes.size() ) {
			auto& subpass = renderTarget.passes[descriptor.subpass];
			for ( auto& input : subpass.inputs ) {
				infos.input.emplace_back(ext::vulkan::initializers::descriptorImageInfo( 
					renderTarget.attachments[input.attachment].views[subpass.layer],
					ext::vulkan::Texture::remapRenderpassLayout( input.layout )
				));
			}
		}

		#define INSERT_IMAGE(texture) {\
			infos.image.emplace_back(texture.descriptor);\
			switch ( texture.viewType ) {\
				case VK_IMAGE_VIEW_TYPE_2D: infos.image2D.emplace_back(texture.descriptor); break;\
				case VK_IMAGE_VIEW_TYPE_2D_ARRAY: infos.image2DA.emplace_back(texture.descriptor); break;\
				case VK_IMAGE_VIEW_TYPE_CUBE: infos.imageCube.emplace_back(texture.descriptor); break;\
				case VK_IMAGE_VIEW_TYPE_3D: infos.image3D.emplace_back(texture.descriptor); break;\
				default: infos.imageUnknown.emplace_back(texture.descriptor); break;\
			}\
		}

		for ( auto& descriptor : shader->metadata.attachments ) {
			auto texture = Texture2D::empty.alias();
			texture.sampler.descriptor.filter.min = descriptor.filter;
			texture.sampler.descriptor.filter.mag = descriptor.filter;

			auto matches = uf::string::match(descriptor.name, R"(/^(.+?)\[(\d+)\]$/)");
			auto name = matches.size() == 2 ? matches[0] : descriptor.name;
			auto view = matches.size() == 2 ? stoi(matches[1]) : -1;
			const ext::vulkan::RenderTarget::Attachment* attachment = NULL;
			if ( descriptor.renderMode ) {
				if ( descriptor.renderMode->hasAttachment(name) ) 
					attachment = &descriptor.renderMode->getAttachment(name);
			} else if ( renderMode.hasAttachment(name) ) {
				attachment = &renderMode.getAttachment(name);
			}

			if ( attachment ) {
				
				// subscript on name will grab the view # from attachment->views 
				if ( 0 <= view ) {
					texture.aliasAttachment( *attachment, (size_t) view );
				} else {
					texture.aliasAttachment( *attachment );
				}
			}

			if ( descriptor.layout != VK_IMAGE_LAYOUT_UNDEFINED ) {
				texture.imageLayout = descriptor.layout;
				texture.descriptor.imageLayout = descriptor.layout;
			}
			
			INSERT_IMAGE(texture);
		}

	#if 0
		for ( auto& texture : renderMode.textures ) {
			INSERT_IMAGE(texture);
		}
	#endif
		for ( auto& texture : shader->textures ) {
			INSERT_IMAGE(texture);
		}
		for ( auto& texture : graphic.material.textures ) {
			INSERT_IMAGE(texture);
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

		if ( !graphic.accelerationStructures.tops.empty() )  {
			infos.accelerationStructure.emplace_back(graphic.accelerationStructures.tops[0].buffer.descriptor);
		}

		for ( auto& info : infos.accelerationStructure ) {
			auto& descriptorAccelerationStructureInfo = infos.accelerationStructureInfos.emplace_back();
			descriptorAccelerationStructureInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
			descriptorAccelerationStructureInfo.accelerationStructureCount = 1;
			descriptorAccelerationStructureInfo.pAccelerationStructures = &graphic.accelerationStructures.tops[infos.accelerationStructureInfos.size()-1].handle;
		}
		
		auto uniformBufferInfo = infos.uniform.begin();
		auto storageBufferInfo = infos.storage.begin();
		auto accelerationStructureInfo = infos.accelerationStructure.begin();
		auto accelerationStructureInfos = infos.accelerationStructureInfos.begin();
		
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
						UF_ASSERT_BREAK_MSG( image2DInfo != infos.image2D.end(), "Filename: {}\tCount: {}", shader->filename, layout.descriptorCount )
						writeDescriptorSets.emplace_back(ext::vulkan::initializers::writeDescriptorSet(
							descriptorSet,
							layout.descriptorType,
							layout.binding,
							&(*image2DInfo),
							layout.descriptorCount
						));
						image2DInfo += layout.descriptorCount;
					} else if ( imageType == ext::vulkan::enums::Image::VIEW_TYPE_2D_ARRAY ) {
						UF_ASSERT_BREAK_MSG( image2DAInfo != infos.image2DA.end(), "Filename: {}\tCount: {}", shader->filename, layout.descriptorCount )
						writeDescriptorSets.emplace_back(ext::vulkan::initializers::writeDescriptorSet(
							descriptorSet,
							layout.descriptorType,
							layout.binding,
							&(*image2DAInfo),
							layout.descriptorCount
						));
						image2DAInfo += layout.descriptorCount;
					} else if ( imageType == ext::vulkan::enums::Image::VIEW_TYPE_CUBE ) {
						UF_ASSERT_BREAK_MSG( imageCubeInfo != infos.imageCube.end(), "Filename: {}\tCount: {}", shader->filename, layout.descriptorCount )
						writeDescriptorSets.emplace_back(ext::vulkan::initializers::writeDescriptorSet(
							descriptorSet,
							layout.descriptorType,
							layout.binding,
							&(*imageCubeInfo),
							layout.descriptorCount
						));
						imageCubeInfo += layout.descriptorCount;
					} else if ( imageType == ext::vulkan::enums::Image::VIEW_TYPE_3D ) {
						UF_ASSERT_BREAK_MSG( image3DInfo != infos.image3D.end(), "Filename: {}\tCount: {}", shader->filename, layout.descriptorCount )
						writeDescriptorSets.emplace_back(ext::vulkan::initializers::writeDescriptorSet(
							descriptorSet,
							layout.descriptorType,
							layout.binding,
							&(*image3DInfo),
							layout.descriptorCount
						));
						image3DInfo += layout.descriptorCount;
					} else {
						UF_ASSERT_BREAK_MSG( imageUnknownInfo != infos.imageUnknown.end(), "Filename: {}\tCount: {}", shader->filename, layout.descriptorCount )
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
					UF_ASSERT_BREAK_MSG( inputInfo != infos.input.end(), "Filename: {}\tCount: {}", shader->filename, layout.descriptorCount )
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
					UF_ASSERT_BREAK_MSG( samplerInfo != infos.sampler.end(), "Filename: {}\tCount: {}", shader->filename, layout.descriptorCount )
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
					UF_ASSERT_BREAK_MSG( uniformBufferInfo != infos.uniform.end(), "Filename: {}\tCount: {}", shader->filename, layout.descriptorCount )
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
					UF_ASSERT_BREAK_MSG( storageBufferInfo != infos.storage.end(), "Filename: {}\tCount: {}", shader->filename, layout.descriptorCount )
					writeDescriptorSets.emplace_back(ext::vulkan::initializers::writeDescriptorSet(
						descriptorSet,
						layout.descriptorType,
						layout.binding,
						&(*storageBufferInfo),
						layout.descriptorCount
					));
					storageBufferInfo += layout.descriptorCount;
				} break;
				case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR: {
					UF_ASSERT_BREAK_MSG( accelerationStructureInfo != infos.accelerationStructure.end(), "Filename: {}\tCount: {}", shader->filename, layout.descriptorCount )
					auto& writeDescriptorSet = writeDescriptorSets.emplace_back(ext::vulkan::initializers::writeDescriptorSet(
						descriptorSet,
						layout.descriptorType,
						layout.binding,
						&(*accelerationStructureInfo),
						layout.descriptorCount
					));
					writeDescriptorSet.pNext = &(*accelerationStructureInfos);

					accelerationStructureInfo += layout.descriptorCount;
					accelerationStructureInfos += layout.descriptorCount;
				} break;
			}
		}
	}

	// validate writeDescriptorSets
	for ( auto& descriptor : writeDescriptorSets ) {
		for ( size_t i = 0; i < descriptor.descriptorCount; ++i ) {
			if ( descriptor.pBufferInfo ) {
				if ( descriptor.pBufferInfo[i].offset % device->properties.limits.minUniformBufferOffsetAlignment != 0 ) {
				//	VK_DEBUG_VALIDATION_MESSAGE("Invalid descriptor for buffer: " << descriptor.pBufferInfo[i].buffer << " (Offset: " << descriptor.pBufferInfo[i].offset << ", Range: " << descriptor.pBufferInfo[i].range << "), invalidating...");
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
				#if 0
					if ( descriptor.pImageInfo[i].imageLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ) {
						VK_DEBUG_VALIDATION_MESSAGE("Image layout is VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, fixing to VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL");
						auto pointer = const_cast<VkDescriptorImageInfo*>(&descriptor.pImageInfo[i]);
						pointer->imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
					}
				#endif
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

	{
		bool locked = renderMode.tryMutex();
		renderMode.rebuild = true;
		vkUpdateDescriptorSets( *device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, NULL );
		if ( locked ) renderMode.unlockMutex();
	}
	return;

PIPELINE_UPDATE_INVALID:
//	graphic.process = false;
	VK_DEBUG_VALIDATION_MESSAGE("Pipeline update invalid, updating next tick...");
	uf::thread::queue( uf::thread::get(uf::thread::mainThreadName), [&]{
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

//	if ( settings::experimental::dedicatedThread ) ext::vulkan::states::rebuild = true;
/*
	if ( ext::vulkan::hasRenderMode(descriptor.renderMode, true) ) {
		RenderMode& renderMode = ext::vulkan::getRenderMode(descriptor.renderMode, true);
		renderMode.rebuild = true;
	}
*/
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
	shader.metadata.autoInitializeUniformBuffers = metadata.autoInitializeUniformBuffers;
	shader.metadata.autoInitializeUniformUserdatas = metadata.autoInitializeUniformUserdatas;
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
		case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT: type = "tessellation:control"; break;
		case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT: type = "tessellation:evaluation"; break;
		case VK_SHADER_STAGE_GEOMETRY_BIT: type = "geometry"; break;
		case VK_SHADER_STAGE_FRAGMENT_BIT: type = "fragment"; break;
		case VK_SHADER_STAGE_COMPUTE_BIT: type = "compute"; break;
		case VK_SHADER_STAGE_ALL_GRAPHICS: type = "all:graphics"; break;
		case VK_SHADER_STAGE_ALL: type = "all"; break;
		case VK_SHADER_STAGE_RAYGEN_BIT_KHR: type = "ray:gen"; break;
		case VK_SHADER_STAGE_ANY_HIT_BIT_KHR: type = "ray:hit:any"; break;
		case VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR: type = "ray:hit:closest"; break;
		case VK_SHADER_STAGE_MISS_BIT_KHR: type = "ray:miss"; break;
		case VK_SHADER_STAGE_INTERSECTION_BIT_KHR: type = "ray:intersection"; break;
		case VK_SHADER_STAGE_CALLABLE_BIT_KHR: type = "ray:callable"; break;
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

	if ( this->accelerationStructures.tops.empty() ) this->accelerationStructures.tops.resize(2);
}
void ext::vulkan::Graphic::initializePipeline() {
	initializePipeline( this->descriptor, false );
}
ext::vulkan::Pipeline& ext::vulkan::Graphic::initializePipeline( const GraphicDescriptor& descriptor, bool update ) {
	auto& pipeline = pipelines[descriptor];

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

#if 0
	// it makes my life 10000x easier if we interleave a mesh while also requesting RT pipelines
	if ( !mesh.isInterleaved() && uf::renderer::settings::pipelines::rt ) {
		auto interleaved = mesh.interleave();
		return initializeMesh(interleaved);
	}
#endif

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
		descriptor.inputs.bufferOffset = buffers.size(); // buffers.empty() ? 0 : buffers.size() - 1;
		VkBufferUsageFlags baseUsage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;

	/*
		#define PARSE_ATTRIBUTE(i, usage) {\
			auto& buffer = mesh.buffers[i];\
			if ( !buffer.empty() ) queue.emplace_back(Queue{ (void*) buffer.data(), buffer.size(), usage | baseUsage });\
		}
		#define PARSE_INPUT(name, usage){\
			if ( mesh.isInterleaved( mesh.name.interleaved ) ) PARSE_ATTRIBUTE(descriptor.inputs.name.interleaved, usage | baseUsage)\
			else for ( auto& attribute : descriptor.inputs.name.attributes ) PARSE_ATTRIBUTE(attribute.buffer, usage | baseUsage)\
		}
	*/
		#define PARSE_INPUT(name, usage){\
			if ( mesh.isInterleaved( mesh.name.interleaved ) ) {\
				auto& buffer = mesh.buffers[mesh.name.interleaved];\
				if ( !buffer.empty() ) {\
					descriptor.inputs.name.interleaved = initializeBuffer( (const void*) buffer.data(), buffer.size(), usage | baseUsage );\
					this->metadata.buffers[#name"[0]"] = descriptor.inputs.name.interleaved;\
				} else mesh.name.interleaved = -1;\
			} else for ( size_t i = 0; i < descriptor.inputs.name.attributes.size(); ++i ) {\
				auto& attribute = descriptor.inputs.name.attributes[i];\
				auto& buffer = mesh.buffers[attribute.buffer];\
				if ( !buffer.empty() ) {\
					attribute.buffer = initializeBuffer( (const void*) buffer.data(), buffer.size(), usage | baseUsage );\
					this->metadata.buffers[#name"["+std::to_string(i)+"]"] = attribute.buffer;\
				} else attribute.buffer = -1;\
			}\
		}
		// allocate buffers
		auto previousRequestedAlignment = this->requestedAlignment;
		this->requestedAlignment = 16;
		PARSE_INPUT(vertex, uf::renderer::enums::Buffer::VERTEX)
		PARSE_INPUT(index, uf::renderer::enums::Buffer::INDEX)
		PARSE_INPUT(instance, uf::renderer::enums::Buffer::VERTEX)
		PARSE_INPUT(indirect, uf::renderer::enums::Buffer::INDIRECT | uf::renderer::enums::Buffer::STORAGE)
	// 	for ( auto& q : queue ) initializeBuffer( q.data, q.size, q.usage );
		this->requestedAlignment = previousRequestedAlignment;
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

/*
	// copy descriptors
	#define UPDATE_DESCRIPTOR(N) {\
		for ( size_t i = 0; i < mesh.N.attributes.size(); ++i ) {\
			descriptor.inputs.N.attributes[i].descriptor = mesh.N.attributes[i].descriptor;\
			descriptor.inputs.N.attributes[i].offset = mesh.N.attributes[i].offset;\
			descriptor.inputs.N.attributes[i].stride = mesh.N.attributes[i].stride;\
			descriptor.inputs.N.attributes[i].length = mesh.N.attributes[i].length;\
			descriptor.inputs.N.attributes[i].pointer = mesh.N.attributes[i].pointer;\
		}\
		descriptor.inputs.N.count = mesh.N.count;\
		descriptor.inputs.N.first = mesh.N.first;\
		descriptor.inputs.N.size = mesh.N.size;\
		descriptor.inputs.N.offset = mesh.N.offset;\
	}

	UPDATE_DESCRIPTOR(vertex);
	UPDATE_DESCRIPTOR(index);
	UPDATE_DESCRIPTOR(instance);
	UPDATE_DESCRIPTOR(indirect);
*/
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

/*
	#define PARSE_ATTRIBUTE(i, usage) {\
		auto& buffer = mesh.buffers[i];\
		if ( !buffer.empty() ) queue.emplace_back(Queue{ (void*) buffer.data(), buffer.size(), usage });\
	}
	#define PARSE_INPUT(name, usage){\
		if ( mesh.isInterleaved( mesh.name.interleaved ) ) PARSE_ATTRIBUTE(descriptor.inputs.name.interleaved, usage)\
		else for ( auto& attribute : descriptor.inputs.name.attributes ) PARSE_ATTRIBUTE(attribute.buffer, usage)\
	}
*/
/*
	#define PARSE_INPUT(name, usage){\
		if ( mesh.isInterleaved( mesh.name.interleaved ) ) {\
			auto& buffer = mesh.buffers[mesh.name.interleaved];\
			if ( !buffer.empty() ) rebuild = rebuild || updateBuffer( (const void*) buffer.data(), buffer.size(), descriptor.inputs.name.interleaved );\
		} else for ( size_t i = 0; i < descriptor.inputs.name.attributes.size(); ++i ) {\
			auto& buffer = mesh.buffers[mesh.name.attributes[i].buffer];\
			if ( !buffer.empty() ) rebuild = rebuild || updateBuffer( (const void*) buffer.data(), buffer.size(), descriptor.inputs.name.attributes[i].buffer );\
		}\
	}
*/
	#define PARSE_INPUT(name, usage){\
		if ( mesh.isInterleaved( mesh.name.interleaved ) ) {\
			auto& buffer = mesh.buffers[mesh.name.interleaved];\
			if ( !buffer.empty() ) {\
				const uf::stl::string key = #name"[0]";\
				if ( this->metadata.buffers.count(key) > 0 ) {\
					descriptor.inputs.name.interleaved = this->metadata.buffers[key];\
					rebuild = rebuild || updateBuffer( (const void*) buffer.data(), buffer.size(), descriptor.inputs.name.interleaved );\
				}\
			} else mesh.name.interleaved = -1;\
		} else for ( size_t i = 0; i < descriptor.inputs.name.attributes.size(); ++i ) {\
			auto& attribute = descriptor.inputs.name.attributes[i];\
			auto& buffer = mesh.buffers[attribute.buffer];\
			if ( !buffer.empty() ) {\
				const uf::stl::string key = #name"["+std::to_string(i)+"]";\
				if ( this->metadata.buffers.count(key) == 0 ) continue;\
				attribute.buffer = this->metadata.buffers[key];\
				rebuild = rebuild || updateBuffer( (const void*) buffer.data(), buffer.size(), attribute.buffer );\
			} else attribute.buffer = -1;\
		}\
	}

	bool rebuild = false;
	auto previousRequestedAlignment = this->requestedAlignment;
	this->requestedAlignment = 16;
	PARSE_INPUT(vertex, uf::renderer::enums::Buffer::VERTEX)
	PARSE_INPUT(index, uf::renderer::enums::Buffer::INDEX)
	PARSE_INPUT(instance, uf::renderer::enums::Buffer::VERTEX)
	PARSE_INPUT(indirect, uf::renderer::enums::Buffer::INDIRECT)
	this->requestedAlignment = previousRequestedAlignment;
/*
	// allocate buffers
	bool rebuild = false;
	for ( auto i = 0; i < queue.size(); ++i ) {
		auto& q = queue[i];
		rebuild = rebuild || updateBuffer( q.data, q.size, descriptor.inputs.bufferOffset + i );
	}
*/
	if ( mesh.instance.count == 0 && mesh.instance.attributes.empty() ) {
		descriptor.inputs.instance.count = 1;
	}
	return rebuild;
}
void ext::vulkan::Graphic::generateBottomAccelerationStructures() {
	auto& device = *this->device; // ext::vulkan::device;

	VkPhysicalDeviceAccelerationStructurePropertiesKHR acclerationStructureProperties{};
	VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingPipelineProperties{};
	VkPhysicalDeviceProperties2 deviceProperties2{}; {
		acclerationStructureProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR;

		rayTracingPipelineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
		rayTracingPipelineProperties.pNext = &acclerationStructureProperties;

		deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		deviceProperties2.pNext = &rayTracingPipelineProperties;

		vkGetPhysicalDeviceProperties2(device.physicalDevice, &deviceProperties2);
	}

	struct BlasData {
		uf::stl::vector<VkAccelerationStructureGeometryKHR> asGeom;
		uf::stl::vector<VkAccelerationStructureBuildRangeInfoKHR> asBuildOffsetInfo;
		VkBuildAccelerationStructureFlagsKHR flags{};

    	VkAccelerationStructureBuildGeometryInfoKHR buildInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    	VkAccelerationStructureBuildSizesInfoKHR sizeInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};

    	const VkAccelerationStructureBuildRangeInfoKHR* rangeInfo;
    	uf::renderer::AccelerationStructure as;
    	uf::renderer::AccelerationStructure cleanup;
  	};
  	uf::stl::vector<BlasData> blasDatas;

	// setup BLAS geometry
	{
		auto& mesh = this->descriptor.inputs;

		uf::Mesh::Attribute vertexAttribute;
		size_t vertexBufferAddress{};

		uf::Mesh::Attribute indexAttribute;
		size_t indexBufferAddress{};

		/*if ( mesh.vertex.count )*/{
			for ( auto& attribute : mesh.vertex.attributes ) if ( attribute.descriptor.name == "position" ) vertexAttribute = attribute;
			UF_ASSERT( vertexAttribute.descriptor.name == "position" );

			size_t vertexBufferIndex = (0 <= mesh.vertex.interleaved ? mesh.vertex.interleaved : vertexAttribute.buffer) + mesh.bufferOffset;
			vertexBufferAddress = this->buffers[vertexBufferIndex].getAddress();
		}

		if ( mesh.index.count ) {
			indexAttribute = mesh.index.attributes.front();

			size_t indexBufferIndex = (0 <= mesh.index.interleaved ? mesh.index.interleaved : indexAttribute.buffer) + mesh.bufferOffset;
			indexBufferAddress = this->buffers[indexBufferIndex].getAddress();
		}

		if ( mesh.indirect.count ) {
			uf::Mesh::Attribute indirectAttribute = mesh.indirect.attributes.front();
			const pod::DrawCommand* drawCommands = (const pod::DrawCommand*) indirectAttribute.pointer;
			for ( size_t i = 0; i < mesh.indirect.count; ++i ) {
				const auto& drawCommand = drawCommands[i];
				
				uf::Mesh::Input vertexInput = mesh.vertex;
				uf::Mesh::Input indexInput = mesh.index;

				vertexInput.first = drawCommand.vertexID;
				vertexInput.count = drawCommand.vertices;

				indexInput.first = drawCommand.indexID;
				indexInput.count = drawCommand.indices;

				auto& blasData = blasDatas.emplace_back();
			//	blasData.as.drawID 					= i;
				blasData.as.instanceID 				= drawCommand.instanceID;

				VkAccelerationStructureGeometryKHR& asGeom = blasData.asGeom.emplace_back();
				asGeom.sType 						= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
				asGeom.geometryType					= VK_GEOMETRY_TYPE_TRIANGLES_KHR;
				asGeom.flags						= VK_GEOMETRY_OPAQUE_BIT_KHR | VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR;

				VkAccelerationStructureGeometryTrianglesDataKHR& triangles = asGeom.geometry.triangles;
				triangles.sType 					= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
				triangles.vertexFormat				= vertexAttribute.descriptor.format;
				triangles.vertexData.deviceAddress	= vertexBufferAddress;
				triangles.vertexStride				= vertexAttribute.stride;
				triangles.maxVertex 				= vertexInput.count;

				triangles.indexType					= VK_INDEX_TYPE_NONE_KHR;
				triangles.indexData.deviceAddress	= indexBufferAddress;

				VkAccelerationStructureBuildRangeInfoKHR& offset = blasData.asBuildOffsetInfo.emplace_back();
				offset.firstVertex     				= vertexInput.first;
				offset.primitiveCount  				= vertexInput.count / 3;
				offset.primitiveOffset 				= indexInput.first * indexInput.size;
				offset.transformOffset 				= 0;

				if ( indexInput.count ) {
					offset.primitiveCount 			= indexInput.count / 3;
					switch ( indexInput.size ) {
						case sizeof( uint8_t): triangles.indexType = VK_INDEX_TYPE_UINT8_EXT; break;
						case sizeof(uint16_t): triangles.indexType = VK_INDEX_TYPE_UINT16; break;
						case sizeof(uint32_t): triangles.indexType = VK_INDEX_TYPE_UINT32; break;
					}
				}
			}
		} else {
			uf::Mesh::Input vertexInput = mesh.vertex;
			uf::Mesh::Input indexInput = mesh.index;

			auto& blasData = blasDatas.emplace_back();
			blasData.as.instanceID 				= 0;

			VkAccelerationStructureGeometryKHR& asGeom = blasData.asGeom.emplace_back();
			asGeom.sType 						= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
			asGeom.geometryType					= VK_GEOMETRY_TYPE_TRIANGLES_KHR;
			asGeom.flags						= VK_GEOMETRY_OPAQUE_BIT_KHR;

			VkAccelerationStructureGeometryTrianglesDataKHR& triangles = asGeom.geometry.triangles;
			triangles.sType 					= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
			triangles.vertexFormat				= vertexAttribute.descriptor.format;
			triangles.vertexData.deviceAddress	= vertexBufferAddress;
			triangles.vertexStride				= vertexAttribute.stride;
			triangles.maxVertex 				= vertexInput.count;

			triangles.indexType					= VK_INDEX_TYPE_NONE_KHR;
			triangles.indexData.deviceAddress	= indexBufferAddress;

			VkAccelerationStructureBuildRangeInfoKHR& offset = blasData.asBuildOffsetInfo.emplace_back();
			offset.firstVertex     				= vertexInput.first;
			offset.primitiveCount  				= vertexInput.count / 3;
			offset.primitiveOffset 				= indexInput.first * indexInput.size;
			offset.transformOffset 				= 0;

			if ( indexInput.count ) {
				offset.primitiveCount 			= indexInput.count / 3;
				switch ( indexInput.size ) {
					case sizeof( uint8_t): triangles.indexType = VK_INDEX_TYPE_UINT8_EXT; break;
					case sizeof(uint16_t): triangles.indexType = VK_INDEX_TYPE_UINT16; break;
					case sizeof(uint32_t): triangles.indexType = VK_INDEX_TYPE_UINT32; break;
				}
			}
		}
	}

	bool shouldCompact = true;

	// determine BLAS size and its scratch buffer
	VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	if ( shouldCompact) flags |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR;
	size_t totalBlasBufferSize{};
	size_t maxScratchBufferSize{};
	for ( auto& blasData : blasDatas ) {
		blasData.buildInfo.type				= VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		blasData.buildInfo.mode				= VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
		blasData.buildInfo.flags			= blasData.flags | flags;
		blasData.buildInfo.geometryCount	= static_cast<uint32_t>(blasData.asGeom.size());
		blasData.buildInfo.pGeometries		= blasData.asGeom.data();

		blasData.rangeInfo 					= blasData.asBuildOffsetInfo.data();

		uf::stl::vector<uint32_t> maxPrimCount(blasData.asBuildOffsetInfo.size());
		for(auto _ = 0; _ < blasData.asBuildOffsetInfo.size(); _++) maxPrimCount[_] = blasData.asBuildOffsetInfo[_].primitiveCount;
		
		uf::renderer::vkGetAccelerationStructureBuildSizesKHR(
			device,
			VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
			&blasData.buildInfo,
			maxPrimCount.data(),
			&blasData.sizeInfo
		);

		blasData.sizeInfo.accelerationStructureSize = ALIGNED_SIZE(blasData.sizeInfo.accelerationStructureSize, 256);
		maxScratchBufferSize = std::max( maxScratchBufferSize, blasData.sizeInfo.buildScratchSize );
		totalBlasBufferSize += blasData.sizeInfo.accelerationStructureSize;
	}

	// create BLAS buffer and handle
	size_t blasBufferIndex = this->initializeBuffer( NULL, totalBlasBufferSize, uf::renderer::enums::Buffer::ACCELERATION_STRUCTURE | uf::renderer::enums::Buffer::ADDRESS );
	this->metadata.buffers["blasBuffer"] = blasBufferIndex;
	
	VkQueryPool queryPool{VK_NULL_HANDLE};
	if ( shouldCompact ) {
		VkQueryPoolCreateInfo queryPoolCreateInfo{VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
		queryPoolCreateInfo.queryType  = VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR;
		queryPoolCreateInfo.queryCount = blasDatas.size();

		VK_CHECK_RESULT(vkCreateQueryPool(device, &queryPoolCreateInfo, nullptr, &queryPool));
		vkResetQueryPool(device, queryPool, 0, blasDatas.size());
	}

	uint32_t queryCount{};
	{
		size_t blasBufferOffset = 0;
		scratchBuffer.alignment = acclerationStructureProperties.minAccelerationStructureScratchOffsetAlignment;
		scratchBuffer.update( NULL, maxScratchBufferSize );
		
		auto commandBuffer = device.fetchCommandBuffer(uf::renderer::QueueEnum::COMPUTE);
		for ( auto& blasData : blasDatas ) {
			blasData.as.buffer = this->buffers[blasBufferIndex].alias();
			blasData.as.buffer.descriptor.offset = blasBufferOffset;
			blasBufferOffset += blasData.sizeInfo.accelerationStructureSize;

			VkAccelerationStructureCreateInfoKHR createInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
			createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
			createInfo.size = blasData.sizeInfo.accelerationStructureSize;
			createInfo.buffer = blasData.as.buffer.buffer;
			createInfo.offset = blasData.as.buffer.descriptor.offset;

			VK_CHECK_RESULT(uf::renderer::vkCreateAccelerationStructureKHR(device, &createInfo, nullptr, &blasData.as.handle));

			VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
			accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
			accelerationDeviceAddressInfo.accelerationStructure = blasData.as.handle;
			blasData.as.deviceAddress = uf::renderer::vkGetAccelerationStructureDeviceAddressKHR(device, &accelerationDeviceAddressInfo);

			// write to BLAS
			blasData.buildInfo.dstAccelerationStructure  = blasData.as.handle;
			blasData.buildInfo.scratchData.deviceAddress = scratchBuffer.getAddress();

			uf::renderer::vkCmdBuildAccelerationStructuresKHR(
				commandBuffer,
				1,
				&blasData.buildInfo,
				&blasData.rangeInfo
			);

			// pipeline barrier here for scratch buffer
			VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
			barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
			barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
			vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1, &barrier, 0, nullptr, 0, nullptr);

			if ( queryPool ) uf::renderer::vkCmdWriteAccelerationStructuresPropertiesKHR(
				commandBuffer,
				1,
				&blasData.buildInfo.dstAccelerationStructure,
				VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR,
				queryPool,
				queryCount++
			);
		}
		device.flushCommandBuffer(commandBuffer);
	}
	// compact
	if ( queryPool ) {
		uf::stl::vector<VkDeviceSize> compactSizes(blasDatas.size());
		vkGetQueryPoolResults(device, queryPool, 0, (uint32_t) compactSizes.size(), compactSizes.size() * sizeof(VkDeviceSize), compactSizes.data(), sizeof(VkDeviceSize), VK_QUERY_RESULT_WAIT_BIT);

		size_t queryIndex{};
		size_t totalBlasBufferSize{};
		for ( auto& blasData : blasDatas ) {
			size_t compactedSize = compactSizes[queryIndex++];
			size_t oldSize = blasData.sizeInfo.accelerationStructureSize;

			blasData.cleanup = blasData.as;
			blasData.sizeInfo.accelerationStructureSize = ALIGNED_SIZE(compactedSize, 256);
			totalBlasBufferSize += blasData.sizeInfo.accelerationStructureSize;
		//	UF_MSG_DEBUG("Reduced size to {}% ({} -> {} = {})", (oldSize - compactedSize) * 100.0f / oldSize, oldSize, compactedSize, oldSize - compactedSize);
		}

		ext::vulkan::Buffer oldBuffer;
		oldBuffer.initialize( NULL, totalBlasBufferSize, uf::renderer::enums::Buffer::ACCELERATION_STRUCTURE | uf::renderer::enums::Buffer::ADDRESS );
		this->buffers[blasBufferIndex].swap(oldBuffer);
		
		size_t blasBufferOffset{};
		auto commandBuffer = device.fetchCommandBuffer(uf::renderer::QueueEnum::COMPUTE);
		for ( auto& blasData : blasDatas ) {
			blasData.as.buffer = this->buffers[blasBufferIndex].alias();
			blasData.as.buffer.descriptor.offset = blasBufferOffset;
			blasBufferOffset += blasData.sizeInfo.accelerationStructureSize;

			VkAccelerationStructureCreateInfoKHR createInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
			createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
			createInfo.size = blasData.sizeInfo.accelerationStructureSize;
			createInfo.buffer = blasData.as.buffer.buffer;
			createInfo.offset = blasData.as.buffer.descriptor.offset;

			VK_CHECK_RESULT(uf::renderer::vkCreateAccelerationStructureKHR(device, &createInfo, nullptr, &blasData.as.handle));

			VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
			accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
			accelerationDeviceAddressInfo.accelerationStructure = blasData.as.handle;
			blasData.as.deviceAddress = uf::renderer::vkGetAccelerationStructureDeviceAddressKHR(device, &accelerationDeviceAddressInfo);

			VkCopyAccelerationStructureInfoKHR copyInfo{VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_INFO_KHR};
			copyInfo.src  = blasData.cleanup.handle;
			copyInfo.dst  = blasData.as.handle;
			copyInfo.mode = VK_COPY_ACCELERATION_STRUCTURE_MODE_COMPACT_KHR;
			
			uf::renderer::vkCmdCopyAccelerationStructureKHR(commandBuffer, &copyInfo);
		}
		device.flushCommandBuffer(commandBuffer);

		oldBuffer.destroy();
	}

	for ( auto& blasData : blasDatas ) {
		this->accelerationStructures.bottoms.emplace_back(blasData.as);

		if ( blasData.cleanup.handle ) uf::renderer::vkDestroyAccelerationStructureKHR(device, blasData.cleanup.handle, nullptr);
	}

	if ( queryPool ) vkDestroyQueryPool(device, queryPool, nullptr);
}
void ext::vulkan::Graphic::generateTopAccelerationStructure( const uf::stl::vector<uf::renderer::Graphic*>& graphics, const uf::stl::vector<pod::Instance>& instances ) {
	auto& device = *this->device;

	bool rebuild = false;
	bool update = this->accelerationStructures.tops[0].handle != VK_NULL_HANDLE;
	bool shouldCompact = false;

	VkPhysicalDeviceAccelerationStructurePropertiesKHR acclerationStructureProperties{};
	VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingPipelineProperties{};
	VkPhysicalDeviceProperties2 deviceProperties2{}; {
		acclerationStructureProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR;

		rayTracingPipelineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
		rayTracingPipelineProperties.pNext = &acclerationStructureProperties;

		deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		deviceProperties2.pNext = &rayTracingPipelineProperties;

		vkGetPhysicalDeviceProperties2(device.physicalDevice, &deviceProperties2);
	}

	VkQueryPool queryPool{VK_NULL_HANDLE};
	if ( shouldCompact ) {
		VkQueryPoolCreateInfo queryPoolCreateInfo{VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
		queryPoolCreateInfo.queryType  = VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR;
		queryPoolCreateInfo.queryCount = 1;

		VK_CHECK_RESULT(vkCreateQueryPool(device, &queryPoolCreateInfo, nullptr, &queryPool));
		vkResetQueryPool(device, queryPool, 0, 1);
	}


	uf::stl::vector<VkAccelerationStructureInstanceKHR> instancesVK; instancesVK.reserve( instances.size() );
	for ( auto& graphic : graphics ) {
		for ( auto& blas : graphic->accelerationStructures.bottoms ) {
			size_t instanceID = blas.instanceID;
			auto instanceKeyName = std::to_string(instanceID);

			auto mat = instances[instanceID].model;
			mat = uf::matrix::transpose(mat);

			auto& instanceVK = instancesVK.emplace_back();
			memcpy(&instanceVK.transform, &mat, sizeof(instanceVK.transform));
			instanceVK.instanceCustomIndex 						= blas.instanceID;
			instanceVK.accelerationStructureReference			= blas.deviceAddress;
			instanceVK.flags 									= 0; // VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
			instanceVK.mask 									= 0xFF;
			instanceVK.instanceShaderBindingTableRecordOffset	= 0;
		}
	}

	size_t instanceIndex{};
	size_t tlasBufferIndex{};
	size_t tlasBackBufferIndex{};

	if ( !update ) {
		// do not stage, because apparently vkQueueWaitIdle doesn't actually wait for the transfer to complete
		instanceIndex = this->initializeBuffer(
			(const void*) instancesVK.data(), instancesVK.size() * sizeof(VkAccelerationStructureInstanceKHR),
			uf::renderer::enums::Buffer::ADDRESS | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR, false
		);
		this->metadata.buffers["tlasInstance"] = instanceIndex;
	} else {
		if ( this->metadata.buffers.count("tlasInstance") > 0 ) {
			instanceIndex = this->metadata.buffers["tlasInstance"];
		} else UF_EXCEPTION("Buffers not found: {}", "tlasInstance");
		rebuild = rebuild || this->updateBuffer( (const void*) instancesVK.data(), instancesVK.size() * sizeof(VkAccelerationStructureInstanceKHR), instanceIndex, false );
	}
	size_t instanceBufferAddress = this->buffers[instanceIndex].getAddress();

	// have a front-and-back TLAS (buffer)
	// provides zero benefit so far
#define TLAS_FRONT_AND_BACK 0

#if TLAS_FRONT_AND_BACK
	rebuild = true;
	auto& tlasBack 	= this->accelerationStructures.tops[0];
	auto& tlas 		= this->accelerationStructures.tops[1];
#else
	auto& tlas 		= this->accelerationStructures.tops[0];
#endif

	{
		VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
		if ( shouldCompact ) flags |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR;
		uint32_t countInstance = instancesVK.size();

		VkAccelerationStructureGeometryInstancesDataKHR instancesVk{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR};
  		instancesVk.data.deviceAddress 			= instanceBufferAddress;

  		VkAccelerationStructureGeometryKHR tlasGeometry{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
		tlasGeometry.geometryType				= VK_GEOMETRY_TYPE_INSTANCES_KHR;
		tlasGeometry.geometry.instances			= instancesVk;

		VkAccelerationStructureBuildGeometryInfoKHR buildInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
		buildInfo.flags							= flags;
		buildInfo.geometryCount					= 1;
		buildInfo.pGeometries					= &tlasGeometry;
		buildInfo.mode							= update ? VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR : VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
		buildInfo.type							= VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		buildInfo.srcAccelerationStructure		= VK_NULL_HANDLE;

		VkAccelerationStructureBuildSizesInfoKHR sizeInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
		uf::renderer::vkGetAccelerationStructureBuildSizesKHR(
			device,
			VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
			&buildInfo,
			&countInstance,
			&sizeInfo
		);

		// create BLAS buffer and handle
		auto tlasBufferUsageFlags = uf::renderer::enums::Buffer::ACCELERATION_STRUCTURE | uf::renderer::enums::Buffer::ADDRESS;
		if ( !update ) {
			const size_t EXTRANEOUS_SIZE = 1024 * 1024; // oversize, to avoid having to constantly resize
			size_t bufferSize = MAX( sizeInfo.accelerationStructureSize, EXTRANEOUS_SIZE );

			tlasBufferIndex = this->initializeBuffer( NULL, bufferSize, tlasBufferUsageFlags);
			this->metadata.buffers["tlasBuffer"] = tlasBufferIndex;
		#if TLAS_FRONT_AND_BACK
			tlasBackBufferIndex = this->initializeBuffer( NULL, bufferSize, tlasBufferUsageFlags);
			this->metadata.buffers["tlasBackBuffer"] = tlasBackBufferIndex;
		#endif
		} else {
			if ( this->metadata.buffers.count("tlasBuffer") > 0 ) {
				tlasBufferIndex = this->metadata.buffers["tlasBuffer"];
			} else UF_EXCEPTION("Buffers not found: {}", "tlasBuffer");
		#if TLAS_FRONT_AND_BACK
			if ( this->metadata.buffers.count("tlasBackBuffer") > 0 ) {
				tlasBackBufferIndex = this->metadata.buffers["tlasBackBuffer"];
			} else UF_EXCEPTION("Buffers not found: {}", "tlasBackBuffer");
		#endif
			rebuild = rebuild || this->updateBuffer( NULL, sizeInfo.accelerationStructureSize, tlasBufferIndex );
		}
		
		if ( !update ) {
			tlas.buffer = this->buffers[tlasBufferIndex].alias();
		#if TLAS_FRONT_AND_BACK
			tlasBack.buffer = this->buffers[tlasBackBufferIndex].alias();
		#endif

			VkAccelerationStructureCreateInfoKHR createInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
			createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
			createInfo.size = sizeInfo.accelerationStructureSize;
			createInfo.buffer = tlas.buffer.buffer;

			VK_CHECK_RESULT(uf::renderer::vkCreateAccelerationStructureKHR(device, &createInfo, nullptr, &tlas.handle));
			
		#if TLAS_FRONT_AND_BACK
			// create back TLAS
			createInfo.buffer = tlasBack.buffer.buffer;			
			VK_CHECK_RESULT(uf::renderer::vkCreateAccelerationStructureKHR(device, &createInfo, nullptr, &tlasBack.handle));
		#endif
		}

		{
			VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
			accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
			accelerationDeviceAddressInfo.accelerationStructure = tlas.handle;
			tlas.deviceAddress = uf::renderer::vkGetAccelerationStructureDeviceAddressKHR(device, &accelerationDeviceAddressInfo);
			
		#if TLAS_FRONT_AND_BACK
			// create back TLAS
			accelerationDeviceAddressInfo.accelerationStructure = tlasBack.handle;
			tlasBack.deviceAddress = uf::renderer::vkGetAccelerationStructureDeviceAddressKHR(device, &accelerationDeviceAddressInfo);
		#endif
		}

		// write to TLAS
		scratchBuffer.alignment = acclerationStructureProperties.minAccelerationStructureScratchOffsetAlignment;
		scratchBuffer.update( NULL, sizeInfo.buildScratchSize );

		buildInfo.srcAccelerationStructure  = update ? tlas.handle : VK_NULL_HANDLE;
		buildInfo.dstAccelerationStructure  = tlas.handle;
		buildInfo.scratchData.deviceAddress = scratchBuffer.getAddress();

		VkAccelerationStructureBuildRangeInfoKHR        buildOffsetInfo{countInstance, 0, 0, 0};
		const VkAccelerationStructureBuildRangeInfoKHR* rangeInfo = &buildOffsetInfo;

		auto commandBuffer = device.fetchCommandBuffer(uf::renderer::QueueEnum::COMPUTE);
		uf::renderer::vkCmdBuildAccelerationStructuresKHR(
			commandBuffer,
			1,
			&buildInfo,
			&rangeInfo
		);

		if ( queryPool ) uf::renderer::vkCmdWriteAccelerationStructuresPropertiesKHR(
			commandBuffer,
			1,
			&buildInfo.dstAccelerationStructure,
			VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR,
			queryPool,
			0
		);

		device.flushCommandBuffer(commandBuffer);
	}

#if TLAS_FRONT_AND_BACK
	if ( !update ) {
		VkCopyAccelerationStructureInfoKHR copyInfo{VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_INFO_KHR};
		copyInfo.src  = tlas.handle;
		copyInfo.dst  = tlasBack.handle;
		copyInfo.mode = VK_COPY_ACCELERATION_STRUCTURE_MODE_CLONE_KHR;
		
		auto commandBuffer = device.fetchCommandBuffer(uf::renderer::QueueEnum::COMPUTE);
		uf::renderer::vkCmdCopyAccelerationStructureKHR(commandBuffer, &copyInfo);
		device.flushCommandBuffer(commandBuffer);
	}
#endif

	// compact
	if ( queryPool ) {
		VkDeviceSize compactSize{};
		vkGetQueryPoolResults(device, queryPool, 0, 1, sizeof(VkDeviceSize), &compactSize, sizeof(VkDeviceSize), VK_QUERY_RESULT_WAIT_BIT);

		auto cleanup = tlas;
		size_t tlasBufferSize = ALIGNED_SIZE(compactSize, 256);
	//	UF_MSG_DEBUG("Reduced size to {}% ({} -> {} = {})", (float) (oldSize - compactedSize) / (float) (oldSize), oldSize, compactedSize, oldSize - compactedSize);

		ext::vulkan::Buffer oldBuffer;
		oldBuffer.initialize( NULL, tlasBufferSize, uf::renderer::enums::Buffer::ACCELERATION_STRUCTURE | uf::renderer::enums::Buffer::ADDRESS );
		this->buffers[tlasBufferIndex].swap(oldBuffer);
		
		tlas.buffer = this->buffers[tlasBufferIndex].alias();
		tlas.buffer.descriptor.offset = 0;

		VkAccelerationStructureCreateInfoKHR createInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
		createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		createInfo.size = tlasBufferSize;
		createInfo.buffer = tlas.buffer.buffer;
		createInfo.offset = tlas.buffer.descriptor.offset;

		VK_CHECK_RESULT(uf::renderer::vkCreateAccelerationStructureKHR(device, &createInfo, nullptr, &tlas.handle));

		VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
		accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
		accelerationDeviceAddressInfo.accelerationStructure = tlas.handle;
		tlas.deviceAddress = uf::renderer::vkGetAccelerationStructureDeviceAddressKHR(device, &accelerationDeviceAddressInfo);

		VkCopyAccelerationStructureInfoKHR copyInfo{VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_INFO_KHR};
		copyInfo.src  = cleanup.handle;
		copyInfo.dst  = tlas.handle;
		copyInfo.mode = VK_COPY_ACCELERATION_STRUCTURE_MODE_COMPACT_KHR;
		
		auto commandBuffer = device.fetchCommandBuffer(uf::renderer::QueueEnum::COMPUTE);
		uf::renderer::vkCmdCopyAccelerationStructureKHR(commandBuffer, &copyInfo);
		device.flushCommandBuffer(commandBuffer);

		uf::renderer::vkDestroyAccelerationStructureKHR(device, cleanup.handle, nullptr);
		oldBuffer.destroy();
	}

	// swap from back to front
#if TLAS_FRONT_AND_BACK
	std::swap( tlas, tlasBack );
//	UF_MSG_DEBUG("{} {}", tlas.deviceAddress, tlasBack.deviceAddress);
#endif

	if ( queryPool ) vkDestroyQueryPool(device, queryPool, nullptr);

	if ( rebuild ) {
		auto& renderMode = ext::vulkan::getRenderMode( descriptor.renderMode, true );
		renderMode.rebuild = true;
	//	uf::renderer::states::rebuild = true;
	}


	// build SBT
	if ( !this->material.hasShader("ray:gen", uf::renderer::settings::pipelines::names::rt) ) {
		{
			uf::stl::string rayGenShaderFilename = uf::io::root+"/shaders/raytrace/shader.ray-gen.spv";
			uf::stl::string rayMissShaderFilename = uf::io::root+"/shaders/raytrace/shader.ray-miss.spv";
			uf::stl::string rayHitClosestShaderFilename = uf::io::root+"/shaders/raytrace/shader.ray-hit-closest.spv";
			uf::stl::string rayHitAnyShaderFilename = uf::io::root+"/shaders/raytrace/shader.ray-hit-any.spv";

			this->material.attachShader(rayGenShaderFilename, VK_SHADER_STAGE_RAYGEN_BIT_KHR, uf::renderer::settings::pipelines::names::rt);
			this->material.attachShader(rayMissShaderFilename, VK_SHADER_STAGE_MISS_BIT_KHR, uf::renderer::settings::pipelines::names::rt);
			this->material.attachShader(rayHitClosestShaderFilename, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, uf::renderer::settings::pipelines::names::rt);
		//	this->material.attachShader(rayHitAnyShaderFilename, VK_SHADER_STAGE_ANY_HIT_BIT_KHR, uf::renderer::settings::pipelines::names::rt);
		}
		{
			auto& shader = this->material.getShader("ray:gen", uf::renderer::settings::pipelines::names::rt);
			shader.buffers.emplace_back( this->buffers[tlasBufferIndex].alias() );
		}
	}			
}
bool ext::vulkan::Graphic::hasPipeline( const GraphicDescriptor& descriptor ) const {
	return pipelines.count( descriptor ) > 0;
}
ext::vulkan::Pipeline& ext::vulkan::Graphic::getPipeline() {
	return getPipeline( descriptor );
}
const ext::vulkan::Pipeline& ext::vulkan::Graphic::getPipeline() const {
	return getPipeline( descriptor );
}
ext::vulkan::Pipeline& ext::vulkan::Graphic::getPipeline( const GraphicDescriptor& descriptor ) {
	if ( !hasPipeline(descriptor) ) return initializePipeline( descriptor );
	return pipelines[descriptor];
}
const ext::vulkan::Pipeline& ext::vulkan::Graphic::getPipeline( const GraphicDescriptor& descriptor ) const {
	if ( !hasPipeline(descriptor) ) UF_EXCEPTION("does not have pipeline");
	return pipelines.at(descriptor);
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
		VK_DEBUG_VALIDATION_MESSAGE(this << ": has no valid pipeline ({} {})", descriptor.renderMode, descriptor.renderTarget);
		return;
	}

	auto& pipeline = this->getPipeline( descriptor );
	if ( pipeline.descriptorSet == VK_NULL_HANDLE ) {
		VK_DEBUG_VALIDATION_MESSAGE(this << ": has no valid pipeline descriptor set ({} {})", descriptor.renderMode, descriptor.renderTarget);
		return;
	}
	if ( !pipeline.metadata.process ) return;
	pipeline.record(*this, descriptor, commandBuffer, pass, draw);

	auto shaders = pipeline.getShaders( material.shaders );
	for ( auto* shader : shaders ) {
		if ( shader->descriptor.stage == VK_SHADER_STAGE_COMPUTE_BIT ) return;
		if (
			shader->descriptor.stage == VK_SHADER_STAGE_RAYGEN_BIT_KHR ||
			shader->descriptor.stage == VK_SHADER_STAGE_MISS_BIT_KHR ||
			shader->descriptor.stage == VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR ||
			shader->descriptor.stage == VK_SHADER_STAGE_ANY_HIT_BIT_KHR ||
			shader->descriptor.stage == VK_SHADER_STAGE_INTERSECTION_BIT_KHR
		) return;
	}

	struct {
		uf::stl::vector<VkBuffer> buffer;
		uf::stl::vector<VkDeviceSize> offset;
	} vertexInstance;

	if ( 0 <= descriptor.inputs.vertex.interleaved && !descriptor.inputs.vertex.attributes.empty() ) {
		vertexInstance.buffer.emplace_back( buffers.at(descriptor.inputs.vertex.interleaved).buffer );
		vertexInstance.offset.emplace_back( descriptor.inputs.vertex.offset );
	} else {
		for ( auto& attribute : descriptor.inputs.vertex.attributes ) {
			vertexInstance.buffer.emplace_back( attribute.buffer < 0 ? VK_NULL_HANDLE : buffers.at(attribute.buffer).buffer );
			vertexInstance.offset.emplace_back( attribute.offset );
		}
	}

	if ( 0 <= descriptor.inputs.instance.interleaved && !descriptor.inputs.instance.attributes.empty() ) {
		vertexInstance.buffer.emplace_back( buffers.at(descriptor.inputs.instance.interleaved).buffer );
		vertexInstance.offset.emplace_back( descriptor.inputs.instance.offset );
	} else {
		for ( auto& attribute : descriptor.inputs.instance.attributes ) {
			vertexInstance.buffer.emplace_back( attribute.buffer < 0 ? VK_NULL_HANDLE : buffers.at(attribute.buffer).buffer );
			vertexInstance.offset.emplace_back( attribute.offset );
		}
	}

	struct {
		VkBuffer buffer = NULL;
		VkDeviceSize offset = 0;
		size_t binding = 0;
		size_t index = 0;
	} index, indirect;

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
				UF_EXCEPTION("invalid indices size of {}", (int) descriptor.inputs.index.size);
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
	if ( device ) {
		for ( auto& as : accelerationStructures.bottoms ) {
			uf::renderer::vkDestroyAccelerationStructureKHR(*device, as.handle, nullptr);
		}
		accelerationStructures.bottoms.clear();
		
		for ( auto& as : accelerationStructures.tops ) {
			uf::renderer::vkDestroyAccelerationStructureKHR(*device, as.handle, nullptr);
		}
		accelerationStructures.tops.clear();
	}
	for ( auto& pair : pipelines ) pair.second.destroy();
	pipelines.clear();
	material.destroy();
	ext::vulkan::Buffers::destroy();
//	ext::vulkan::states::rebuild = true;
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
	size_t seed{};
#if 0
	for ( auto i = 0; i < inputs.vertex.attributes.size(); ++i ) {
		uf::hash( inputs.vertex.attributes[i].descriptor.format );
		uf::hash( inputs.vertex.attributes[i].descriptor.offset );
	}
	for ( auto i = 0; i < inputs.index.attributes.size(); ++i ) {
		uf::hash( inputs.index.attributes[i].descriptor.format );
		uf::hash( inputs.index.attributes[i].descriptor.offset );
	}
	for ( auto i = 0; i < inputs.instance.attributes.size(); ++i ) {
		uf::hash( inputs.instance.attributes[i].descriptor.format );
		uf::hash( inputs.instance.attributes[i].descriptor.offset );
	}
	for ( auto i = 0; i < inputs.indirect.attributes.size(); ++i ) {
		uf::hash( inputs.indirect.attributes[i].descriptor.format );
		uf::hash( inputs.indirect.attributes[i].descriptor.offset );
	}
#endif
	uf::hash( seed, subpass, renderMode, renderTarget, pipeline, topology, cullMode, fill, lineWidth, frontFace, depth.test, depth.write, depth.operation, depth.bias.enable, depth.bias.constant, depth.bias.slope, depth.bias.clamp );
	return seed;
#if 0

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
#endif
}

#endif