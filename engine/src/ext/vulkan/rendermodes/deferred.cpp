#include <uf/ext/vulkan/vulkan.h>
#include <uf/ext/vulkan/rendermodes/deferred.h>
#include <uf/ext/vulkan/rendermodes/rendertarget.h>
#include <uf/ext/vulkan/initializers.h>
#include <uf/utils/window/window.h>
#include <uf/utils/mesh/mesh.h>

namespace {
	// 0 left 1 right
	uint8_t DOMINANT_EYE = 1;
}

std::string ext::vulkan::DeferredRenderMode::getType() const {
	return "Defered";
}
size_t ext::vulkan::DeferredRenderMode::subpasses() const {
	return renderTarget.passes.size();
}
VkRenderPass& ext::vulkan::DeferredRenderMode::getRenderPass() {
	return renderTarget.renderPass;
}

void ext::vulkan::DeferredRenderMode::initialize( Device& device ) {
	ext::vulkan::RenderMode::initialize( device );
	{
		renderTarget.device = &device;
		// attach targets
		struct {
			size_t albedo, normals, depth, output;
		} attachments;

		attachments.albedo = renderTarget.attach( VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT ); // albedo
		attachments.normals = renderTarget.attach( VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT ); // normals
		attachments.depth = renderTarget.attach( swapchain.depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT ); // depth
		// Attach swapchain's image as output
		{
			attachments.output = renderTarget.attachments.size();
			renderTarget.attachments.push_back({ device.formats.color, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, });
		}

		// First pass: fill the G-Buffer
		{
			renderTarget.addPass(
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				{ attachments.albedo, attachments.normals, },
				{},
				attachments.depth
			);
		}
		// Second pass: write to output
		{
			renderTarget.addPass(
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT,
				{ attachments.output, },
				{ attachments.albedo, attachments.normals },
				attachments.depth
			);
		}
	}
	renderTarget.initialize( device );

	blitter.framebuffer = &framebuffer;
	blitter.initializeShaders({
		{"./data/shaders/display.subpass.vert.spv", VK_SHADER_STAGE_VERTEX_BIT},
		{"./data/shaders/display.subpass.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT}
	});
	blitter.initialize( device, *this );
}
void ext::vulkan::DeferredRenderMode::destroy() {
	renderTarget.destroy();
}
void ext::vulkan::DeferredRenderMode::createCommandBuffers( const std::vector<ext::vulkan::Graphic*>& graphics, const std::vector<std::string>& passes ) {
	// destroy if exists
	// ext::vulkan::RenderMode& swapchain = 
	if ( ext::vulkan::rebuild ) {
		if ( swapchain.initialized ) {
			auto* device = swapchain.device;
			swapchain.destroy();
			swapchain.initialize( *device );
		}
		swapchain.initialized = true;

		// destroy if exist
		if ( renderTarget.initialized ) {
			auto* device = renderTarget.device;
			renderTarget.initialize( *device );
		}
		renderTarget.initialized = true;

		// update descriptor set
		if ( blitter.initialized ) {
			VkDescriptorImageInfo textDescriptorAlbedo = ext::vulkan::initializers::descriptorImageInfo( 
				renderTarget.attachments[0].view,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			);
			std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
				// Binding 0 : Projection/View matrix uniform buffer			
				ext::vulkan::initializers::writeDescriptorSet(
					blitter.descriptorSet,
					VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					0,
					&(blitter.buffers.at(0).descriptor)
				),
				// Binding 1 : Albedo input attachment
				ext::vulkan::initializers::writeDescriptorSet(
					blitter.descriptorSet,
					VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
					1,
					&textDescriptorAlbedo
				),
			};
			vkUpdateDescriptorSets( *device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr );
		}
	}

	float width = this->width > 0 ? this->width : ext::vulkan::width;
	float height = this->height > 0 ? this->height : ext::vulkan::height;
	blitter.uniforms.screenSize = { width, height };

	VkCommandBufferBeginInfo cmdBufInfo = {};
	cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBufInfo.pNext = nullptr;
	
	for (int32_t i = 0; i < swapchain.commands.size(); ++i) {
		VK_CHECK_RESULT(vkBeginCommandBuffer(swapchain.commands[i], &cmdBufInfo));
		// Fill GBuffer
		{
			std::vector<VkClearValue> clearValues; clearValues.resize(4);
			clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
			clearValues[1].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
			clearValues[2].depthStencil = { 1.0f, 0 };
			clearValues[3].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };

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

			for ( auto graphic : graphics ) {
				graphic->createImageMemoryBarrier(swapchain.commands[i]);
			}

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

			vkCmdBeginRenderPass(swapchain.commands[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
				vkCmdSetViewport(swapchain.commands[i], 0, 1, &viewport);
				vkCmdSetScissor(swapchain.commands[i], 0, 1, &scissor);
				for ( auto pass : passes ) {
					ext::vulkan::currentPass = pass + ";DEFERRED";
					for ( auto graphic : graphics ) {
						// only draw graphics that are assigned to this type of render mode
						if ( graphic->renderMode->getName() != this->getName() ) continue;
						graphic->createCommandBuffer(swapchain.commands[i] );
					}
				}
			vkCmdNextSubpass(swapchain.commands[i], VK_SUBPASS_CONTENTS_INLINE);
				blitter.createCommandBuffer(swapchain.commands[i]);
				// render gui layer
				{
					RenderMode* layer = &ext::vulkan::getRenderMode("Gui");
					if ( layer->getName() == "Gui" ) {
						RenderTargetRenderMode* guiLayer = (RenderTargetRenderMode*) layer;
						guiLayer->blitter.createCommandBuffer(swapchain.commands[i]);
					}
				}
			vkCmdEndRenderPass(swapchain.commands[i]);
		}

		VK_CHECK_RESULT(vkEndCommandBuffer(swapchain.commands[i]));
	}
}