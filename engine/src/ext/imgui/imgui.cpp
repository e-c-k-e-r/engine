#if UF_USE_IMGUI
#include <uf/ext/imgui/imgui.h>
#include <uf/utils/math/vector.h>
#include <imgui/imgui.h>
#include <uf/spec/renderer/universal.h>
#include <uf/utils/math/physics.h>

#if UF_USE_VULKAN
	#include <imgui/backends/imgui_impl_vulkan.h>
#endif
#if UF_USE_OPENGL
	#include <imgui/backends/imgui_impl_opengl2.h>
#endif
#if UF_ENV_WINDOWS
	#include <imgui/backends/imgui_impl_win32.h>
#endif

#include <imgui/imgui_stdlib.h>

#include <uf/utils/io/fmt.h>
#include <uf/utils/io/payloads.h>
#include <uf/utils/io/console.h>
#include <uf/utils/window/payloads.h>

bool ext::imgui::focused = false;

namespace {
#if UF_USE_VULKAN
	VkDescriptorPool descriptorPool{};
#endif

	struct ConsoleWindow {
		ImGuiTextFilter filter;			
		int history = 0;

		struct {
			bool automatic = true;
			bool bottom = false;
		} scroll;

		uf::stl::unordered_map<uf::stl::string, std::function<uf::stl::string(const uf::stl::string&)>> commands;

		pod::Vector2ui size{800, 600};
		pod::Vector2ui position{64, 32};

