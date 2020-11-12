#include <uf/ext/vulkan/vulkan.h>
#include <uf/ext/vulkan/rendermodes/compute.h>
#include <uf/ext/vulkan/rendermodes/deferred.h>
#include <uf/ext/vulkan/rendermodes/rendertarget.h>
#include <uf/ext/vulkan/initializers.h>
#include <uf/ext/openvr/openvr.h>
#include <uf/utils/window/window.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/ext/vulkan/graphic.h>
#include <uf/engine/scene/scene.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/math/transform.h>

#include <uf/ext/vulkan/graphic.h>

std::string ext::vulkan::ComputeRenderMode::getType() const {
	return "Compute";
}

void ext::vulkan::ComputeRenderMode::initialize( Device& device ) {
	ext::vulkan::RenderMode::initialize( device );

//	this->width = 3840;
//	this->height = 2160;

	{
		auto width = this->width > 0 ? this->width : ext::vulkan::settings::width;
		auto height = this->height > 0 ? this->height : ext::vulkan::settings::height;

		compute.device = &device;
		compute.material.device = &device;
		compute.descriptor.renderMode = this->name;
		compute.material.initializeShaders({
			{"./data/shaders/raytracing.comp.spv", VK_SHADER_STAGE_COMPUTE_BIT},
		});

		for ( size_t i = 0; i < (ext::openvr::context ? 2 : 1); ++i ) {
			Texture2D& texture = compute.material.textures.emplace_back();
			texture.asRenderTarget( device, width, height );
		}

		{
			auto& scene = uf::scene::getCurrentScene();
			auto& metadata = scene.getComponent<uf::Serializer>();

			auto& shader = compute.material.shaders.front();

			struct SpecializationConstant {
				uint32_t maxLights = 16;
				uint32_t eyes = 2;
			};
			auto* specializationConstants = (SpecializationConstant*) &shader.specializationConstants[0];
			specializationConstants->maxLights = metadata["system"]["config"]["engine"]["scenes"]["max lights"].as<size_t>();
			specializationConstants->eyes = ext::openvr::context ? 2 : 1;
			for ( auto& binding : shader.descriptorSetLayoutBindings ) {
				if ( binding.descriptorCount > 1 ) {
					binding.descriptorCount = specializationConstants->eyes;
				}
			}
		}

		// update buffers
		if ( !compute.buffers.empty() ) {
		//	compute.getPipeline().update( compute );
			compute.updatePipelines();
		}
	}
	{
		uf::BaseMesh<pod::Vertex_2F2F, uint32_t> mesh;
		mesh.vertices = {
			{ {-1.0f, 1.0f}, {0.0f, 1.0f}, },
			{ {-1.0f, -1.0f}, {0.0f, 0.0f}, },
			{ {1.0f, -1.0f}, {1.0f, 0.0f}, },
			{ {1.0f, 1.0f}, {1.0f, 1.0f}, }
		};
		mesh.indices = {
			0, 1, 2, 2, 3, 0
		};
		blitter.device = &device;
		blitter.material.device = &device;
		blitter.descriptor.subpass = 1;

		blitter.descriptor.depthTest.test = false;
		blitter.descriptor.depthTest.write = false;

		blitter.initializeGeometry( mesh );
		blitter.material.initializeShaders({
			{"./data/shaders/display.blit.vert.spv", VK_SHADER_STAGE_VERTEX_BIT},
			{"./data/shaders/display.blit.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT}
		});
		for ( auto& computeTexture : compute.material.textures ) {
			Texture2D& texture = blitter.material.textures.emplace_back();
			texture = computeTexture;
			texture.device = NULL;
		}
		blitter.initializePipeline();
	}
}
void ext::vulkan::ComputeRenderMode::render() {
	if ( compute.buffers.empty() ) return;

	auto& commands = getCommands( this->mostRecentCommandPoolId );
	// Submit compute commands
	// Use a fence to ensure that compute command buffer has finished executing before using it again
	VK_CHECK_RESULT(vkWaitForFences( *device, 1, &fences[states::currentBuffer], VK_TRUE, UINT64_MAX ));
	VK_CHECK_RESULT(vkResetFences( *device, 1, &fences[states::currentBuffer] ));

	VkSubmitInfo submitInfo = ext::vulkan::initializers::submitInfo();
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commands[states::currentBuffer];

	VK_CHECK_RESULT(vkQueueSubmit(device->getQueue( Device::QueueEnum::COMPUTE ), 1, &submitInfo, fences[states::currentBuffer]));
	//vkQueueSubmit(device->queues.compute, 1, &submitInfo, fences[states::currentBuffer]);
}
void ext::vulkan::ComputeRenderMode::tick() {
	ext::vulkan::RenderMode::tick();

	if ( ext::vulkan::states::resized ) {
		// auto-resize texture
		if ( this->width == 0 && this->height == 0 ) {
			auto width = this->width > 0 ? this->width : ext::vulkan::settings::width;
			auto height = this->height > 0 ? this->height : ext::vulkan::settings::height;
			for ( auto& texture : compute.material.textures ) {
				texture.destroy();
			}
			compute.material.textures.clear();
			for ( size_t i = 0; i < (ext::openvr::context ? 2 : 1); ++i ) {
				Texture2D& texture = compute.material.textures.emplace_back();
				texture.asRenderTarget( *device, width, height );
			}
		}
		// update blitter descriptor set
		if ( blitter.initialized ) {
			blitter.material.textures.clear();
			for ( auto& computeTexture : compute.material.textures ) {
				Texture2D& texture = blitter.material.textures.emplace_back();
				texture = computeTexture;
				texture.device = NULL;
			}
			blitter.getPipeline().update( blitter );
		//	blitter.updatePipelines();
		}
		if ( !compute.buffers.empty() ) {
			compute.getPipeline().update( compute );
		//	compute.updatePipelines();
		}
	}
}
void ext::vulkan::ComputeRenderMode::destroy() {
	ext::vulkan::RenderMode::destroy();
	blitter.destroy();
	compute.destroy();
}
void ext::vulkan::ComputeRenderMode::pipelineBarrier( VkCommandBuffer commandBuffer, uint8_t state ) {
	if ( compute.buffers.empty() ) return;
	
	VkImageMemoryBarrier imageMemoryBarrier = {};
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

	for ( auto& texture : compute.material.textures ) {
		VkPipelineStageFlags srcStageMask, dstStageMask;
		imageMemoryBarrier.image = texture.image;
		imageMemoryBarrier.oldLayout = texture.descriptor.imageLayout;
		imageMemoryBarrier.newLayout = texture.descriptor.imageLayout;
	
		switch ( state ) {
			case 0: {
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_SHADER_READ_BIT;
				imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
				srcStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
				dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			} break;
			case 1: {
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_SHADER_READ_BIT;
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
				imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
				srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
				dstStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			} break;
			// ensure 
			default: {
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
				dstStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			}
		}
		
		vkCmdPipelineBarrier( commandBuffer,
			srcStageMask, dstStageMask,
			VK_FLAGS_NONE,
			0, NULL,
			0, NULL,
			1, &imageMemoryBarrier
		);
	
		texture.descriptor.imageLayout = imageMemoryBarrier.newLayout;
	}
}
void ext::vulkan::ComputeRenderMode::createCommandBuffers( ) {
	this->synchronize();

	float width = this->width > 0 ? this->width : ext::vulkan::settings::width;
	float height = this->height > 0 ? this->height : ext::vulkan::settings::height;

	VkCommandBufferBeginInfo cmdBufInfo = ext::vulkan::initializers::commandBufferBeginInfo();
	auto& pipeline = compute.getPipeline();
	auto& commands = getCommands();
	if ( compute.buffers.empty() ) return;
	for (size_t i = 0; i < commands.size(); ++i) {
		VK_CHECK_RESULT(vkBeginCommandBuffer(commands[i], &cmdBufInfo));
			pipeline.record(compute, commands[i]);
			vkCmdDispatch(commands[i], width / dispatchSize.x, height / dispatchSize.y, 1);
		VK_CHECK_RESULT(vkEndCommandBuffer(commands[i]));
	}
	
	this->mostRecentCommandPoolId = std::this_thread::get_id();
}
void ext::vulkan::ComputeRenderMode::bindPipelines() {
	
}