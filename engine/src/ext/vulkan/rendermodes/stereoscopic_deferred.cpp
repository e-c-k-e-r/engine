#include <uf/ext/vulkan/vulkan.h>
#include <uf/ext/vulkan/rendermodes/deferred.h>
#include <uf/ext/vulkan/rendermodes/stereoscopic_deferred.h>
#include <uf/ext/vulkan/rendermodes/rendertarget.h>
#include <uf/ext/vulkan/initializers.h>
#include <uf/utils/window/window.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/ext/vulkan/graphic.h>
#include <uf/engine/scene/scene.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/math/transform.h>
#include <uf/ext/openvr/openvr.h>
#include <uf/ext/gltf/graph.h>

// ext::vulkan::StereoscopicDeferredRenderMode::StereoscopicDeferredRenderMode() : renderTargets({ renderTarget }), blitters({ blitter }) {
ext::vulkan::StereoscopicDeferredRenderMode::StereoscopicDeferredRenderMode() : renderTargets({ renderTarget }) {
}

const std::string ext::vulkan::StereoscopicDeferredRenderMode::getType() const {
	return "Deferred (Stereoscopic)";
}
ext::vulkan::RenderTarget& ext::vulkan::StereoscopicDeferredRenderMode::getRenderTarget( size_t i ) {
	switch ( i ) {
		case 0: return renderTargets.left;
		case 1: return renderTargets.right;
		default: return renderTarget;
	}
}
const ext::vulkan::RenderTarget& ext::vulkan::StereoscopicDeferredRenderMode::getRenderTarget( size_t i ) const {
	switch ( i ) {
		case 0: return renderTargets.left;
		case 1: return renderTargets.right;
		default: return renderTarget;
	}
}
const size_t ext::vulkan::StereoscopicDeferredRenderMode::blitters() const {
	return 2;
}
ext::vulkan::Graphic* ext::vulkan::StereoscopicDeferredRenderMode::getBlitter( size_t i ) {
	switch ( i ) {
		case 0: return &this->renderBlitters.left;
		case 1: return &this->renderBlitters.right;
		default: return &this->renderBlitters.left;
	}
}
std::vector<ext::vulkan::Graphic*> ext::vulkan::StereoscopicDeferredRenderMode::getBlitters() {
	return { &this->renderBlitters.left, &this->renderBlitters.right };
}

