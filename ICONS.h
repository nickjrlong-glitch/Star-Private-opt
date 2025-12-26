#include "Sophie_rain_esp/SophierainEsp-bytes.hpp"
#include <imgui.h>

extern ImFont* Icon;
void icons()
{
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	Icon = io.Fonts->AddFontFromMemoryTTF(&Icon_Pack, sizeof Icon_Pack, 26.f, NULL, io.Fonts->GetGlyphRangesCyrillic());
}