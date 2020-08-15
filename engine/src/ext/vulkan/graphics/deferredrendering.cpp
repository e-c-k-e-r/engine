#include <uf/ext/vulkan/initializers.h>
#include <uf/ext/vulkan/graphics/deferredrendering.h>
#include <uf/ext/vulkan/graphic.h>
#include <uf/ext/vulkan/vulkan.h>
#include <uf/ext/vulkan/texture.h>
#include <uf/ext/openvr/openvr.h>
#include <uf/utils/graphic/graphic.h>

namespace {
	uint32_t VERTEX_BUFFER_BIND_ID = 0;
	float r() {
		return (rand() % 100) / 100.0;
	}
}

size_t ext::vulkan::DeferredRenderingGraphic::maxLights = 32;

bool ext::vulkan::DeferredRenderingGraphic::autoAssignable() const {
	return false;
}
std::string ext::vulkan::DeferredRenderingGraphic::name() const {
	return "DeferredRenderingGraphic";
}
void ext::vulkan::DeferredRenderingGraphic::createCommandBuffer( VkCommandBuffer commandBuffer ) {
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
void ext::vulkan::DeferredRenderingGraphic::initialize( const std::string& renderMode ) {
	return initialize(this->device ? *device : ext::vulkan::device, ext::vulkan::getRenderMode(renderMode));
}
void ext::vulkan::DeferredRenderingGraphic::initialize( Device& device, RenderMode& renderMode ) {
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
	ext::vulkan::GraphicOld::initialize( device, renderMode );

	// set descriptor layout
	{
		std::vector<VkDescriptorSetLayoutBinding> bindings = {
			// Uniforms
			ext::vulkan::initializers::descriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				VK_SHADER_STAGE_FRAGMENT_BIT,
				0
			)
		};
		auto& subpass = renderMode.renderTarget.passes[this->subpass];
		for ( auto& input : subpass.inputs ) {
			bindings.push_back(ext::vulkan::initializers::descriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
				VK_SHADER_STAGE_FRAGMENT_BIT,
				bindings.size()
			));
		}
		initializeDescriptorLayout(bindings, sizeof(pushConstants));
	}

	// Create uniform buffer
	// max kludge
	{	
		size_t MAX_LIGHTS = ext::vulkan::DeferredRenderingGraphic::maxLights;
		struct UniformDescriptor {
			struct Matrices {
				alignas(16) pod::Matrix4f view[2];
				alignas(16) pod::Matrix4f projection[2];
			} matrices;
			alignas(16) pod::Vector4f ambient;
			struct Light {
				alignas(16) pod::Vector4f position;
				alignas(16) pod::Vector4f color;
			} lights;
		};
		size_t size = sizeof(UniformDescriptor) + sizeof(UniformDescriptor::Light) * (MAX_LIGHTS - 1);
		uint8_t buffer[size]; for ( size_t i = 0; i < size; ++i ) buffer[i] = 0;
		{
			UniformDescriptor* uniforms = (UniformDescriptor*) &buffer[0];
			uniforms->ambient = { 1, 1, 1, 1 };
			UniformDescriptor::Light* lights = (UniformDescriptor::Light*) &buffer[sizeof(UniformDescriptor) - sizeof(UniformDescriptor::Light)];
			for ( size_t i = 0; i < MAX_LIGHTS; ++i ) {
				UniformDescriptor::Light& light = lights[i];
				light.position = { 0, 0, 0, 0 };
				light.color = { 0, 0, 0, 0 };
			}
		}
		uniforms.create( size, (void*) &buffer );
	}
	pod::Userdata& userdata = uniforms.data();
	initializeBuffer(
		(void*) userdata.data,
		userdata.len,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		false
	);
/*
	initializeBuffer(
		(void*) &uniforms,
		sizeof(uniforms),
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		false
	);
*/
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
	// set pipeline
	{
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = ext::vulkan::initializers::pipelineInputAssemblyStateCreateInfo(
			VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			0,
			VK_FALSE
		);
		VkPipelineRasterizationStateCreateInfo rasterizationState = ext::vulkan::initializers::pipelineRasterizationStateCreateInfo(
			VK_POLYGON_MODE_FILL,
			VK_CULL_MODE_BACK_BIT,
		//	VK_CULL_MODE_NONE,
			VK_FRONT_FACE_CLOCKWISE,
			0
		);
		std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentStates;
		auto& subpass = renderMode.renderTarget.passes[this->subpass];
		for ( auto& input : subpass.colors ) {
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
			VK_FALSE,
			VK_FALSE,
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

		// Specialization Constants
		
		struct SpecializationConstants {
			uint32_t lights;
		} specializationConstants;
		// grab requested light count
		specializationConstants.lights = ext::vulkan::DeferredRenderingGraphic::maxLights;
		
		std::vector<VkSpecializationMapEntry> specializationMapEntries;
		{
			VkSpecializationMapEntry specializationMapEntry;
			specializationMapEntry.constantID = 0;
			specializationMapEntry.size = sizeof(specializationConstants.lights);
			specializationMapEntry.offset = offsetof(SpecializationConstants, lights);
			specializationMapEntries.push_back(specializationMapEntry);
		}
		VkSpecializationInfo specializationInfo{};
		specializationInfo.dataSize = sizeof(specializationConstants);
		specializationInfo.mapEntryCount = static_cast<uint32_t>(specializationMapEntries.size());
		specializationInfo.pMapEntries = specializationMapEntries.data();
		specializationInfo.pData = &specializationConstants;
		// fragment shader
		shader.stages[1].pSpecializationInfo = &specializationInfo;

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
	{
		std::vector<VkDescriptorImageInfo> inputDescriptors;
		auto& subpass = renderMode.renderTarget.passes[this->subpass];
		for ( auto& input : subpass.inputs ) {
			inputDescriptors.push_back(ext::vulkan::initializers::descriptorImageInfo( 
				renderTarget->attachments[input.attachment].view,
				input.layout
			));
		}
		// Set descriptor pool
		initializeDescriptorPool({
			ext::vulkan::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1),
			ext::vulkan::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, inputDescriptors.size()),
		}, 1);
		// Set descriptor set
		std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
			// Binding 0 : Projection/View matrix uniform buffer			
			ext::vulkan::initializers::writeDescriptorSet(
				descriptorSet,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				0,
				&(buffers.at(0).descriptor)
			)
		};
		for ( size_t i = 0; i < inputDescriptors.size(); ++i ) {
				writeDescriptorSets.push_back(ext::vulkan::initializers::writeDescriptorSet(
					descriptorSet,
					VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
					i + 1,
					&inputDescriptors[i]
				));
			}
		initializeDescriptorSet(writeDescriptorSets);
	}
}
void ext::vulkan::DeferredRenderingGraphic::destroy() {
	ext::vulkan::GraphicOld::destroy();
}