void ext::vulkan::StereoscopicDeferredRenderMode::initialize( Device& device ) {
	ext::vulkan::RenderMode::initialize( device );
	
	struct EYES {
		RenderTarget* renderTarget;
		Graphic* blitter;
	};
	std::vector<EYES> eyes = {
		{ &renderTargets.left, &renderBlitters.left },
		{ &renderTargets.right, &renderBlitters.right },
	};
	std::size_t i = 0;
	for ( auto& eye : eyes ) {
		auto& renderTarget = *eye.renderTarget;
		auto& blitter = *eye.blitter;
		renderTarget.device = &device;
		renderTarget.width = this->width;
		renderTarget.height = this->height;

		struct {
			size_t id, normals, uvs, albedo, depth, output;
		} attachments;
		size_t msaa = ext::vulkan::settings::msaa;

		attachments.id = renderTarget.attach(RenderTarget::Attachment::Descriptor{
			/*.format = */VK_FORMAT_R16G16_UINT,
			/*.layout = */VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			/*.usage = */VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
			/*.blend = */false,
			/*.samples = */msaa,
		});
		attachments.normals = renderTarget.attach(RenderTarget::Attachment::Descriptor{
			/*.format = */VK_FORMAT_R16G16_SFLOAT,
			/*.layout = */VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			/*.usage = */VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
			/*.blend = */false,
			/*.samples = */msaa,
		});
		if ( ext::vulkan::settings::experimental::deferredMode == "deferredSampling" ) {
			attachments.uvs = renderTarget.attach(RenderTarget::Attachment::Descriptor{
				/*.format = */VK_FORMAT_R16G16_UNORM,
				/*.layout = */VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				/*.usage = */VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
				/*.blend = */false,
				/*.samples = */msaa,
			});
		} else {
			attachments.albedo = renderTarget.attach(RenderTarget::Attachment::Descriptor{
				/*.format = */VK_FORMAT_R8G8B8A8_UNORM,
				/*.layout = */VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				/*.usage = */VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
				/*.blend = */false,
				/*.samples = */msaa,
			});
		}
		attachments.depth = renderTarget.attach(RenderTarget::Attachment::Descriptor{
			/*.format = */ext::vulkan::settings::formats::depth,
			/*.layout = */VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			/*.usage = */VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
			/*.blend = */false,
			/*.samples = */msaa,
		});
		attachments.output = renderTarget.attach(RenderTarget::Attachment::Descriptor{
			/*.format =*/ VK_FORMAT_R8G8B8A8_UNORM,
			/*.layout = */ VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			/*.usage =*/ VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			/*.blend =*/ true,
			/*.samples =*/ 1,
		});
		if ( ext::vulkan::settings::experimental::deferredMode == "deferredSampling" ) {
			// First pass: fill the G-Buffer
			{
				renderTarget.addPass(
					VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
					{ attachments.id, attachments.normals, attachments.uvs },
					{},
					{},
					attachments.depth
				);
			}
			// Second pass: write to output
			{
				renderTarget.addPass(
					VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,
					{ attachments.output },
					{ attachments.id, attachments.normals, attachments.uvs, attachments.depth },
					{},
					attachments.depth
				);
			}
		} else {
			// First pass: fill the G-Buffer
			{
				renderTarget.addPass(
					VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
					{ attachments.id, attachments.normals, attachments.albedo },
					{},
					{},
					attachments.depth
				);
			}
			// Second pass: write to output
			{
				renderTarget.addPass(
					VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,
					{ attachments.output },
					{ attachments.id, attachments.normals, attachments.albedo, attachments.depth },
					{},
					attachments.depth
				);
			}
		}
	#if 0
		// attach targets
		struct {
			size_t albedo, normals, position, depth, output;
		} attachments;
		attachments.albedo = renderTarget.attach( ext::vulkan::settings::formats::color, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, false ); // albedo
		attachments.normals = renderTarget.attach( ext::vulkan::settings::formats::normal, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, false ); // normals
		if ( !settings::experimental::deferredReconstructPosition )
			attachments.position = renderTarget.attach( ext::vulkan::settings::formats::position, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, false ); // position
		attachments.depth = renderTarget.attach( ext::vulkan::settings::formats::depth, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, false ); // depth
		attachments.output = renderTarget.attach( ext::vulkan::settings::formats::color, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, true ); // albedo
	//	attachments.ping = renderTarget.attach( VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL ); // albedo
	//	attachments.pong = renderTarget.attach( VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL ); // albedo
	/*
		// Attach swapchain's image as output
		if ( !true ) {
			attachments.swapchain = renderTarget.attachments.size();
			RenderTarget::Attachment swapchainAttachment;
			swapchainAttachment.format = device.formats.color;
			swapchainAttachment.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			swapchainAttachment.layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			swapchainAttachment.aliased = true;
			renderTarget.attachments.push_back(swapchainAttachment);
		}
	*/
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
	#endif
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

			blitter.material.initializeShaders({
				{"./data/shaders/display.subpass.vert.spv", VK_SHADER_STAGE_VERTEX_BIT},
				{"./data/shaders/display.subpass.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT}
			});
			{
				auto& scene = uf::scene::getCurrentScene();

				auto& shader = blitter.material.shaders.back();
				struct SpecializationConstant {
					uint32_t maxTextures = 512;
				};
				auto& specializationConstants = shader.specializationConstants.get<SpecializationConstant>();

				auto& metadata = scene.getComponent<uf::Serializer>();
				size_t maxLights = metadata["system"]["config"]["engine"]["scenes"]["lights"]["max"].as<size_t>();
				specializationConstants.maxTextures = metadata["system"]["config"]["engine"]["scenes"]["textures"]["max"].as<size_t>();
				for ( auto& binding : shader.descriptorSetLayoutBindings ) {
					if ( binding.descriptorCount > 1 )
						binding.descriptorCount = specializationConstants.maxTextures;
				}

				std::vector<pod::Light::Storage> lights(maxLights);
				std::vector<pod::Material::Storage> materials(specializationConstants.maxTextures);
				std::vector<pod::Texture::Storage> textures(specializationConstants.maxTextures);
				std::vector<pod::DrawCall::Storage> drawCalls(specializationConstants.maxTextures);

				for ( auto& material : materials ) material.colorBase = {0,0,0,0};

				this->metadata["lightBufferIndex"] = blitter.initializeBuffer(
					(void*) lights.data(),
					lights.size() * sizeof(pod::Light::Storage),
					VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
				);
				this->metadata["materialBufferIndex"] = blitter.initializeBuffer(
					(void*) materials.data(),
					materials.size() * sizeof(pod::Material::Storage),
					VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
				);

				this->metadata["textureBufferIndex"] = blitter.initializeBuffer(
					(void*) textures.data(),
					textures.size() * sizeof(pod::Texture::Storage),
					VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
				);

				this->metadata["drawCallBufferIndex"] = blitter.initializeBuffer(
					(void*) drawCalls.data(),
					drawCalls.size() * sizeof(pod::DrawCall::Storage),
					VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
				);
			}
			blitter.initializePipeline();
		}
	}
}
void ext::vulkan::StereoscopicDeferredRenderMode::tick() {
	ext::vulkan::RenderMode::tick();
	if ( ext::vulkan::states::resized ) {
		struct EYES {
			RenderTarget* renderTarget;
			Graphic* blitter;
		};
		std::vector<EYES> eyes = {
			{ &renderTargets.left, &renderBlitters.left },
			{ &renderTargets.right, &renderBlitters.right },
		};
		for ( auto& eye : eyes ) {
			auto& renderTarget = *eye.renderTarget;
			auto& blitter = *eye.blitter;

			renderTarget.initialize( *renderTarget.device );
			if ( blitter.initialized ) {
				blitter.getPipeline().update( blitter );
			//	blitter.updatePipelines();
			}
		}
	}
}
void ext::vulkan::StereoscopicDeferredRenderMode::destroy() {
	ext::vulkan::RenderMode::destroy();
	renderTargets.right.destroy();
	renderBlitters.left.destroy();
	renderBlitters.right.destroy();
//	blitter.destroy();
}
void ext::vulkan::StereoscopicDeferredRenderMode::createCommandBuffers( const std::vector<ext::vulkan::Graphic*>& graphics ) {
	// destroy if exists
	// ext::vulkan::RenderMode& swapchain = 
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
	for (size_t i = 0; i < commands.size(); ++i) {
		VK_CHECK_RESULT(vkBeginCommandBuffer(commands[i], &cmdBufInfo));
		struct EYES {
			RenderTarget* renderTarget;
			Graphic* blitter;
		};
		std::vector<EYES> eyes = {
			{ &renderTargets.left, &renderBlitters.left },
			{ &renderTargets.right, &renderBlitters.right },
		};
		for ( size_t eyePass = 0; eyePass < eyes.size(); ++eyePass ) {
			auto& renderTarget = *eyes[ext::openvr::renderPass].renderTarget;
			auto& blitter = *eyes[ext::openvr::renderPass].blitter;
			// Fill GBuffer
			{
				std::vector<VkClearValue> clearValues;
				for ( auto& attachment : renderTarget.attachments ) {
					VkClearValue clearValue;
					if ( attachment.descriptor.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT ) {
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
					} else if ( attachment.descriptor.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT ) {
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
					size_t currentDraw = 0;
					for ( auto graphic : graphics ) {
						// only draw graphics that are assigned to this type of render mode
						if ( graphic->descriptor.renderMode != this->getName() ) continue;
						ext::vulkan::GraphicDescriptor descriptor = bindGraphicDescriptor(graphic->descriptor);
						graphic->record( commands[i], descriptor, eyePass, currentDraw++ );
					}
					// render gui layer
					{
						for ( auto _ : layers ) {
							RenderTargetRenderMode* layer = (RenderTargetRenderMode*) _;
							auto& blitter = layer->blitter;
							if ( !blitter.initialized || !blitter.process || blitter.descriptor.subpass != currentPass ) continue;
							blitter.record(commands[i]);
						}
					}
				vkCmdNextSubpass(commands[i], VK_SUBPASS_CONTENTS_INLINE); ++currentPass;
					{
						blitter.record(commands[i]);
					}
					// render gui layer
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
			}
			// Blit eye to swapchain
			if ( ext::openvr::dominantEye == ext::openvr::renderPass ) {
			//	if ( false ) {
				// transition swapchain to proper layout
				auto& swapchainRender = ext::vulkan::getRenderMode("Swapchain");
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
						imageMemoryBarrier.oldLayout = outputAttachment.descriptor.layout;
						imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
						vkCmdPipelineBarrier( commands[i], VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, NULL, 0, NULL, 1, &imageMemoryBarrier );
						outputAttachment.descriptor.layout = imageMemoryBarrier.newLayout;
					}
					{
						imageMemoryBarrier.image = swapchainRender.renderTarget.attachments[i].image;
						imageMemoryBarrier.srcAccessMask = 0;
						imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
						imageMemoryBarrier.oldLayout = swapchainRender.renderTarget.attachments[i].descriptor.layout;
						imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
						vkCmdPipelineBarrier( commands[i], VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, NULL, 0, NULL, 1, &imageMemoryBarrier );
						swapchainRender.renderTarget.attachments[i].descriptor.layout = imageMemoryBarrier.newLayout;
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
						imageMemoryBarrier.oldLayout = outputAttachment.descriptor.layout;
						imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
						vkCmdPipelineBarrier( commands[i], VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, NULL, 0, NULL, 1, &imageMemoryBarrier );
						outputAttachment.descriptor.layout = imageMemoryBarrier.newLayout;
					}
					{
						imageMemoryBarrier.image = swapchainRender.renderTarget.attachments[i].image;
						imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
						imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
						imageMemoryBarrier.oldLayout = swapchainRender.renderTarget.attachments[i].descriptor.layout;
						imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
						vkCmdPipelineBarrier( commands[i], VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, NULL, 0, NULL, 1, &imageMemoryBarrier );
						swapchainRender.renderTarget.attachments[i].descriptor.layout = imageMemoryBarrier.newLayout;
					}
				}
			}
		}
		VK_CHECK_RESULT(vkEndCommandBuffer(commands[i]));
	}
	ext::openvr::renderPass = 0;
}