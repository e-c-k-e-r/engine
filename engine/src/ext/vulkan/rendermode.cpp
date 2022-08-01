#if UF_USE_VULKAN

#include <uf/ext/vulkan/vulkan.h>
#include <uf/ext/vulkan/rendermode.h>
#include <uf/ext/vulkan/initializers.h>
#include <uf/ext/vulkan/graphic.h>
#include <uf/utils/window/window.h>

#include <uf/ext/vulkan/rendertarget.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/utils/serialize/serializer.h>
#include <uf/engine/scene/scene.h>

#include <uf/ext/openvr/openvr.h>

ext::vulkan::RenderMode::~RenderMode() {
	this->destroy();
}
const uf::stl::string ext::vulkan::RenderMode::getType() const {
	return "";
}
const uf::stl::string ext::vulkan::RenderMode::getName() const {
//	auto& metadata = *const_cast<uf::Serializer*>(&this->metadata);
//	return metadata["name"].as<uf::stl::string>();
	return metadata.name;
}
ext::vulkan::RenderTarget& ext::vulkan::RenderMode::getRenderTarget( size_t i ) {
	return renderTarget;
}
const ext::vulkan::RenderTarget& ext::vulkan::RenderMode::getRenderTarget( size_t i ) const {
	return renderTarget;
}

void ext::vulkan::RenderMode::bindCallback( int32_t subpass, const ext::vulkan::RenderMode::callback_t& callback ) {
	commandBufferCallbacks[subpass] = callback;
}

const size_t ext::vulkan::RenderMode::blitters() const {
	return 0;
}
ext::vulkan::Graphic* ext::vulkan::RenderMode::getBlitter( size_t i ) {
	return NULL;
}
uf::stl::vector<ext::vulkan::Graphic*> ext::vulkan::RenderMode::getBlitters() {
	return {};
}
bool ext::vulkan::RenderMode::hasAttachment( const uf::stl::string& name ) const {
	return metadata.attachments.count(name) > 0;
}
const ext::vulkan::RenderTarget::Attachment& ext::vulkan::RenderMode::getAttachment( const uf::stl::string& name ) const {
	UF_ASSERT_MSG( hasAttachment( name ), "attachment in `{}`: {} not found: {}", this->getName(), this->getType(), name );
	return renderTarget.attachments[metadata.attachments.at(name)];
}
size_t ext::vulkan::RenderMode::getAttachmentIndex( const uf::stl::string& name ) const {
	return hasAttachment( name ) ? metadata.attachments.at(name) : SIZE_MAX;
}

