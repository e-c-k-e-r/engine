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
const std::string ext::vulkan::RenderMode::getType() const {
	return "";
}
const std::string ext::vulkan::RenderMode::getName() const {
	auto& metadata = *const_cast<uf::Serializer*>(&this->metadata);
	return metadata["name"].as<std::string>();
}
ext::vulkan::RenderTarget& ext::vulkan::RenderMode::getRenderTarget( size_t i ) {
	return renderTarget;
}
const ext::vulkan::RenderTarget& ext::vulkan::RenderMode::getRenderTarget( size_t i ) const {
	return renderTarget;
}

const size_t ext::vulkan::RenderMode::blitters() const {
	return 0;
}
ext::vulkan::Graphic* ext::vulkan::RenderMode::getBlitter( size_t i ) {
	return NULL;
}
std::vector<ext::vulkan::Graphic*> ext::vulkan::RenderMode::getBlitters() {
	return {};
}

ext::vulkan::GraphicDescriptor ext::vulkan::RenderMode::bindGraphicDescriptor( const ext::vulkan::GraphicDescriptor& reference, size_t pass ) {
	ext::vulkan::GraphicDescriptor descriptor = reference;
	descriptor.renderMode = this->getName();
	descriptor.subpass = pass;
	descriptor.parse( metadata );
	return descriptor;
}
void ext::vulkan::RenderMode::bindGraphicPushConstants( ext::vulkan::Graphic* pointer, size_t pass ) {
	auto& graphic = *pointer;
	// bind vertex shader push constants
	{
		auto& shader = graphic.material.getShader("vertex");
		auto& metadata = shader.metadata["definitions"]["pushConstants"]["PushConstant"];
		struct PushConstant {
			uint32_t pass = 0;
		};
		if ( ext::json::isObject( metadata ) ) {
			auto& pushConstant = shader.pushConstants.at( metadata["index"].as<size_t>() ).get<PushConstant>();
			pushConstant.pass = pass;
		}
	}
	// bind fragment shader push constants
	{
		auto& shader = graphic.material.getShader("fragment");
		auto& metadata = shader.metadata["definitions"]["pushConstants"]["PushConstant"];
	}
}

void ext::vulkan::RenderMode::createCommandBuffers() {
	this->execute = true;

	std::vector<ext::vulkan::Graphic*> graphics;
	std::function<void(uf::Entity*)> filter = [&]( uf::Entity* entity ) {
		if ( !entity->hasComponent<uf::Graphic>() ) return;
		ext::vulkan::Graphic& graphic = entity->getComponent<uf::Graphic>();
		if ( !graphic.initialized || !graphic.process ) return;
		graphics.push_back(&graphic);
	};
	for ( uf::Scene* scene : uf::scene::scenes ) {
		if ( !scene ) continue;
		scene->process(filter);
	}

	this->synchronize();
//	bindPipelines( graphics );
	createCommandBuffers( graphics );
	this->mostRecentCommandPoolId = std::this_thread::get_id();
	this->rebuild = false;
}
ext::vulkan::RenderMode::commands_container_t& ext::vulkan::RenderMode::getCommands() {
	return getCommands( std::this_thread::get_id() );
}
ext::vulkan::RenderMode::commands_container_t& ext::vulkan::RenderMode::getCommands( std::thread::id id ) {
	bool exists = this->commands.has(id); //this->commands.count(id) > 0;
	auto& commands = this->commands.get(id); //this->commands[id];
	if ( !exists ) {
		commands.resize( swapchain.buffers );

		VkCommandBufferAllocateInfo cmdBufAllocateInfo = ext::vulkan::initializers::commandBufferAllocateInfo(
			this->getType() == "Compute" ? device->getCommandPool(Device::QueueEnum::COMPUTE) : device->getCommandPool(Device::QueueEnum::GRAPHICS),
			VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			static_cast<uint32_t>(commands.size())
		);

		VK_CHECK_RESULT(vkAllocateCommandBuffers(*device, &cmdBufAllocateInfo, commands.data()));
	}
	return commands;
}
void ext::vulkan::RenderMode::createCommandBuffers( const std::vector<ext::vulkan::Graphic*>& graphics ) {

}
void ext::vulkan::RenderMode::bindPipelines() {
	this->execute = true;

	std::vector<ext::vulkan::Graphic*> graphics;
	std::function<void(uf::Entity*)> filter = [&]( uf::Entity* entity ) {
		if ( !entity->hasComponent<uf::Graphic>() ) return;
		ext::vulkan::Graphic& graphic = entity->getComponent<uf::Graphic>();
		if ( !graphic.initialized ) return;
		if ( !graphic.process ) return;
		graphics.push_back(&graphic);
	};
	for ( uf::Scene* scene : uf::scene::scenes ) {
		if ( !scene ) continue;
		scene->process(filter);
	}

	this->synchronize();
	this->bindPipelines( graphics );
}
void ext::vulkan::RenderMode::bindPipelines( const std::vector<ext::vulkan::Graphic*>& graphics ) {
	size_t subpasses = metadata["subpasses"].as<size_t>();
	for ( auto* pointer : graphics ) {
		auto& graphic = *pointer;
		for ( size_t currentPass = 0; currentPass < subpasses; ++currentPass ) {
			// bind to this render mode
			ext::vulkan::GraphicDescriptor descriptor = bindGraphicDescriptor(graphic.descriptor, currentPass);
			// ignore if pipeline exists for this render mode
			if ( graphic.hasPipeline( descriptor ) ) continue;
			graphic.initializePipeline( descriptor );
		}
	}
}

