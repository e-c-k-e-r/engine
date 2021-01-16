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

const std::string ext::vulkan::DeferredRenderMode::getType() const {
	return "Deferred";
}
const size_t ext::vulkan::DeferredRenderMode::blitters() const {
	return 1;
}
ext::vulkan::Graphic* ext::vulkan::DeferredRenderMode::getBlitter( size_t i ) {
	return &this->blitter;
}
std::vector<ext::vulkan::Graphic*> ext::vulkan::DeferredRenderMode::getBlitters() {
	return { &this->blitter };
}

void ext::vulkan::DeferredRenderMode::initialize( Device& device ) {
	{
		float width = this->width > 0 ? this->width : ext::vulkan::settings::width;
		float height = this->height > 0 ? this->height : ext::vulkan::settings::height;
		auto& swapchainRender = ext::vulkan::getRenderMode("Swapchain");
		if ( swapchainRender.renderTarget.width != width || swapchainRender.renderTarget.height != height ) {
			settings::experimental::deferredAliasOutputToSwapchain = false;
		}
	}

	ext::vulkan::RenderMode::initialize( device );
	{
		renderTarget.device = &device;
		// attach targets
		struct {
			size_t albedo, normals, position, depth, output, ping, pong;
		} attachments;

		attachments.albedo = renderTarget.attach( ext::vulkan::settings::formats::color, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, false ); // albedo
		attachments.normals = renderTarget.attach( ext::vulkan::settings::formats::normal, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, false ); // normals
		if ( !settings::experimental::deferredReconstructPosition )
			attachments.position = renderTarget.attach( ext::vulkan::settings::formats::position, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, false ); // position
		attachments.depth = renderTarget.attach( ext::vulkan::settings::formats::depth, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, false ); // depth

	//	attachments.ping = renderTarget.attach( VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL ); // albedo
	//	attachments.pong = renderTarget.attach( VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL ); // albedo

		// Attach swapchain's image as output
		if ( settings::experimental::deferredAliasOutputToSwapchain ) {
			attachments.output = renderTarget.attachments.size();
			RenderTarget::Attachment swapchainAttachment;
			swapchainAttachment.format = ext::vulkan::settings::formats::color; //device.formats.color;
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
		} else {
			attachments.output = renderTarget.attach( VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, true ); // albedo
		}

		// First pass: fill the G-Buffer
		if ( settings::experimental::deferredReconstructPosition ) {
			renderTarget.addPass(
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				{ attachments.albedo, attachments.normals },
				{},
				attachments.depth
			);
		} else {
			renderTarget.addPass(
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				{ attachments.albedo, attachments.normals, attachments.position },
				{},
				attachments.depth
			);
		}
		// Second pass: write to output
		if ( settings::experimental::deferredReconstructPosition ) {
			renderTarget.addPass(
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,
				{ attachments.output },
				{ attachments.albedo, attachments.normals, attachments.depth },
				attachments.depth
			);
		} else {
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
		uf::BaseMesh<pod::Vertex_2F2F> mesh;
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
		blitter.descriptor.depth.test = false;
		blitter.descriptor.depth.write = false;

		blitter.initialize( this->getName() );
		blitter.initializeGeometry( mesh );
		if ( settings::experimental::deferredReconstructPosition ) {
			blitter.material.initializeShaders({
				{"./data/shaders/display.subpass.vert.spv", VK_SHADER_STAGE_VERTEX_BIT},
				{"./data/shaders/display.subpass.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT}
			});
		} else {
			blitter.material.initializeShaders({
				{"./data/shaders/display.subpass.vert.spv", VK_SHADER_STAGE_VERTEX_BIT},
				{"./data/shaders/display.subpass.stereo.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT}
			});
		}
		{
			auto& scene = uf::scene::getCurrentScene();
			auto& metadata = scene.getComponent<uf::Serializer>();

			auto& shader = blitter.material.shaders.back();
			struct SpecializationConstant {
				uint32_t maxLights = 16;
			};
			auto& specializationConstants = shader.specializationConstants.get<SpecializationConstant>();
			specializationConstants.maxLights = metadata["system"]["config"]["engine"]["scenes"]["lights"]["max"].as<size_t>();
			for ( auto& binding : shader.descriptorSetLayoutBindings ) {
				if ( binding.descriptorCount > 1 )
					binding.descriptorCount = specializationConstants.maxLights;
			}

		/*
			struct Light {
				alignas(16) pod::Vector4f position;
				alignas(16) pod::Vector4f color;

				alignas(4) int32_t type = 0;
				alignas(4) float depthBias = 0;
				alignas(4) float padding1 = 0;
				alignas(4) float padding2 = 0;
				
				alignas(16) pod::Matrix4f view;
				alignas(16) pod::Matrix4f projection;
			};
			std::vector<Light> lights;
			lights.resize(specializationConstants.maxLights);

			blitter.initializeBuffer(
				(void*) lights.data(),
				lights.size() * sizeof(Light),
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				true
			);
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

	VkImageMemoryBarrier imageMemoryBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // ext::vulkan::device.queueFamilyIndices.graphics; //VK_QUEUE_FAMILY_IGNORED
	imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // ext::vulkan::device.queueFamilyIndices.graphics; //VK_QUEUE_FAMILY_IGNORED
	imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
	imageMemoryBarrier.subresourceRange.levelCount = 1;
	imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
	imageMemoryBarrier.subresourceRange.layerCount = 1;
	imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

	std::vector<RenderMode*> layers = ext::vulkan::getRenderModes(std::vector<std::string>{"RenderTarget", "Compute"}, false);
	auto& scene = uf::scene::getCurrentScene();
	auto& metadata = scene.getComponent<uf::Serializer>();
	auto& commands = getCommands();
	auto& swapchainRender = ext::vulkan::getRenderMode("Swapchain");
	for (size_t i = 0; i < commands.size(); ++i) {
		VK_CHECK_RESULT(vkBeginCommandBuffer(commands[i], &cmdBufInfo));
		// Fill GBuffer
		{
			std::vector<VkClearValue> clearValues;
			for ( auto& attachment : renderTarget.attachments ) {
				VkClearValue clearValue;
				if ( attachment.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT ) {
					if ( !ext::json::isNull( metadata["system"]["renderer"]["clear values"][(int) clearValues.size()] ) ) {
						auto& v = metadata["system"]["renderer"]["clear values"][(int) clearValues.size()];
						clearValue.color = { {
							v[0].as<float>(),
							v[1].as<float>(),
							v[2].as<float>(),
							v[3].as<float>(),
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

			size_t currentPass = 0;
			vkCmdBeginRenderPass(commands[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
				vkCmdSetViewport(commands[i], 0, 1, &viewport);
				vkCmdSetScissor(commands[i], 0, 1, &scissor);
				// render to geometry buffers
				for ( auto graphic : graphics ) {
					// only draw graphics that are assigned to this type of render mode
					if ( graphic->descriptor.renderMode != this->getName() ) continue;
					ext::vulkan::GraphicDescriptor descriptor = bindGraphicDescriptor(graphic->descriptor);
					graphic->record( commands[i], descriptor, currentPass );
				}
				// blit any RT's that request this subpass
				{
					for ( auto _ : layers ) {
						RenderTargetRenderMode* layer = (RenderTargetRenderMode*) _;
						auto& blitter = layer->blitter;
						if ( !blitter.initialized || !blitter.process || blitter.descriptor.subpass != currentPass ) continue;
						blitter.record(commands[i]);
					}
				}
			vkCmdNextSubpass(commands[i], VK_SUBPASS_CONTENTS_INLINE); ++currentPass;
				// deferred post-processing lighting pass
				{
					blitter.record(commands[i]);
				}
				// blit any RT's that request this subpass
				{
					for ( auto _ : layers ) {
						RenderTargetRenderMode* layer = (RenderTargetRenderMode*) _;
						auto& blitter = layer->blitter;
						if ( !blitter.initialized || !blitter.process || blitter.descriptor.subpass != currentPass ) continue;
						blitter.record(commands[i]);
					}
				}
			vkCmdEndRenderPass(commands[i]);

			for ( auto layer : layers ) {
				layer->pipelineBarrier( commands[i], 1 );
			}
			
			if ( !settings::experimental::deferredAliasOutputToSwapchain ) {
				{
					auto& renderTarget = swapchainRender.renderTarget;
					float width = renderTarget.width;
					float height = renderTarget.height;

					std::vector<VkClearValue> clearValues; clearValues.resize(2);
					clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
					clearValues[1].depthStencil = { 1.0f, 0 };

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

					vkCmdBeginRenderPass(commands[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
					vkCmdEndRenderPass(commands[i]);
				}

				{
					VkImageBlit imageBlitRegion{};
					imageBlitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					imageBlitRegion.srcSubresource.layerCount = 1;
					imageBlitRegion.srcOffsets[1] = { width, height, 1 };
					imageBlitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					imageBlitRegion.dstSubresource.layerCount = 1;
					imageBlitRegion.dstOffsets[1] = {
						swapchainRender.width > 0 ? swapchainRender.width : ext::vulkan::settings::width,
						swapchainRender.height > 0 ? swapchainRender.height : ext::vulkan::settings::height,
						1
					};

					auto& outputAttachment = renderTarget.attachments[renderTarget.attachments.size()-1];
					// Transition to KHR
					{
						imageMemoryBarrier.image = outputAttachment.image;
						imageMemoryBarrier.srcAccessMask = 0;
						imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
						imageMemoryBarrier.oldLayout = outputAttachment.layout;
						imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
						vkCmdPipelineBarrier( commands[i], VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, NULL, 0, NULL, 1, &imageMemoryBarrier );
						outputAttachment.layout = imageMemoryBarrier.newLayout;
					}
					{
						imageMemoryBarrier.image = swapchainRender.renderTarget.attachments[i].image;
						imageMemoryBarrier.srcAccessMask = 0;
						imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
						imageMemoryBarrier.oldLayout = swapchainRender.renderTarget.attachments[i].layout;
						imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
						vkCmdPipelineBarrier( commands[i], VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, NULL, 0, NULL, 1, &imageMemoryBarrier );
						swapchainRender.renderTarget.attachments[i].layout = imageMemoryBarrier.newLayout;
					}

					vkCmdBlitImage(
						commands[i],
						outputAttachment.image,
						VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
						swapchainRender.renderTarget.attachments[i].image,
						VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						1,
						&imageBlitRegion,
						settings::swapchainUpscaleFilter
					);

					{
						imageMemoryBarrier.image = outputAttachment.image;
						imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
						imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
						imageMemoryBarrier.oldLayout = outputAttachment.layout;
						imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
						vkCmdPipelineBarrier( commands[i], VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, NULL, 0, NULL, 1, &imageMemoryBarrier );
						outputAttachment.layout = imageMemoryBarrier.newLayout;
					}
					{
						imageMemoryBarrier.image = swapchainRender.renderTarget.attachments[i].image;
						imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
						imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
						imageMemoryBarrier.oldLayout = swapchainRender.renderTarget.attachments[i].layout;
						imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
						vkCmdPipelineBarrier( commands[i], VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, NULL, 0, NULL, 1, &imageMemoryBarrier );
						swapchainRender.renderTarget.attachments[i].layout = imageMemoryBarrier.newLayout;
					}
				}
			}
		}

		VK_CHECK_RESULT(vkEndCommandBuffer(commands[i]));
	}
}