uf::Image ext::vulkan::RenderMode::screenshot( size_t attachmentID, size_t layerID ) {
	uf::Image image;
	if ( !device || renderTarget.attachments.size() < attachmentID ) return image;
	auto& attachment = renderTarget.attachments[attachmentID];
	
	bool blitting = true;
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(device->physicalDevice, attachment.descriptor.format, &formatProperties);
	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT)) blitting = false;
	vkGetPhysicalDeviceFormatProperties(device->physicalDevice, VK_FORMAT_R8G8B8A8_UNORM, &formatProperties);
	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT)) blitting = false;

	uint32_t width = this->width > 0 ? this->width : ext::vulkan::settings::width;
	uint32_t height = this->height > 0 ? this->height : ext::vulkan::settings::height;

	VkImage temporary;
	VkImageCreateInfo imageCreateInfo = {};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	imageCreateInfo.extent = { width, height, 1 };
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;
	imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VmaAllocation allocation;
	VmaAllocationInfo allocationInfo;
	VmaAllocationCreateInfo allocationCreateInfo = {};
	allocationCreateInfo.usage = VMA_MEMORY_USAGE_GPU_TO_CPU;
	VK_CHECK_RESULT(vmaCreateImage(allocator, &imageCreateInfo, &allocationCreateInfo, &temporary, &allocation, &allocationInfo));
	VkDeviceMemory temporaryMemory = allocationInfo.deviceMemory;

	auto commandBuffer = device->fetchCommandBuffer(QueueEnum::GRAPHICS, true); // waits on flush
	
	VkImageMemoryBarrier imageMemoryBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // ext::vulkan::device.queueFamilyIndices.graphics; //VK_QUEUE_FAMILY_IGNORED
	imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // ext::vulkan::device.queueFamilyIndices.graphics; //VK_QUEUE_FAMILY_IGNORED
	imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
	imageMemoryBarrier.subresourceRange.levelCount = 1;
	imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
	imageMemoryBarrier.subresourceRange.layerCount = 1;
	imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageMemoryBarrier.srcAccessMask = 0;
	imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	
	imageMemoryBarrier.image = temporary;
	imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, NULL, 0, NULL, 1, &imageMemoryBarrier );

	imageMemoryBarrier.image = attachment.image;
	imageMemoryBarrier.subresourceRange.baseArrayLayer = layerID;
	imageMemoryBarrier.oldLayout = attachment.descriptor.layout;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, NULL, 0, NULL, 1, &imageMemoryBarrier );

	if ( attachment.descriptor.samples > 1 ) {
		VkOffset3D blitSize;
		blitSize.x = renderTarget.width;
		blitSize.y = renderTarget.height;
		blitSize.z = 1;

		VkImageResolve imageResolveRegion{};
		imageResolveRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageResolveRegion.srcSubresource.baseArrayLayer = layerID;
		imageResolveRegion.srcSubresource.layerCount = 1;
	//	imageResolveRegion.srcOffsets[1] = blitSize;
		imageResolveRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageResolveRegion.dstSubresource.baseArrayLayer = 0;
		imageResolveRegion.dstSubresource.layerCount = 1;
	//	imageResolveRegion.dstOffsets[1] = blitSize;
		imageResolveRegion.extent = { renderTarget.width, renderTarget.height, 1 };

		vkCmdResolveImage(  commandBuffer, attachment.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, temporary, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageResolveRegion );
	} else if ( blitting ) {
		VkOffset3D blitSize;
		blitSize.x = renderTarget.width;
		blitSize.y = renderTarget.height;
		blitSize.z = 1;

		VkImageBlit imageBlit{};
		imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBlit.srcSubresource.baseArrayLayer = layerID;
		imageBlit.srcSubresource.layerCount = 1;
		imageBlit.srcOffsets[1] = blitSize;
		imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBlit.dstSubresource.baseArrayLayer = 0;
		imageBlit.dstSubresource.layerCount = 1;
		imageBlit.dstOffsets[1] = blitSize;

		vkCmdBlitImage( commandBuffer, attachment.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, temporary, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit, VK_FILTER_NEAREST);
	} else {
		VkImageCopy imageCopy{};
		imageCopy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopy.srcSubresource.baseArrayLayer = layerID;
		imageCopy.srcSubresource.layerCount = 1;
		imageCopy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopy.dstSubresource.baseArrayLayer = 0;
		imageCopy.dstSubresource.layerCount = 1;
		imageCopy.extent = { renderTarget.width, renderTarget.height, 1 };

		vkCmdCopyImage( commandBuffer, attachment.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, temporary, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopy );
	}
	imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

	imageMemoryBarrier.image = temporary;
	imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
	imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, NULL, 0, NULL, 1, &imageMemoryBarrier );

	imageMemoryBarrier.image = attachment.image;
	imageMemoryBarrier.subresourceRange.baseArrayLayer = layerID;
	imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	imageMemoryBarrier.newLayout = attachment.descriptor.layout;
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, NULL, 0, NULL, 1, &imageMemoryBarrier );
	device->flushCommandBuffer(commandBuffer);

	const uint8_t* data;
	vmaMapMemory( allocator, allocation, (void**)&data );
	image.loadFromBuffer( data, {renderTarget.width, renderTarget.height}, 8, 4, false );
	vmaUnmapMemory( allocator, allocation );
	vmaDestroyImage(allocator, temporary, allocation);
	return image;
}

