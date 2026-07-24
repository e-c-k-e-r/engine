#if UF_USE_VULKAN

#include <uf/ext/vulkan/vulkan.h>
#include <uf/ext/vulkan/rendermodes/vr.h>
#include <uf/ext/vulkan/initializers.h>
#include <uf/utils/window/window.h>
#include <uf/utils/math/physics.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/ext/vulkan/graphic.h>
#include <uf/engine/graph/graph.h>
#include <uf/utils/camera/camera.h>
#include <uf/engine/ext.h>
#include <uf/utils/io/fmt.h>
#include <uf/ext/openvr/openvr.h>

const uf::stl::string ext::vulkan::VrRenderMode::getType() const {
	return "VR";
}

void ext::vulkan::VrRenderMode::initialize(Device& device) {
	uint32_t width = this->width > 0 ? this->width : (ext::vulkan::settings::width * this->scale);
	uint32_t height = this->height > 0 ? this->height : (ext::vulkan::settings::height * this->scale);

	metadata.pipeline = "vr";

	ext::vulkan::RenderMode::initialize( device );
	renderTarget.device = &device;

	struct {
		size_t left, right, depth;
	} attachments = {};

	attachments.left = renderTarget.attach(RenderTarget::Attachment::Descriptor{
		/*.format =*/ ext::vulkan::settings::pipelines::hdr ? enums::Format::HDR : enums::Format::SDR,
		/*.layout = */ VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		/*.usage =*/ VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		/*.blend =*/ true,
		/*.samples =*/ 1,
	});
	attachments.right = renderTarget.attach(RenderTarget::Attachment::Descriptor{
		/*.format =*/ ext::vulkan::settings::pipelines::hdr ? enums::Format::HDR : enums::Format::SDR,
		/*.layout = */ VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		/*.usage =*/ VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		/*.blend =*/ true,
		/*.samples =*/ 1,
	});
	attachments.depth = renderTarget.attach(RenderTarget::Attachment::Descriptor{
		/*.format = */ext::vulkan::settings::formats::depth,
		/*.layout = */VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		/*.usage = */VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
		/*.blend = */false,
		/*.samples = */1,
	});

	metadata.attachments["left"] = attachments.left;
	metadata.attachments["right"] = attachments.right;
	metadata.attachments["output"] = attachments.left;

	renderTarget.addPass(
		/*.*/ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		/*.colors =*/ { attachments.left, attachments.right },
		/*.inputs =*/ {},
		/*.resolve =*/{},
		/*.depth = */ attachments.depth,
		/*.layer = */0,
		/*.autoBuildPipeline =*/ true
	);

	renderTarget.initialize( device );

	blitter.process = false;
}

