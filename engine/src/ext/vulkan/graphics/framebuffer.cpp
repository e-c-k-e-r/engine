#include <uf/ext/vulkan/initializers.h>
#include <uf/ext/vulkan/graphics/framebuffer.h>
#include <uf/ext/vulkan/vulkan.h>
#include <uf/ext/vulkan/texture.h>

#include <uf/utils/mesh/mesh.h>

namespace {
	uint32_t VERTEX_BUFFER_BIND_ID = 0;
}
bool ext::vulkan::FramebufferGraphic::autoAssignable() const {
	return false;
}
std::string ext::vulkan::FramebufferGraphic::name() const {
	return "FramebufferGraphic";
}
void ext::vulkan::FramebufferGraphic::createCommandBuffer( VkCommandBuffer commandBuffer ) {
	assert( buffers.size() >= 2 );
	Buffer& vertexBuffer = buffers.at(1);
	Buffer& indexBuffer = buffers.at(2);
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
void ext::vulkan::FramebufferGraphic::initialize( const std::string& renderMode ) {
	return initialize(this->device ? *device : ext::vulkan::device, ext::vulkan::getRenderMode(renderMode));
}
void ext::vulkan::FramebufferGraphic::initialize( Device& device, RenderMode& renderMode ) {
	this->device = &device;

	std::vector<Vertex> vertices = {
	
		{ {-1.0f, 1.0f}, {0.0f, 0.0f}, },
		{ {-1.0f, -1.0f}, {0.0f, 1.0f}, },
		{ {1.0f, -1.0f}, {1.0f, 1.0f}, },
		{ {1.0f, 1.0f}, {1.0f, 0.0f}, }
	
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
		0, 1, 2, 0, 2, 3
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
			VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
			VK_SHADER_STAGE_FRAGMENT_BIT,
			1
		),
	/*
		ext::vulkan::initializers::descriptorSetLayoutBinding(
			VK_DESCRIPTOR_TYPE_SAMPLER,
			VK_SHADER_STAGE_FRAGMENT_BIT,
			2
		),
	*/
	});
	// Create sampler
/*
	{
		VkSamplerCreateInfo samplerInfo = {};
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
*/
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
		std::move(buffers.at(2)),
		std::move(buffers.at(0)),
		std::move(buffers.at(1)),
	};
	// set pipeline
	{
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
			renderMode.getRenderPass(),
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
		pipelineCreateInfo.subpass = 1;

		initializePipeline(pipelineCreateInfo);
	}
	// Set descriptor pool
	initializeDescriptorPool({
		ext::vulkan::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1),
		ext::vulkan::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1),
	}, 1);
	// Set descriptor set
	{
		VkDescriptorImageInfo textDescriptorAlbedo = ext::vulkan::initializers::descriptorImageInfo( 
			framebuffer->attachments[0].view,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		);

		initializeDescriptorSet({
			// Binding 0 : Projection/View matrix uniform buffer			
			ext::vulkan::initializers::writeDescriptorSet(
				descriptorSet,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				0,
				&(buffers.at(0).descriptor)
			),
			// Binding 1 : Albedo input attachment
			ext::vulkan::initializers::writeDescriptorSet(
				descriptorSet,
				VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
				1,
				&textDescriptorAlbedo
			),
		/*
			// Binding 1 : Fragment shader texture
			//	Fragment shader: layout (binding = 1) uniform texture2D tex;
			ext::vulkan::initializers::writeDescriptorSet(
				descriptorSet,
				VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
				1,
				&imageInfo
			),
			// Binding 2 : Fragment shader texture sampler
			//	Fragment shader: layout (binding = 1) uniform sampler samp;
			ext::vulkan::initializers::writeDescriptorSet(
				descriptorSet,
				VK_DESCRIPTOR_TYPE_SAMPLER,
				2,
				&samplerInfo
			)
		*/
		});
	}
/*
	for (int32_t i = 0; i < swapchain.drawCommandBuffers.size(); ++i) {
		VkImageMemoryBarrier imageMemoryBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		imageMemoryBarrier.srcAccessMask = 0;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		imageMemoryBarrier.image = swapchain.buffers[i].image;
		imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
		imageMemoryBarrier.subresourceRange.levelCount = 1;
		imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
		imageMemoryBarrier.subresourceRange.layerCount = 1;
		imageMemoryBarrier.srcQueueFamilyIndex = ext::vulkan::device.queueFamilyIndices.graphics;
		imageMemoryBarrier.dstQueueFamilyIndex = ext::vulkan::device.queueFamilyIndices.graphics;
		vkCmdPipelineBarrier( swapchain.drawCommandBuffers[i], VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, NULL, 0, NULL, 1, &imageMemoryBarrier );
	}
*/
}
void ext::vulkan::FramebufferGraphic::destroy() {
	ext::vulkan::Graphic::destroy();
	// vkDestroySampler( *device, sampler, nullptr );
}