ext::vulkan::GraphicDescriptor ext::vulkan::RenderMode::bindGraphicDescriptor( const ext::vulkan::GraphicDescriptor& reference, size_t pass ) {
	ext::vulkan::GraphicDescriptor descriptor = reference;
	descriptor.subpass = pass;
	descriptor.pipeline = metadata.pipeline;
	descriptor.bind.width = this->width ? this->width : settings::width;
	descriptor.bind.height = this->height ? this->height : settings::height;
	descriptor.bind.depth = metadata.eyes;
	descriptor.bind.point = VK_PIPELINE_BIND_POINT_GRAPHICS;

	descriptor.parse( metadata.json["descriptor"] );

//	descriptor.renderMode = this->getName();
	// invalidate
	if ( /*metadata.target != "" &&*/ descriptor.renderMode != this->getName() && descriptor.renderMode != metadata.target ) {
		descriptor.invalidated = true;
	} else {
		descriptor.renderMode = this->getName();
	}
	return descriptor;
}

void ext::vulkan::RenderMode::createCommandBuffers() {
	this->execute = true;

	uf::stl::vector<ext::vulkan::Graphic*> graphics;
	auto& scene = uf::scene::getCurrentScene(); 
	auto& graph = scene.getGraph();
	for ( auto entity : graph ) {
		if ( !entity->hasComponent<uf::Graphic>() ) continue;
		ext::vulkan::Graphic& graphic = entity->getComponent<uf::Graphic>();
		if ( !graphic.initialized || !graphic.process ) continue;
		graphics.emplace_back(&graphic);
	}

	this->synchronize();
//	bindPipelines( graphics );

	//lockMutex();
	createCommandBuffers( graphics );
	//unlockMutex();

	this->mostRecentCommandPoolId = std::this_thread::get_id();
	this->rebuild = false;
}
ext::vulkan::RenderMode::commands_container_t& ext::vulkan::RenderMode::getCommands( std::thread::id id ) {
	bool exists = this->commands.has(id); //this->commands.count(id) > 0;
	auto& commands = this->commands.get(id); //this->commands[id];
	if ( !exists ) {
		commands.resize( swapchain.buffers );

		VkCommandBufferAllocateInfo cmdBufAllocateInfo = ext::vulkan::initializers::commandBufferAllocateInfo(
			device->getCommandPool(this->getType() == "Compute" ? QueueEnum::COMPUTE : QueueEnum::GRAPHICS),
			VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			static_cast<uint32_t>(commands.size())
		);

		VK_CHECK_RESULT(vkAllocateCommandBuffers(*device, &cmdBufAllocateInfo, commands.data()));
	}
	return commands;
}
void ext::vulkan::RenderMode::lockMutex( std::thread::id id ) {
	this->commands.lockMutex( id );
}
bool ext::vulkan::RenderMode::tryMutex( std::thread::id id ) {
	return this->commands.tryMutex( id );
}
void ext::vulkan::RenderMode::unlockMutex( std::thread::id id ) {
	this->commands.unlockMutex( id );
}
std::lock_guard<std::mutex> ext::vulkan::RenderMode::guardMutex( std::thread::id id ) {
	return this->commands.guardMutex( id );
}
void ext::vulkan::RenderMode::cleanupAllCommands() {
	auto& container = this->commands.container();
	for ( auto& pair : container ) {
		if ( pair.second.empty() ) continue;
		vkFreeCommandBuffers( *device, device->getCommandPool(this->getType() == "Compute" ? QueueEnum::COMPUTE : QueueEnum::GRAPHICS, pair.first), static_cast<uint32_t>(pair.second.size()), pair.second.data());
		pair.second.clear();
	}
	container.clear();
}
void ext::vulkan::RenderMode::cleanupCommands( std::thread::id id ) {
	auto& container = this->commands.container();
	for ( auto& pair : container ) {
		if ( pair.first == id ) continue;
		if ( pair.second.empty() ) continue;
		vkFreeCommandBuffers( *device, device->getCommandPool(this->getType() == "Compute" ? QueueEnum::COMPUTE : QueueEnum::GRAPHICS, pair.first), static_cast<uint32_t>(pair.second.size()), pair.second.data());
		pair.second.clear();
	}
	this->commands.cleanup( id );
}
void ext::vulkan::RenderMode::createCommandBuffers( const uf::stl::vector<ext::vulkan::Graphic*>& graphics ) {

}
void ext::vulkan::RenderMode::bindPipelines() {
	this->execute = true;

	uf::stl::vector<ext::vulkan::Graphic*> graphics;
	auto& scene = uf::scene::getCurrentScene(); 
	auto& graph = scene.getGraph();
	for ( auto entity : graph ) {
		if ( !entity->hasComponent<uf::Graphic>() ) continue;
		ext::vulkan::Graphic& graphic = entity->getComponent<uf::Graphic>();
		if ( !graphic.initialized || !graphic.process ) continue;
	//	if ( graphic.descriptor.renderMode != "" && graphic.descriptor.renderMode != this->getName() ) continue;
		graphics.emplace_back(&graphic);
	}
	this->synchronize();
	this->bindPipelines( graphics );
}
void ext::vulkan::RenderMode::bindPipelines( const uf::stl::vector<ext::vulkan::Graphic*>& graphics ) {
	//lockMutex();
	for ( auto* pointer : graphics ) {
		auto& graphic = *pointer;
		for ( size_t currentPass = 0; currentPass < renderTarget.passes.size(); ++currentPass ) {
			auto& subpass = renderTarget.passes[currentPass];
			if ( !subpass.autoBuildPipeline ) continue;

			// bind to this render mode
			for ( auto& pipeline : metadata.pipelines ) {
				ext::vulkan::GraphicDescriptor descriptor = bindGraphicDescriptor(graphic.descriptor, currentPass);
				descriptor.pipeline = pipeline;
				// ignore invalidated descriptors
				if ( descriptor.invalidated ) continue;
				// ignore if pipeline exists for this render mode
				if ( graphic.hasPipeline( descriptor ) ) continue;
				// if pipeline name is specified for the rendermode, check if we have shaders for it
				size_t shaders = 0;
				for ( auto& shader : graphic.material.shaders ) if ( shader.metadata.pipeline == descriptor.pipeline ) ++shaders;
				if ( shaders == 0 ) continue;
				graphic.initializePipeline( descriptor );
			}
		}
	}
	//unlockMutex();
}
VkSubmitInfo ext::vulkan::RenderMode::queue() {
	if ( !metadata.limiter.execute ) return {};

	auto& commands = getCommands( this->mostRecentCommandPoolId );
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pWaitDstStageMask = NULL; 								// Pointer to the list of pipeline stages that the semaphore waits will occur at
	submitInfo.pWaitSemaphores = NULL;									// Semaphore(s) to wait upon before the submitted command buffer starts executing
	submitInfo.waitSemaphoreCount = 0;									// One wait semaphore																				
	submitInfo.pSignalSemaphores = NULL;								// Semaphore(s) to be signaled when command buffers have completed
	submitInfo.signalSemaphoreCount = 0;								// One signal semaphore
	submitInfo.pCommandBuffers = &commands[states::currentBuffer];		// Command buffers(s) to execute in this batch (submission)
	submitInfo.commandBufferCount = 1;

	return submitInfo;
}

