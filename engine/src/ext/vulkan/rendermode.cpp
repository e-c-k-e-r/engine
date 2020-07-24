#include <uf/ext/vulkan/vulkan.h>
#include <uf/ext/vulkan/rendermode.h>
#include <uf/ext/vulkan/initializers.h>
#include <uf/utils/window/window.h>

#include <uf/ext/vulkan/rendertarget.h>
#include <uf/utils/mesh/mesh.h>

/*
ext::vulkan::RenderMode::~RenderMode() {
	this->destroy();
}
*/
std::string ext::vulkan::RenderMode::getType() const {
	return "";
}
const std::string& ext::vulkan::RenderMode::getName() const {
	return this->name;
}
size_t ext::vulkan::RenderMode::subpasses() const {
	return 1;
}

void ext::vulkan::RenderMode::createCommandBuffers() {
	std::vector<ext::vulkan::Graphic*> graphics;
	std::function<void(uf::Entity*)> filter = [&]( uf::Entity* entity ) {
		if ( !entity->hasComponent<uf::Mesh>() ) return;
		uf::MeshBase& mesh = entity->getComponent<uf::Mesh>();
		if ( !mesh.generated ) return;
		ext::vulkan::Graphic& graphic = mesh.graphic;
		if ( !graphic.initialized ) return;
		if ( !graphic.renderMode ) return;
		graphics.push_back(&graphic);
	};
	for ( uf::Scene* scene : ext::vulkan::scenes ) {
		if ( !scene ) continue;
		scene->process(filter);
	}

	createCommandBuffers( graphics, ext::vulkan::passes );
}
void ext::vulkan::RenderMode::createCommandBuffers( const std::vector<ext::vulkan::Graphic*>& graphics, const std::vector<std::string>& passes ) {
}

void ext::vulkan::RenderMode::render() {
	// Get next image in the swap chain (back/front buffer)
	VK_CHECK_RESULT(swapchain.acquireNextImage(&currentBuffer, swapchain.presentCompleteSemaphore));

	// Use a fence to wait until the command buffer has finished execution before using it again
	VK_CHECK_RESULT(vkWaitForFences(*device, 1, &fences[currentBuffer], VK_TRUE, UINT64_MAX));
	VK_CHECK_RESULT(vkResetFences(*device, 1, &fences[currentBuffer]));

	// Pipeline stage at which the queue submission will wait (via pWaitSemaphores)
	VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	// The submit info structure specifices a command buffer queue submission batch
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pWaitDstStageMask = &waitStageMask;									// Pointer to the list of pipeline stages that the semaphore waits will occur at
	submitInfo.pWaitSemaphores = &swapchain.presentCompleteSemaphore;				// Semaphore(s) to wait upon before the submitted command buffer starts executing
	submitInfo.waitSemaphoreCount = 1;												// One wait semaphore																				
	submitInfo.pSignalSemaphores = &swapchain.renderCompleteSemaphore;				// Semaphore(s) to be signaled when command buffers have completed
	submitInfo.signalSemaphoreCount = 1;											// One signal semaphore
	submitInfo.pCommandBuffers = &commands[currentBuffer];		// Command buffers(s) to execute in this batch (submission)
	submitInfo.commandBufferCount = 1;

	// Submit to the graphics queue passing a wait fence
	VK_CHECK_RESULT(vkQueueSubmit(device->graphicsQueue, 1, &submitInfo, fences[currentBuffer]));
	
	// Present the current buffer to the swap chain
	// Pass the semaphore signaled by the command buffer submission from the submit info as the wait semaphore for swap chain presentation
	// This ensures that the image is not presented to the windowing system until all commands have been submitted
	VK_CHECK_RESULT(swapchain.queuePresent(device->presentQueue, currentBuffer, swapchain.renderCompleteSemaphore));
	VK_CHECK_RESULT(vkQueueWaitIdle(device->presentQueue));
}

void ext::vulkan::RenderMode::initialize( Device& device ) {
	this->device = &device;

	this->width = ext::vulkan::width;
	this->height = ext::vulkan::height;

	// Create command buffers
	{
		commands.resize( swapchain.buffers );

		VkCommandBufferAllocateInfo cmdBufAllocateInfo = ext::vulkan::initializers::commandBufferAllocateInfo(
			device.commandPool,
			VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			static_cast<uint32_t>(commands.size())
		);

		VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, commands.data()));
	}
	// Set sync objects
	{
		// Fences (Used to check draw command buffer completion)
		VkFenceCreateInfo fenceCreateInfo = {};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		// Create in signaled state so we don't wait on first render of each command buffer
		fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		fences.resize( commands.size() );
		for ( auto& fence : fences ) {
			VK_CHECK_RESULT(vkCreateFence(device, &fenceCreateInfo, nullptr, &fence));
		}

		VkPipelineStageFlags submitPipelineStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	}
}

void ext::vulkan::RenderMode::destroy() {
	renderTarget.destroy();
	
	if ( commands.size() > 0 ) {
		vkFreeCommandBuffers( *device, device->commandPool, static_cast<uint32_t>(commands.size()), commands.data());
	}

	for ( auto& fence : fences ) {
		vkDestroyFence( *device, fence, nullptr);
		fence = VK_NULL_HANDLE;
	}
}