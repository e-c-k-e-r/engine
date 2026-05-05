#if UF_USE_IMGUI
#include <uf/ext/imgui/imgui.h>
#include <uf/utils/math/vector.h>
#include <imgui/imgui.h>
#include <imgui/imgui_stdlib.h>
#include <uf/spec/renderer/universal.h>
#include <uf/utils/math/physics.h>
#include <uf/utils/window/payloads.h>

#if UF_USE_VULKAN
	#include <imgui/backends/imgui_impl_vulkan.h>
#endif
#if UF_USE_OPENGL
	#include <imgui/backends/imgui_impl_opengl2.h>
#endif
#if UF_ENV_WINDOWS
	#include <imgui/backends/imgui_impl_win32.h>
#endif
#if UF_ENV_LINUX
	// to-do: linux
#endif

bool ext::imgui::focused = false;

namespace {
#if UF_USE_VULKAN
	uf::renderer::RenderMode* boundRenderMode = NULL;
	VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
#endif
	bool initialized = false;

	void initPlatform() {
#if UF_ENV_WINDOWS
		ImGui_ImplWin32_Init(uf::renderer::device.window->getHandle());
#endif
#if UF_ENV_LINUX
		// to-do: linux
#endif
	}

	void initRenderer() {
#if UF_USE_VULKAN
		auto& renderMode = uf::renderer::getRenderMode("Gui", true);
		::boundRenderMode = &renderMode;

		// create descriptor pool
		VkDescriptorPoolSize poolSizes[] = {
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
		};
		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		poolInfo.maxSets = 1000 * IM_ARRAYSIZE(poolSizes);
		poolInfo.poolSizeCount = (uint32_t)IM_ARRAYSIZE(poolSizes);
		poolInfo.pPoolSizes = poolSizes;
		VK_CHECK_RESULT(vkCreateDescriptorPool(uf::renderer::device, &poolInfo, NULL, &descriptorPool));

		// initialize ImGui vulkan backend
		ImGui_ImplVulkan_InitInfo imguiInitInfo = {};
		imguiInitInfo.Instance = uf::renderer::device.instance;
		imguiInitInfo.PhysicalDevice = uf::renderer::device.physicalDevice;
		imguiInitInfo.Device = uf::renderer::device.logicalDevice;
		imguiInitInfo.QueueFamily = uf::renderer::device.queueFamilyIndices.graphics;
		imguiInitInfo.Queue = uf::renderer::device.getQueue(uf::renderer::QueueEnum::GRAPHICS);
		imguiInitInfo.PipelineCache = uf::renderer::device.pipelineCache;
		imguiInitInfo.DescriptorPool = descriptorPool;
		imguiInitInfo.Subpass = 0;
		imguiInitInfo.MinImageCount = uf::renderer::swapchain.buffers;
		imguiInitInfo.ImageCount = uf::renderer::swapchain.buffers;
		imguiInitInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		imguiInitInfo.Allocator = nullptr;
		imguiInitInfo.CheckVkResultFn = nullptr;
		ImGui_ImplVulkan_Init(&imguiInitInfo, renderMode.renderTarget.renderPass);

		// upload fonts
		auto commandBuffer = uf::renderer::device.fetchCommandBuffer(uf::renderer::QueueEnum::GRAPHICS);
		ImGui_ImplVulkan_CreateFontsTexture(commandBuffer.handle);
		uf::renderer::device.flushCommandBuffer(commandBuffer);
		ImGui_ImplVulkan_DestroyFontUploadObjects();

		// bind render callback
		renderMode.bindCallback(0, [&]( VkCommandBuffer cb, size_t _ ) {
			ImDrawData* drawData = ImGui::GetDrawData();
			if (drawData) ImGui_ImplVulkan_RenderDrawData(drawData, cb);
		});

#elif UF_USE_OPENGL
		ImGui_ImplOpenGL2_Init();
#endif
	}

	void newFramePlatform() {
#if UF_ENV_WINDOWS
		ImGui_ImplWin32_NewFrame();
#endif
#if UF_ENV_LINUX
		// to-do: linux
#endif
	}

	void newFrameRenderer() {
#if UF_USE_VULKAN
		auto& renderMode = uf::renderer::getRenderMode("Gui", true);
		renderMode.rerecord = true;
		ImGui_ImplVulkan_NewFrame();
#elif UF_USE_OPENGL
		ImGui_ImplOpenGL2_NewFrame();
#endif
	}

	void shutdownPlatform() {
#if UF_ENV_WINDOWS
		ImGui_ImplWin32_Shutdown();
#endif
#if UF_ENV_LINUX
		// to-do: linux
#endif
	}

	void shutdownRenderer() {
#if UF_USE_VULKAN
		ImGui_ImplVulkan_Shutdown();
		if ( descriptorPool != VK_NULL_HANDLE ) {
			vkDestroyDescriptorPool(uf::renderer::device, descriptorPool, nullptr);
			descriptorPool = VK_NULL_HANDLE;
		}
#elif UF_USE_OPENGL
		ImGui_ImplOpenGL2_Shutdown();
#endif
	}
}

void ext::imgui::initialize() {
#if UF_USE_VULKAN
	if ( !uf::renderer::hasRenderMode("Gui", true) ) return;
#endif

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
	io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
	io.DisplaySize = ImVec2((float) uf::renderer::settings::width, (float) uf::renderer::settings::height);
	io.MouseDrawCursor = false;
	io.IniFilename = nullptr;

	uf::hooks.addHook("window:Mouse.CursorVisibility", [&](pod::payloads::windowMouseCursorVisibility& payload) {
		ImGui::GetIO().MouseDrawCursor = payload.mouse.visible;
	});

	::initPlatform();
	::initRenderer();

	::initialized = true;
}

void ext::imgui::tick() {
	if ( !::initialized ) ext::imgui::initialize();
	if ( !::initialized ) return;
	// no GUI render mode found
	if ( !uf::renderer::hasRenderMode("Gui", true) ) return;
	auto& renderMode = uf::renderer::getRenderMode("Gui", true);

	// check if rendermode changed
	if ( ::boundRenderMode != &renderMode ) {
		::shutdownRenderer();
		::initRenderer();
	}

	ImGuiIO& io = ImGui::GetIO();
	io.DeltaTime = uf::physics::time::delta;
	io.DisplaySize = ImVec2((float)uf::renderer::settings::width, (float)uf::renderer::settings::height);

	ext::imgui::focused = io.WantCaptureKeyboard || io.WantCaptureMouse;

	::newFrameRenderer();
	::newFramePlatform();
	ImGui::NewFrame();

	uf::hooks.call("gui:IMGUI.tick");

	ImGui::Render();
}

void ext::imgui::render() {
	if (!::initialized) return;

#if UF_USE_OPENGL
	auto renderMode = uf::renderer::getCurrentRenderMode();
	if ( !renderMode || renderMode->getName() != "Gui" ) return;

	ImDrawData* data = ImGui::GetDrawData();
	if (data) ImGui_ImplOpenGL2_RenderDrawData(data);
#endif
}

void ext::imgui::terminate() {
	if (!::initialized) return;

	::shutdownRenderer();
	::shutdownPlatform();
	ImGui::DestroyContext();

	::initialized = false;
}
#endif