void ext::vulkan::VrRenderMode::createCommandBuffers(const uf::stl::vector<ext::vulkan::Graphic*>& graphics) {
	uint32_t width = this->width > 0 ? this->width : (ext::vulkan::settings::width * this->scale);
	uint32_t height = this->height > 0 ? this->height : (ext::vulkan::settings::height * this->scale);

	VkCommandBufferBeginInfo cmdBufInfo = ext::vulkan::initializers::commandBufferBeginInfo();
	cmdBufInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	VkImageMemoryBarrier imageMemoryBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // ext::vulkan::device.queueFamilyIndices.graphics; //VK_QUEUE_FAMILY_IGNORED
	imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // ext::vulkan::device.queueFamilyIndices.graphics; //VK_QUEUE_FAMILY_IGNORED
	imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
	imageMemoryBarrier.subresourceRange.levelCount = 1;
	imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
	imageMemoryBarrier.subresourceRange.layerCount = 1;
	imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

	uf::stl::vector<RenderMode*> layers = ext::vulkan::getRenderModes(uf::stl::vector<uf::stl::string>{"Deferred", "Compute:RT", "RenderTarget"}, false);
	
	float depthClear = uf::matrix::reverseInfiniteProjection ? 0.0f : 1.0f;
	uf::stl::vector<VkClearValue> clearValues;
	for ( auto& attachment : renderTarget.attachments ) {
		pod::Vector4f clearColor = {0, 0, 0, 0};
		auto& clearValue = clearValues.emplace_back();
		if ( attachment.descriptor.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT ) {
			clearValue.color.float32[0] = clearColor[0];
			clearValue.color.float32[1] = clearColor[1];
			clearValue.color.float32[2] = clearColor[2];
			clearValue.color.float32[3] = clearColor[3];
		} else if ( attachment.descriptor.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT ) {
			clearValue.depthStencil = { depthClear, 0 };
		}
	}

	auto& commands = getCommands();
	for (size_t frame = 0; frame < commands.size(); ++frame) {
		auto& commandBuffer = commands[frame];
		VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffer, &cmdBufInfo));
		device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::BEGIN, "begin" );
		{

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
			renderPassBeginInfo.framebuffer = renderTarget.framebuffers[frame];

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

			size_t subpasses = renderTarget.passes.size();
			size_t currentPass = 0;

			// pre-renderpass commands
			VK_COMMAND_BUFFER_CALLBACK( CALLBACK_BEGIN, commandBuffer, frame, {
				device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::GENERIC, "callback[begin]" );
			} );

			{
				device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::BEGIN, "renderPass[begin]" );
				vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
					vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
					vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
					for ( auto currentPass = 0; currentPass < 2; ++currentPass ) {
						size_t currentDraw = 0;
						for ( auto _ : layers ) {
							RenderTargetRenderMode* layer = (RenderTargetRenderMode*) _;
							auto& blitter = layer->blitter;
							if ( !blitter.initialized || !blitter.process ) continue;
							ext::vulkan::GraphicDescriptor descriptor = blitter.descriptor;
							descriptor.pipeline = "vr";
							descriptor.renderMode = this->getName();
							descriptor.bind.width = width;
							descriptor.bind.height = height;
							descriptor.bind.depth = 1;
							descriptor.bind.point = VK_PIPELINE_BIND_POINT_GRAPHICS;
							descriptor.subpass = 0;
							descriptor.depth.test = false;
							descriptor.inputs.vertex.count = 6;
							descriptor.cullMode = uf::renderer::enums::CullMode::NONE;

							// to-do: transition attachment here

							device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::GENERIC, FMT_FORMAT("blitter[{}: {}]", layer->getName(), layer->getType()) );
							blitter.record(commandBuffer, descriptor, currentPass, currentDraw++, frame);
						}
					}
				device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::END, "renderPass[end]" );
				vkCmdEndRenderPass(commandBuffer);
			}

			
			// post-renderpass commands
			VK_COMMAND_BUFFER_CALLBACK( CALLBACK_END, commandBuffer, frame, {
				device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::GENERIC, "callback[end]" );
			} );

			// transition attachments
			{
				auto& leftEyeAttachment = this->getAttachment("left");
				auto& rightEyeAttachment = this->getAttachment("right");

				VkImageSubresourceRange range = {};
				range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				range.baseMipLevel = 0;
				range.levelCount = 1;
				range.baseArrayLayer = 0;
				range.layerCount = 1;

				uf::renderer::Texture::setImageLayout( commandBuffer, leftEyeAttachment.image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, range );
				uf::renderer::Texture::setImageLayout( commandBuffer, rightEyeAttachment.image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, range );
			}
		}
		device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::END, "end" );
		VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));
	}
}

void ext::vulkan::VrRenderMode::build( bool resized ) {
	ext::vulkan::RenderMode::build();

	uint32_t width = this->width > 0 ? this->width : (ext::vulkan::settings::width * this->scale);
	uint32_t height = this->height > 0 ? this->height : (ext::vulkan::settings::height * this->scale);

	// (re)initialize pipelines
	if ( blitter.process ) {
		blitter.descriptor.bind.width = width;
		blitter.descriptor.bind.height = height;

		blitter.update( blitter.descriptor );
	}
}
void ext::vulkan::VrRenderMode::tick() {
	ext::vulkan::RenderMode::tick();

	bool resized = this->width == 0 && this->height == 0 && (ext::vulkan::states::resized || this->resized);
	bool rebuild = resized || ext::vulkan::states::rebuild || this->rebuild;

	uint32_t width = this->width > 0 ? this->width : (ext::vulkan::settings::width * this->scale);
	uint32_t height = this->height > 0 ? this->height : (ext::vulkan::settings::height * this->scale);

	if ( resized ) {
		this->resized = false;
		renderTarget.initialize( *renderTarget.device );
	}
	if ( rebuild && blitter.process ) {
		this->build( resized );
	}
}

void ext::vulkan::VrRenderMode::destroy() {
	ext::vulkan::RenderMode::destroy();
}

void ext::vulkan::VrRenderMode::render() {
	if ( this->commands.container().empty() ) return;

	auto& commands = getCommands( this->mostRecentCommandPoolId );

	VK_COMMAND_BUFFER_CALLBACK( EXECUTE_BEGIN, VkCommandBuffer{}, 0, {} );

	VkSubmitInfo submitInfo = this->queue();

	VkQueue queue = device->getQueue( QueueEnum::GRAPHICS );
	VkResult res = vkQueueSubmit( queue, 1, &submitInfo, /*VK_NULL_HANDLE*/fences[states::currentBuffer]);
	VK_CHECK_QUEUE_CHECKPOINT( queue, res );
	VK_COMMAND_BUFFER_CALLBACK( EXECUTE_END, VkCommandBuffer{}, 0, {} );

	ext::openvr::submit();

	this->executed = true;
}

#endif