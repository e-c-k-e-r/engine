#if UF_USE_IMGUI
#include <uf/ext/imgui/imgui.h>
#include <uf/utils/math/vector.h>
#include <imgui/imgui.h>

void ext::imgui::initialize() {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();

	uint8_t* tex_pixels{};
	pod::Vector2i tex_size{};
	io.Fonts->GetTexDataAsRGBA32(&tex_pixels, &tex_size.x, &tex_size.y);

	for ( size_t frame = 0; frame < 32; ++frame ) {
		io.DisplaySize = ImVec2(800,600);
		io.DeltaTime = 1.0f / 60.0f;
		ImGui::NewFrame();

		ImGui::ShowDemoWindow(NULL);

		ImGui::Render();
	}
}
void ext::imgui::tick() {
}
void ext::imgui::render() {
}
void ext::imgui::terminate() {
	ImGui::DestroyContext();
}
#endif