void ext::vulkan::RenderMode::render() {
/*
	if ( ext::openvr::context ) {
		ext::openvr::submit();
	}
*/
	auto& commands = getCommands( this->mostRecentCommandPoolId );
	// Get next image in the swap chain (back/front buffer)
	VK_CHECK_RESULT(swapchain.acquireNextImage(&states::currentBuffer, swapchain.presentCompleteSemaphore));

	// Use a fence to wait until the command buffer has finished execution before using it again
	VK_CHECK_RESULT(vkWaitForFences(*device, 1, &fences[states::currentBuffer], VK_TRUE, UINT64_MAX));
	VK_CHECK_RESULT(vkResetFences(*device, 1, &fences[states::currentBuffer]));

	// Pipeline stage at which the queue submission will wait (via pWaitSemaphores)
	VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	// The submit info structure specifices a command buffer queue submission batch
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pWaitDstStageMask = &waitStageMask;									// Pointer to the list of pipeline stages that the semaphore waits will occur at
	submitInfo.pWaitSemaphores = &swapchain.presentCompleteSemaphore;				// Semaphore(s) to wait upon before the submitted command buffer starts executing
	submitInfo.waitSemaphoreCount = 1;												// One wait semaphore																				
	submitInfo.pSignalSemaphores = &renderCompleteSemaphore;						// Semaphore(s) to be signaled when command buffers have completed
	submitInfo.signalSemaphoreCount = 1;											// One signal semaphore
	submitInfo.pCommandBuffers = &commands[states::currentBuffer];					// Command buffers(s) to execute in this batch (submission)
	submitInfo.commandBufferCount = 1;

	// Submit to the graphics queue passing a wait fence
	VK_CHECK_RESULT(vkQueueSubmit( device->getQueue( Device::QueueEnum::GRAPHICS ), 1, &submitInfo, fences[states::currentBuffer]));
	//vkQueueSubmit(device->queues.graphics, 1, &submitInfo, fences[states::currentBuffer]);
	
	// Present the current buffer to the swap chain
	// Pass the semaphore signaled by the command buffer submission from the submit info as the wait semaphore for swap chain presentation
	// This ensures that the image is not presented to the windowing system until all commands have been submitted
	VK_CHECK_RESULT(swapchain.queuePresent(device->getQueue( Device::QueueEnum::PRESENT ), states::currentBuffer, renderCompleteSemaphore));
	VK_CHECK_RESULT(vkQueueWaitIdle(device->getQueue( Device::QueueEnum::PRESENT )));
}

void ext::vulkan::RenderMode::initialize( Device& device ) {
	this->device = &device;

	// this->width = 0; //ext::vulkan::width;
	// this->height = 0; //ext::vulkan::height;
	{
		if ( this->width > 0 ) renderTarget.width = this->width;
		if ( this->height > 0 ) renderTarget.height = this->height;
	}

	// Create command buffers
/*
	{
		commands.resize( swapchain.buffers );

		VkCommandBufferAllocateInfo cmdBufAllocateInfo = ext::vulkan::initializers::commandBufferAllocateInfo(
			this->getType() == "Compute" ? device.getCommandPool(Device::QueueEnum::COMPUTE) : device.getCommandPool(Device::QueueEnum::GRAPHICS),
			VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			static_cast<uint32_t>(commands.size())
		);

		VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, commands.data()));
	}
*/
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
}

void ext::vulkan::RenderMode::tick() {
	this->synchronize();
}

void ext::vulkan::RenderMode::destroy() {
	this->synchronize();

	renderTarget.destroy();
/*	
	if ( commands.size() > 0 ) {
		vkFreeCommandBuffers( *device, this->getType() == "Compute" ? device->getCommandPool(Device::QueueEnum::COMPUTE) : device->getCommandPool(Device::QueueEnum::GRAPHICS), static_cast<uint32_t>(commands.size()), commands.data());
	}
	commands.clear();
*/
	for ( auto& pair : this->commands.container() ) {
		vkFreeCommandBuffers( *device, device->getCommandPool(this->getType() == "Compute" ? Device::QueueEnum::COMPUTE : Device::QueueEnum::GRAPHICS, pair.first), static_cast<uint32_t>(pair.second.size()), pair.second.data());
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
	if ( !device ) return;
	if ( fences.empty() ) return;
	VK_CHECK_RESULT(vkWaitForFences( *device, fences.size(), fences.data(), VK_TRUE, timeout ));
}
void ext::vulkan::RenderMode::pipelineBarrier( VkCommandBuffer command, uint8_t stage ) {
}