void ext::vulkan::RenderMode::initialize( Device& device ) {
	this->device = &device;

	// this->width = 0; //ext::vulkan::width;
	// this->height = 0; //ext::vulkan::height;
	if ( this->scale == 0 ) this->scale = 1;
	
	{
		if ( this->width > 0 ) renderTarget.width = this->width;
		if ( this->height > 0 ) renderTarget.height = this->height;
		if ( this->scale > 0 ) renderTarget.scale = this->scale;
	}

	// Set sync objects
	{
		// Fences (Used to check draw command buffer completion)
		VkFenceCreateInfo fenceCreateInfo = {};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		// Create in signaled state so we don't wait on first render of each command buffer
		fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		fences.resize( swapchain.buffers );
		for ( auto& fence : fences ) {
			VK_CHECK_RESULT(vkCreateFence(device, &fenceCreateInfo, nullptr, &fence));
		}
		// Set sync objects
		{
			// Semaphores (Used for correct command ordering)
			VkSemaphoreCreateInfo semaphoreCreateInfo = {};
			semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			semaphoreCreateInfo.pNext = nullptr;
			// Semaphore used to ensures that all commands submitted have been finished before submitting the image to the queue
			VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &renderCompleteSemaphore));
		}
	}

	if ( std::find( metadata.pipelines.begin(), metadata.pipelines.end(), metadata.pipeline ) == metadata.pipelines.end() ) {
		metadata.pipelines.emplace_back(metadata.pipeline);
	}

}

