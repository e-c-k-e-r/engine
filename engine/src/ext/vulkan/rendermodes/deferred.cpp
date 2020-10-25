#include <uf/ext/vulkan/vulkan.h>
#include <uf/ext/vulkan/rendermodes/deferred.h>
#include <uf/ext/vulkan/rendermodes/rendertarget.h>
#include <uf/ext/vulkan/initializers.h>
#include <uf/utils/window/window.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/ext/vulkan/graphic.h>
#include <uf/engine/scene/scene.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/math/transform.h>

#include <uf/ext/vulkan/graphic.h>

std::string ext::vulkan::DeferredRenderMode::getType(  ) const {
	return "Deferred";
}

void ext::vulkan::DeferredRenderMode::initialize( Device& device ) {
	ext::vulkan::RenderMode::initialize( device );
	{
		renderTarget.device = &device;
		// attach targets
		struct {
			size_t albedo, normals, position, depth, output, ping, pong;
		} attachments;

		attachments.albedo = renderTarget.attach( VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, true ); // albedo
		attachments.normals = renderTarget.attach( VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, true ); // normals
		attachments.position = renderTarget.attach( VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, true ); // position
		attachments.depth = renderTarget.attach( device.formats.depth, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, true ); // depth
	//	attachments.ping = renderTarget.attach( VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL ); // albedo
	//	attachments.pong = renderTarget.attach( VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL ); // albedo
		// Attach swapchain's image as output
		{
			attachments.output = renderTarget.attachments.size();
			RenderTarget::Attachment swapchainAttachment;
			swapchainAttachment.format = device.formats.color;
			swapchainAttachment.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			swapchainAttachment.layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			swapchainAttachment.aliased = true;
			{
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
					blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
					blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
					blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
				}
				swapchainAttachment.blendState = blendAttachmentState;
			}
			renderTarget.attachments.push_back(swapchainAttachment);
		}

		// First pass: fill the G-Buffer
		{
			renderTarget.addPass(
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				{ attachments.albedo, attachments.normals, attachments.position },
				{},
				attachments.depth
			);
		}
		// Second pass: write to output
		{
			renderTarget.addPass(
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,
				{ attachments.output },
				{ attachments.albedo, attachments.normals, attachments.position },
				attachments.depth
			);
		}
	}
	renderTarget.initialize( device );

	{
		uf::BaseMesh<pod::Vertex_2F2F, uint16_t> mesh;
		mesh.vertices = {
			{ {-1.0f, 1.0f}, {0.0f, 1.0f}, },
			{ {-1.0f, -1.0f}, {0.0f, 0.0f}, },
			{ {1.0f, -1.0f}, {1.0f, 0.0f}, },
			{ {1.0f, 1.0f}, {1.0f, 1.0f}, }
		};
		mesh.indices = {
			0, 1, 2, 0, 2, 3
		};
		blitter.descriptor.subpass = 1;
		blitter.descriptor.depthTest.test = false;
		blitter.descriptor.depthTest.write = false;

		blitter.initialize( this->getName() );
		blitter.initializeGeometry( mesh );
		blitter.material.initializeShaders({
			{"./data/shaders/display.subpass.vert.spv", VK_SHADER_STAGE_VERTEX_BIT},
			{"./data/shaders/display.subpass.stereo.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT}
		});
		{
			auto& scene = uf::scene::getCurrentScene();
			auto& metadata = scene.getComponent<uf::Serializer>();

			auto& shader = blitter.material.shaders.back();
			struct SpecializationConstant {
				uint32_t maxLights = 16;
			};
			auto* specializationConstants = (SpecializationConstant*) &shader.specializationConstants[0];
			specializationConstants->maxLights = metadata["system"]["config"]["engine"]["scenes"]["max lights"].asUInt64();
			for ( auto& binding : shader.descriptorSetLayoutBindings ) {
				if ( binding.descriptorCount > 1 )
					binding.descriptorCount = specializationConstants->maxLights;
			}
		/*
			std::vector<pod::Vector4f> palette;
			// size of palette
			if ( metadata["system"]["config"]["engine"]["scenes"]["palette"].isNumeric() ) {
				size_t size = metadata["system"]["config"]["engine"]["scenes"]["palette"].asUInt64();
				for ( size_t x = 0; x < size; ++x ) {
					palette.push_back(pod::Vector4f{
						(128.0f + 128.0f * sin(3.1415f * x / 16.0f)) / 256.0f,
						(128.0f + 128.0f * sin(3.1415f * x / 32.0f)) / 256.0f,
						(128.0f + 128.0f * sin(3.1415f * x / 64.0f)) / 256.0f,
						1.0f,
					});
				}
			// palette array
			} else if ( metadata["system"]["config"]["engine"]["scenes"]["palette"].isArray() ) {
				for ( int i = 0; i < metadata["system"]["config"]["engine"]["scenes"]["palette"].size(); ++i ) {
					auto& color = palette.emplace_back();
					palette.push_back(pod::Vector4f{
						metadata["system"]["config"]["engine"]["scenes"]["palette"][i][0].asFloat(),
						metadata["system"]["config"]["engine"]["scenes"]["palette"][i][1].asFloat(),
						metadata["system"]["config"]["engine"]["scenes"]["palette"][i][2].asFloat(),
						metadata["system"]["config"]["engine"]["scenes"]["palette"][i][3].asFloat(),
					});
				}
			}
			if ( !palette.empty() ) {
				shader.initializeBuffer(
					(void*) palette.data(),
					palette.size() * sizeof(pod::Vector4f),
					VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
					true
				);
			}
		*/
		}
		blitter.initializePipeline();
	}
}
void ext::vulkan::DeferredRenderMode::tick() {
	ext::vulkan::RenderMode::tick();
	
	if ( ext::vulkan::states::resized ) {
		renderTarget.initialize( *renderTarget.device );
		// update blitter descriptor set
		if ( blitter.initialized ) {
			blitter.getPipeline().update( blitter );
		//	blitter.updatePipelines();
		}
	}
}
void ext::vulkan::DeferredRenderMode::destroy() {
	ext::vulkan::RenderMode::destroy();
	blitter.destroy();
}
void ext::vulkan::DeferredRenderMode::createCommandBuffers( const std::vector<ext::vulkan::Graphic*>& graphics ) {
	float width = this->width > 0 ? this->width : ext::vulkan::settings::width;
	float height = this->height > 0 ? this->height : ext::vulkan::settings::height;

	VkCommandBufferBeginInfo cmdBufInfo = {};
	cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBufInfo.pNext = nullptr;

	std::vector<RenderMode*> layers = ext::vulkan::getRenderModes(std::vector<std::string>{"RenderTarget", "Compute"}, false);
	auto& scene = uf::scene::getCurrentScene();
	auto& metadata = scene.getComponent<uf::Serializer>();
	auto& commands = getCommands();
	for (size_t i = 0; i < commands.size(); ++i) {
		VK_CHECK_RESULT(vkBeginCommandBuffer(commands[i], &cmdBufInfo));
		// Fill GBuffer
		{
			std::vector<VkClearValue> clearValues;
			for ( auto& attachment : renderTarget.attachments ) {
				VkClearValue clearValue;
				if ( attachment.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT ) {
					if ( !metadata["system"]["renderer"]["clear values"][(int) clearValues.size()].isNull() ) {
						auto& v = metadata["system"]["renderer"]["clear values"][(int) clearValues.size()];
						clearValue.color = { {
							v[0].asFloat(),
							v[1].asFloat(),
							v[2].asFloat(),
							v[3].asFloat(),
						} };
					} else {
						clearValue.color = { { 0, 0, 0, 0 } };
					}
				} else if ( attachment.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT ) {
					clearValue.depthStencil = { 0.0f, 0 };
				}
				clearValues.push_back(clearValue);
			}

			VkRenderPassBeginInfo renderPassBeginInfo = {};
			renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassBeginInfo.pNext = nullptr;
			renderPassBeginInfo.renderArea.offset.x = 0;
			renderPassBeginInfo.renderArea.offset.y = 0;
			renderPassBeginInfo.renderArea.extent.width = width;
			renderPassBeginInfo.renderArea.extent.height = height;
			renderPassBeginInfo.clearValueCount = clearValues.size();
			renderPassBeginInfo.pClearValues = &clearValues[0];
			renderPassBeginInfo.renderPass = renderTarget.renderPass;
			renderPassBeginInfo.framebuffer = renderTarget.framebuffers[i];

			// Update dynamic viewport state
			VkViewport viewport = {};
			viewport.width = (float) width;
			viewport.height = (float) height;
			viewport.minDepth = (float) 0.0f;
			viewport.maxDepth = (float) 1.0f;
			
			// Update dynamic scissor state
			VkRect2D scissor = {};
			scissor.extent.width = width;
			scissor.extent.height = height;
			scissor.offset.x = 0;
			scissor.offset.y = 0;

			// transition layers for read
			for ( auto layer : layers ) {
				layer->pipelineBarrier( commands[i], 0 );
			}
		
			vkCmdBeginRenderPass(commands[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
				vkCmdSetViewport(commands[i], 0, 1, &viewport);
				vkCmdSetScissor(commands[i], 0, 1, &scissor);
				for ( auto graphic : graphics ) {
					// only draw graphics that are assigned to this type of render mode
					if ( graphic->descriptor.renderMode != this->getName() ) continue;
					graphic->record( commands[i] );
				}
				// blit any RT's
				{
					for ( auto _ : layers ) {
						RenderTargetRenderMode* layer = (RenderTargetRenderMode*) _;
						auto& blitter = layer->blitter;
						if ( !blitter.initialized || !blitter.process || blitter.descriptor.subpass != 0 ) continue;
						blitter.record(commands[i]);
					}
				}
			vkCmdNextSubpass(commands[i], VK_SUBPASS_CONTENTS_INLINE);
				{
					blitter.record(commands[i]);
				}
				// blit any RT's
				{
					for ( auto _ : layers ) {
						RenderTargetRenderMode* layer = (RenderTargetRenderMode*) _;
						auto& blitter = layer->blitter;
						if ( !blitter.initialized || !blitter.process || blitter.descriptor.subpass != 1 ) continue;
						blitter.record(commands[i]);
					}
				}
			vkCmdEndRenderPass(commands[i]);

			for ( auto layer : layers ) {
				layer->pipelineBarrier( commands[i], 1 );
			}
		}

		VK_CHECK_RESULT(vkEndCommandBuffer(commands[i]));
	}
}