		void Draw( const uf::stl::string& title, bool& open ) {
			ImGui::SetNextWindowSize(ImVec2(size.x, size.y), ImGuiCond_FirstUseEver);
			ImGui::SetNextWindowPos(ImVec2(position.x, position.y), ImGuiCond_FirstUseEver);

			if (!ImGui::Begin(title.c_str(), &open)) {
				ImGui::End();
				return;
			}

			if ( ImGui::BeginPopupContextItem() ) {
				if ( ImGui::MenuItem("Close Console") ) open = false;
				ImGui::EndPopup();
			}

			if ( ImGui::SmallButton("Clear") ) uf::console::clear();

			ImGui::SameLine();
			bool copyToClipboard = ImGui::SmallButton("Copy");
			ImGui::Separator();

			if ( ImGui::BeginPopup("Options") ) {
				ImGui::Checkbox("Auto-scroll", &this->scroll.bottom);
				ImGui::EndPopup();
			}

			if ( ImGui::Button("Options") ) {
				ImGui::OpenPopup("Options");
			}

			ImGui::SameLine();
			this->filter.Draw("Filter (\"incl,-excl\") (\"error\")", 180);
			ImGui::Separator();

			const float footerReserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
			ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footerReserve), false, ImGuiWindowFlags_HorizontalScrollbar);
			if ( ImGui::BeginPopupContextWindow() ) {
				if ( ImGui::Selectable("Clear") ) uf::console::clear();
				ImGui::EndPopup();
			}

			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1));

			if ( copyToClipboard ) ImGui::LogToClipboard();
			for ( auto& item : uf::console::log ) {
				if ( !this->filter.PassFilter(item.c_str()) ) continue;

				ImVec4 color = ImVec4(0, 0, 0, 0);

				if ( uf::string::matched(item, "^\\[ERROR\\]") ) color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
				else if ( uf::string::matched(item, "^\\> ") ) color = ImVec4(1.0f, 0.8f, 0.6f, 1.0f);

				if ( color.w > 0.0f ) ImGui::PushStyleColor(ImGuiCol_Text, color);
				ImGui::TextUnformatted( item.c_str() );
				if ( color.w > 0.0f ) ImGui::PopStyleColor();
			}
			if ( copyToClipboard ) ImGui::LogFinish();

			if ( this->scroll.bottom || (this->scroll.automatic && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) ) {
				ImGui::SetScrollHereY(1.0f);
			}
			this->scroll.bottom = false;

			ImGui::PopStyleVar();
			ImGui::EndChild();
			ImGui::Separator();

			bool reclaimFocus = false;
			ImGuiInputTextFlags inputTextFlags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory;

			uf::stl::string command;
			if ( ImGui::InputText("Input", &command, inputTextFlags, &TextEditCallbackStub, (void *)this) ) {
				this->history = -1;
				this->scroll.bottom = true;
				reclaimFocus = true;


				// to-do: add a way to either asynchronously invoke commands or not
				uf::console::execute( command );
			/*
				uf::thread::queue( uf::thread::asyncThreadName, [=](){
					uf::console::execute( command );
				});
			*/
			/*
				// this blocks
				uf::thread::queue( uf::thread::fetchWorker(), [=](){
					uf::console::execute( command );
				});
			*/
			/*
				// this still blocks
				auto tasks = uf::thread::schedule(true);
				tasks.queue([=](){
					uf::console::execute( command );
				});
				uf::thread::execute( tasks );
			*/

			}

			ImGui::SetItemDefaultFocus();
			if ( reclaimFocus ) ImGui::SetKeyboardFocusHere(-1);
			ImGui::End();
		}

		static int TextEditCallbackStub( ImGuiInputTextCallbackData *data ) {
			ConsoleWindow *console = (ConsoleWindow*) data->UserData;
			return console->TextEditCallback(data);
		}

		int TextEditCallback( ImGuiInputTextCallbackData* data ) {
			switch ( data->EventFlag ) {
				case ImGuiInputTextFlags_CallbackCompletion: {
					const char* end = data->Buf + data->CursorPos;
					const char* start = end;
					while ( start > data->Buf ) {
						const char c = start[-1];
						if (c == ' ' || c == '\t' || c == ',' || c == ';')
							break;
						start--;
					}

					uf::stl::string input{ start, end };
					uf::stl::vector<uf::stl::string> candidates;
					for ( auto& pair : uf::console::commands ) {
						if ( pair.first.find(input) == 0 ) candidates.emplace_back( input );
					}
					if ( candidates.empty() ) {
						UF_MSG_ERROR("No match for `{}`", input);
					} else if ( candidates.size() == 1 ) {
						data->DeleteChars((int)(start - data->Buf), (int)(end - start));
						data->InsertChars(data->CursorPos, candidates[0].c_str());
						data->InsertChars(data->CursorPos, " ");
					} else {
					/*
						int match = (int) (end - start);
						while ( true ) {
							int c = 0;
							bool allCandidatesMatches = true;
							for ( auto i = 0; i < candidates.size() && allCandidatesMatches; ++i ) {
								if ( i == 0 ) c = uf::string::uppercase( candidates[i].at(match) );
							}
						}
					*/
					}
				} break;
				case ImGuiInputTextFlags_CallbackHistory: {
					const int previousPosition = this->history;
					if ( data->EventKey == ImGuiKey_UpArrow ) {
						if (this->history == -1) this->history = uf::console::history.size() - 1;
						else if (this->history > 0) this->history--;
					} else if ( data->EventKey == ImGuiKey_DownArrow ) {
						if ( this->history != -1 && ++this->history >= uf::console::history.size() ) this->history = -1;
					}

					if ( previousPosition != this->history ) {
						const char* history = (this->history >= 0) ? uf::console::history[this->history].c_str() : "";
						data->DeleteChars(0, data->BufTextLen);
						data->InsertChars(0, history);
					}
				} break;
			}
			return 0;
		}
	};

	ConsoleWindow console;
	bool initialized = false;
}
void ext::imgui::initialize() {
#if UF_USE_VULKAN
	if ( !uf::renderer::hasRenderMode("Gui", true) ) return;
#endif

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;	// Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;	// Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;	//
//	io.ConfigFlags |= ImGuiConfigFlags_NoMouse;				// 
	io.DisplaySize = ImVec2(uf::renderer::settings::width,uf::renderer::settings::height);
	io.MouseDrawCursor = false;
	io.IniFilename = NULL;

	uf::hooks.addHook( "window:Mouse.CursorVisibility", [&]( pod::payloads::windowMouseCursorVisibility& payload ){
		io.MouseDrawCursor = payload.mouse.visible;
	});

	::console.position.x = uf::renderer::settings::width - ::console.size.x;

#if UF_ENV_WINDOWS
	ImGui_ImplWin32_Init(uf::renderer::device.window->getHandle());
#endif

#if UF_USE_VULKAN
	auto& renderMode = uf::renderer::getRenderMode("Gui", true);
	{
		VkDescriptorPoolSize poolSizes[] =
		{
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
		VK_CHECK_RESULT(vkCreateDescriptorPool(uf::renderer::device, &poolInfo, NULL, &::descriptorPool));
	}
	{
		ImGui_ImplVulkan_InitInfo imguiInitInfo = {};
		imguiInitInfo.Instance = uf::renderer::device.instance;
		imguiInitInfo.PhysicalDevice = uf::renderer::device.physicalDevice;
		imguiInitInfo.Device = uf::renderer::device.logicalDevice;
		imguiInitInfo.QueueFamily = uf::renderer::device.queueFamilyIndices.graphics;
		imguiInitInfo.Queue = uf::renderer::device.getQueue( uf::renderer::QueueEnum::GRAPHICS );
		imguiInitInfo.PipelineCache = uf::renderer::device.pipelineCache;
		imguiInitInfo.DescriptorPool = ::descriptorPool;
		imguiInitInfo.Subpass = 0;
		imguiInitInfo.MinImageCount = uf::renderer::swapchain.buffers;
		imguiInitInfo.ImageCount = uf::renderer::swapchain.buffers;
		imguiInitInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		imguiInitInfo.Allocator = NULL;
		imguiInitInfo.CheckVkResultFn = NULL;

		ImGui_ImplVulkan_Init(&imguiInitInfo, renderMode.renderTarget.renderPass);
	}
	{
		// Use any command queue
		auto commandBuffer = uf::renderer::device.fetchCommandBuffer(uf::renderer::QueueEnum::GRAPHICS);
		ImGui_ImplVulkan_CreateFontsTexture(commandBuffer.handle);
		uf::renderer::device.flushCommandBuffer(commandBuffer);
		ImGui_ImplVulkan_DestroyFontUploadObjects();
	}

	{
		renderMode.bindCallback( 0, [&]( VkCommandBuffer commandBuffer, size_t _ ){
			ImDrawData* drawData = ImGui::GetDrawData();
			ImGui_ImplVulkan_RenderDrawData(drawData, commandBuffer);
		});
	}

#elif UF_USE_OPENGL
    ImGui_ImplOpenGL2_Init();
#endif

    ::initialized = true;
}
void ext::imgui::tick() {
    if ( !::initialized ) ext::imgui::initialize();
    if ( !::initialized ) return;
	ImGuiIO& io = ImGui::GetIO();
	io.DeltaTime = uf::physics::time::delta;
	io.DisplaySize = ImVec2(uf::renderer::settings::width,uf::renderer::settings::height);

	ext::imgui::focused = io.WantCaptureKeyboard || io.WantCaptureMouse;

#if UF_USE_VULKAN
	auto& renderMode = uf::renderer::getRenderMode("Gui", true);
	renderMode.rerecord = true;

	ImGui_ImplVulkan_NewFrame();
#elif UF_USE_OPENGL
	ImGui_ImplOpenGL2_NewFrame();
#endif

#if UF_ENV_WINDOWS
	ImGui_ImplWin32_NewFrame();
#endif
	ImGui::NewFrame();
	
	auto& scene = uf::scene::getCurrentScene();
	if ( scene.globalFindByName("Gui: Menu") ) {
		bool opened;
		console.Draw("Console", opened);
	}
	
	ImGui::Render();
}
void ext::imgui::render() {
    if ( !::initialized ) return;
#if UF_USE_OPENGL
	auto renderMode = uf::renderer::getCurrentRenderMode();
	if ( !renderMode || renderMode->getName() != "Gui" ) return;
	auto data = ImGui::GetDrawData();
	if ( !data ) return;
	ImGui_ImplOpenGL2_RenderDrawData(data);
#endif
}
void ext::imgui::terminate() {
    if ( !::initialized ) return;
#if UF_USE_VULKAN
	ImGui_ImplVulkan_Shutdown();
#elif UF_USE_OPENGL
	ImGui_ImplOpenGL2_Shutdown();
#endif
#if UF_ENV_WINDOWS
	ImGui_ImplWin32_Shutdown();
#endif

	ImGui::DestroyContext();
}
#endif