void ext::vulkan::RenderMode::tick() {
	if ( ext::vulkan::states::resized || uf::renderer::states::rebuild || rebuild ) {
		cleanupAllCommands();
	}

	this->synchronize();
	
	if ( metadata.limiter.frequency > 0 ) {
		if ( metadata.limiter.timer > metadata.limiter.frequency ) {
			metadata.limiter.timer = 0;
			metadata.limiter.execute = true;
		} else {
			metadata.limiter.timer = metadata.limiter.timer + uf::time::delta;
			metadata.limiter.execute = false;
		}
	}
}

void ext::vulkan::RenderMode::render() {
	this->synchronize();
}

void ext::vulkan::RenderMode::destroy() {
	this->synchronize();

	renderTarget.destroy();

	for ( auto& pair : this->commands.container() ) {
		if ( !pair.second.empty() ) {
			vkFreeCommandBuffers( *device, device->getCommandPool(this->getType() == "Compute" ? QueueEnum::COMPUTE : QueueEnum::GRAPHICS, pair.first), static_cast<uint32_t>(pair.second.size()), pair.second.data());
		}
		pair.second.clear();
	}
	if ( renderCompleteSemaphore != VK_NULL_HANDLE ) {
		vkDestroySemaphore( *device, renderCompleteSemaphore, nullptr);
		renderCompleteSemaphore = VK_NULL_HANDLE;
	}

	for ( auto& fence : fences ) {
		vkDestroyFence( *device, fence, nullptr);
	}
	fences.clear();
}
void ext::vulkan::RenderMode::synchronize( uint64_t timeout ) {
/*
	if ( !device ) return;
	if ( fences.empty() ) return;
	lockMutex();
	VK_CHECK_RESULT(vkWaitForFences( *device, fences.size(), fences.data(), VK_TRUE, timeout ));
	unlockMutex();
*/
}
void ext::vulkan::RenderMode::pipelineBarrier( VkCommandBuffer commandBuffer, uint8_t state ) {
	VkImageMemoryBarrier imageMemoryBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // ext::vulkan::device.queueFamilyIndices.graphics; //VK_QUEUE_FAMILY_IGNORED
	imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // ext::vulkan::device.queueFamilyIndices.graphics; //VK_QUEUE_FAMILY_IGNORED
	imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
	imageMemoryBarrier.subresourceRange.levelCount = 1;
	imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
	imageMemoryBarrier.subresourceRange.layerCount = 1;
	imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

	for ( auto& attachment : renderTarget.attachments ) {
		if ( !(attachment.descriptor.usage & VK_IMAGE_USAGE_SAMPLED_BIT) ) continue;
		if (  (attachment.descriptor.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) ) continue;
		
		VkPipelineStageFlags srcStageMask, dstStageMask;
		imageMemoryBarrier.image = attachment.image;
		imageMemoryBarrier.oldLayout = attachment.descriptor.layout;
		imageMemoryBarrier.newLayout = attachment.descriptor.layout;
	
		switch ( state ) {
			case 0: {
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_SHADER_READ_BIT;
				imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			} break;
			case 1: {
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_SHADER_READ_BIT;
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				imageMemoryBarrier.newLayout = attachment.descriptor.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
				dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
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

		attachment.descriptor.layout = imageMemoryBarrier.newLayout;
	}
}

#endif