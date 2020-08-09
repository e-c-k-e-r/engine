#include <uf/ext/vulkan/initializers.h>
#include <uf/ext/vulkan/graphics/rendertarget.h>
#include <uf/ext/vulkan/vulkan.h>
#include <uf/ext/vulkan/texture.h>

#include <uf/utils/mesh/mesh.h>
#include <uf/ext/openvr/openvr.h>

namespace {
	uint32_t VERTEX_BUFFER_BIND_ID = 0;
}
bool ext::vulkan::RenderTargetGraphic::autoAssignable() const {
	return false;
}
std::string ext::vulkan::RenderTargetGraphic::name() const {
	return "RenderTargetGraphic";
}
void ext::vulkan::RenderTargetGraphic::createCommandBuffer( VkCommandBuffer commandBuffer ) {
	assert( buffers.size() >= 2 );
	Buffer& vertexBuffer = buffers.at(1);
	Buffer& indexBuffer = buffers.at(2);

	pushConstants = { ext::openvr::renderPass };
	vkCmdPushConstants( commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pushConstants), &pushConstants );
	// Bind descriptor sets describing shader binding points
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
	// Bind the rendering pipeline
	// The pipeline (state object) contains all states of the rendering pipeline, binding it will set all the states specified at pipeline creation time
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
	// Bind triangle vertex buffer (contains position and colors)
	VkDeviceSize offsets[1] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer.buffer, offsets);
	// Bind triangle index buffer
	vkCmdBindIndexBuffer(commandBuffer, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
	// Draw indexed triangle
	vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices), 1, 0, 0, 1);
}
void ext::vulkan::RenderTargetGraphic::initialize( const std::string& renderMode ) {
	return initialize(this->device ? *device : ext::vulkan::device, ext::vulkan::getRenderMode(renderMode));
}
void ext::vulkan::RenderTargetGraphic::initialize( Device& device, RenderMode& renderMode ) {
	this->device = &device;

	std::vector<Vertex> vertices = {
	
		{ {-1.0f, 1.0f}, {0.0f, 1.0f}, },
		{ {-1.0f, -1.0f}, {0.0f, 0.0f}, },
		{ {1.0f, -1.0f}, {1.0f, 0.0f}, },
		{ {1.0f, 1.0f}, {1.0f, 1.0f}, }
	
	/*
		{ {-1.0f, 1.0f}, {0.0f, 0.0f}, },
		{ {-1.0f, -1.0f}, {0.0f, 1.0f}, },
		{ {1.0f, -1.0f}, {1.0f, 1.0f}, },

		{ {-1.0f, 1.0f}, {0.0f, 0.0f}, },
		{ {1.0f, -1.0f}, {1.0f, 1.0f}, },
		{ {1.0f, 1.0f}, {1.0f, 0.0f}, }
	*/
	};

	std::vector<uint32_t> indices = {
	//	0, 1, 2, 0, 2, 3
		0, 1, 2, 2, 3, 0
	//	2, 1, 0, 0, 3, 2
	//	0, 3, 2, 2, 1, 0
	//	0, 1, 2, 3, 4, 5
	};

	initializeBuffer(
		(void*) vertices.data(),
		vertices.size() * sizeof(Vertex),
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, //VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		false
	);
	initializeBuffer(
		(void*) indices.data(),
		indices.size() * sizeof(uint32_t),
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, //VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		false
	);
	this->indices = indices.size();
	// asset correct buffer sizes
	assert( buffers.size() >= 2 );
	ext::vulkan::Graphic::initialize( device, renderMode );
	// set descriptor layout
	initializeDescriptorLayout({
		// Vertex shader
		ext::vulkan::initializers::descriptorSetLayoutBinding(
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			VK_SHADER_STAGE_VERTEX_BIT,
			0
		),
		// Fragment shader
		ext::vulkan::initializers::descriptorSetLayoutBinding(
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			VK_SHADER_STAGE_FRAGMENT_BIT,
			1
		)
	}, sizeof(pushConstants));
	// Create sampler
	{
		VkSamplerCreateInfo samplerInfo = {};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_NEAREST;
		samplerInfo.minFilter = VK_FILTER_NEAREST;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeV = samplerInfo.addressModeU;
		samplerInfo.addressModeW = samplerInfo.addressModeU;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.maxAnisotropy = 1.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 1.0f;
		samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		VK_CHECK_RESULT(vkCreateSampler(device, &samplerInfo, nullptr, &sampler));
	}
	// Create uniform buffer
	initializeBuffer(
		(void*) &uniforms,
		sizeof(uniforms),
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		false
	);
	// Swap buffers
	// Move uniform buffer to the front
	{
		for ( auto it = buffers.begin(); it != buffers.end(); ++it ) {
			Buffer& buffer = *it;
			if ( !(buffer.usageFlags & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) ) continue;
			Buffer uniformBuffer = std::move(buffer);
			buffers.erase(it);
			buffers.insert( buffers.begin(), std::move(uniformBuffer) );
			break;
		}
	}
/*
	buffers = {
		std::move(buffers.at(2)),
		std::move(buffers.at(0)),
		std::move(buffers.at(1)),
	};
*/
	// set pipeline
	{
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = ext::vulkan::initializers::pipelineInputAssemblyStateCreateInfo(
			VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			0,
			VK_FALSE
		);
		VkPipelineRasterizationStateCreateInfo rasterizationState = ext::vulkan::initializers::pipelineRasterizationStateCreateInfo(
			VK_POLYGON_MODE_FILL,
		//	VK_CULL_MODE_BACK_BIT,
			VK_CULL_MODE_NONE,
			ext::vulkan::Graphic::DEFAULT_WINDING_ORDER,
			0
		);

		std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentStates;
		for ( auto& attachment : renderMode.renderTarget.attachments ) {
			if ( attachment.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT ) {
				VkPipelineColorBlendAttachmentState blendAttachmentState = ext::vulkan::initializers::pipelineColorBlendAttachmentState(
					0xf,
					VK_FALSE
				);
				blendAttachmentState.blendEnable = VK_TRUE;
				blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
				blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
				blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
				blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
				blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
				blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
				blendAttachmentStates.push_back(blendAttachmentState);
			}
		}
		VkPipelineColorBlendStateCreateInfo colorBlendState = ext::vulkan::initializers::pipelineColorBlendStateCreateInfo(
		//	renderMode.getType() == "Swapchain" ? 1 : blendAttachmentStates.size(),
			1,
			blendAttachmentStates.data()
		);
	/*
		VkPipelineColorBlendAttachmentState blendAttachmentState = ext::vulkan::initializers::pipelineColorBlendAttachmentState(
			0xf,
			VK_FALSE
		);
		VkPipelineColorBlendStateCreateInfo colorBlendState = ext::vulkan::initializers::pipelineColorBlendStateCreateInfo(
			1,
			&blendAttachmentState
		);
	*/
	/*
		VkPipelineColorBlendAttachmentState blendAttachmentState = ext::vulkan::initializers::pipelineColorBlendAttachmentState(
		//	VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
			0xf,
			VK_FALSE
		);
		blendAttachmentState.blendEnable = VK_TRUE;
		blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
		blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
		VkPipelineColorBlendStateCreateInfo colorBlendState = ext::vulkan::initializers::pipelineColorBlendStateCreateInfo(
			1,
			&blendAttachmentState
		);
	*/
	/*
		std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentStates;
		for ( auto& attachment : renderMode.renderTarget.attachments ) {
			if ( attachment.layout == VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT ) {
				VkPipelineColorBlendAttachmentState blendAttachmentState = ext::vulkan::initializers::pipelineColorBlendAttachmentState(
					0xf,
					VK_FALSE
				);
				blendAttachmentStates.push_back(blendAttachmentState);
			}
		}
		VkPipelineColorBlendStateCreateInfo colorBlendState = ext::vulkan::initializers::pipelineColorBlendStateCreateInfo(
			blendAttachmentStates.size(),
			blendAttachmentStates.data()
		);
		colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlendState.logicOpEnable = VK_FALSE;
        colorBlendState.logicOp = VK_LOGIC_OP_COPY;
        colorBlendState.blendConstants[0] = 0.0f;
        colorBlendState.blendConstants[1] = 0.0f;
        colorBlendState.blendConstants[2] = 0.0f;
        colorBlendState.blendConstants[3] = 0.0f;
    */
		VkPipelineDepthStencilStateCreateInfo depthStencilState = ext::vulkan::initializers::pipelineDepthStencilStateCreateInfo(
			VK_FALSE,//VK_TRUE,
			VK_FALSE,//VK_TRUE,
			//VK_COMPARE_OP_LESS_OR_EQUAL
			VK_COMPARE_OP_GREATER_OR_EQUAL
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
		std::vector<VkVertexInputBindingDescription> vertexBindingDescriptions = {
			ext::vulkan::initializers::vertexInputBindingDescription(
				VERTEX_BUFFER_BIND_ID, 
				sizeof(Vertex), 
				VK_VERTEX_INPUT_RATE_VERTEX
			)
		};
		// Attribute descriptions
		// Describes memory layout and shader positions
		std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions = {
			// Location 0 : Position
			ext::vulkan::initializers::vertexInputAttributeDescription(
				VERTEX_BUFFER_BIND_ID,
				0,
				VK_FORMAT_R32G32_SFLOAT,
				offsetof(Vertex, position)
			),			
			// Location 1 : Texture coordinates
			ext::vulkan::initializers::vertexInputAttributeDescription(
				VERTEX_BUFFER_BIND_ID,
				1,
				VK_FORMAT_R32G32_SFLOAT,
				offsetof(Vertex, uv)
			)
		};

		VkPipelineVertexInputStateCreateInfo vertexInputState = ext::vulkan::initializers::pipelineVertexInputStateCreateInfo();
		vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexBindingDescriptions.size());
		vertexInputState.pVertexBindingDescriptions = vertexBindingDescriptions.data();
		vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributeDescriptions.size());
		vertexInputState.pVertexAttributeDescriptions = vertexAttributeDescriptions.data();

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
		pipelineCreateInfo.stageCount = static_cast<uint32_t>(shader.stages.size());
		pipelineCreateInfo.pStages = shader.stages.data();
		pipelineCreateInfo.subpass = this->subpass;

		initializePipeline(pipelineCreateInfo);
	}
	// Set descriptor pool
	initializeDescriptorPool({
		ext::vulkan::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1),
		ext::vulkan::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1),
	}, 2);
	// Set descriptor set
	{
		VkDescriptorImageInfo renderTargetDescriptor = ext::vulkan::initializers::descriptorImageInfo( 
			renderTarget->attachments[0].view,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			sampler
		);

		initializeDescriptorSet({
			// Binding 0 : Projection/View matrix uniform buffer			
			ext::vulkan::initializers::writeDescriptorSet(
				descriptorSet,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				0,
				&(buffers.at(0).descriptor)
			),
			// Binding 1 : Fragment shader texture sampler
			//	Fragment shader: layout (binding = 1) uniform sampler2D samplerColor;
			ext::vulkan::initializers::writeDescriptorSet(
				descriptorSet,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				1,
				&renderTargetDescriptor
			)
		});
	}
}
void ext::vulkan::RenderTargetGraphic::destroy() {
	vkDestroySampler( *device, sampler, nullptr );
	ext::vulkan::Graphic::destroy();
}