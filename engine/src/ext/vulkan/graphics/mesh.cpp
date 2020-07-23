#include <uf/ext/vulkan/initializers.h>
#include <uf/ext/vulkan/graphics/mesh.h>
#include <uf/ext/vulkan/vulkan.h>

namespace {
	uint32_t VERTEX_BUFFER_BIND_ID = 0;
}
bool ext::vulkan::MeshGraphic::autoAssignable() const {
	return false;
}
std::string ext::vulkan::MeshGraphic::name() const {
	return "MeshGraphic";
}
void ext::vulkan::MeshGraphic::createCommandBuffer( VkCommandBuffer commandBuffer ) {
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
void ext::vulkan::MeshGraphic::initialize( const std::string& renderMode ) {
	return initialize(this->device ? *device : ext::vulkan::device, ext::vulkan::getRenderMode(renderMode));
}
void ext::vulkan::MeshGraphic::initialize( Device& device, RenderMode& renderMode ) {
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
	});
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

	// check
/*
	{
		size_t total = 0;
		{
			VK_CHECK_RESULT(buffers.at(2).map());
			std::cout << "INDICES\n"; for ( size_t i = 0; i < indices; ++i ) std::cout << ((uint32_t*) buffers.at(2).mapped)[i] << " "; std::cout << "\n" << std::endl;
			for ( size_t i = 0; i < indices; ++i ) if ( ((uint32_t*) buffers.at(2).mapped)[i] > total ) total = ((uint32_t*) buffers.at(2).mapped)[i];
			buffers.at(2).unmap();
		}
		++total;
		std::cout << (total *= 2 + 3 + 3 + 4) << std::endl;
		{
			VK_CHECK_RESULT(buffers.at(1).map());
			std::cout << "VERTICES\n"; for ( size_t i = 0; i < total; ++i ) std::cout << ((float*) buffers.at(1).mapped)[i] << " "; std::cout << "\n" << std::endl;
			buffers.at(1).unmap();
		}
	}
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
				VK_FORMAT_R32G32B32_SFLOAT,
				offsetof(Vertex, position)
			),			
			// Location 1 : Texture coordinates
			ext::vulkan::initializers::vertexInputAttributeDescription(
				VERTEX_BUFFER_BIND_ID,
				1,
				VK_FORMAT_R32G32_SFLOAT,
				offsetof(Vertex, uv)
			),
			// Location 1 : Vertex normal
			ext::vulkan::initializers::vertexInputAttributeDescription(
				VERTEX_BUFFER_BIND_ID,
				2,
				VK_FORMAT_R32G32B32_SFLOAT,
				offsetof(Vertex, normal)
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
		pipelineCreateInfo.subpass = 0;

		initializePipeline(pipelineCreateInfo);
	}
	// Set descriptor pool
	initializeDescriptorPool({
		ext::vulkan::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1),
		ext::vulkan::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)
	}, 1);
	// Set descriptor set
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
			&texture.descriptor
		)
	});
}
void ext::vulkan::MeshGraphic::destroy() {
	texture.destroy();
	ext::vulkan::Graphic::destroy();
}