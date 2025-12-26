#include "../../imgui/imgui.h"
#include <chrono>
#include <cmath>

void stroked_text(ImFont* font, float fontSize, ImVec2 position, ImColor color, const char* text)
{
	ImGui::GetForegroundDrawList()->AddText(font, fontSize, ImVec2(position.x - 1, position.y - 1), ImColor(0, 0, 0), text);
	ImGui::GetForegroundDrawList()->AddText(font, fontSize, ImVec2(position.x + 1, position.y - 1), ImColor(0, 0, 0), text);
	ImGui::GetForegroundDrawList()->AddText(font, fontSize, ImVec2(position.x - 1, position.y + 1), ImColor(0, 0, 0), text);
	ImGui::GetForegroundDrawList()->AddText(font, fontSize, ImVec2(position.x + 1, position.y + 1), ImColor(0, 0, 0), text);
	ImGui::GetForegroundDrawList()->AddText(font, fontSize, position, color, text);
}

void draw_box(int x, int y, int w, int h, const ImColor color)
{
	ImGui::GetForegroundDrawList()->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h), ImColor(40, 40, 40, 200), 0, 0);
	ImGui::GetForegroundDrawList()->AddRect(ImVec2(x + 1, y + 1), ImVec2(x + w - 1, y + h - 1), ImColor(0, 0, 0), 0, 0, 1.0f);
	ImGui::GetForegroundDrawList()->AddRect(ImVec2(x - 1, y - 1), ImVec2(x + w + 1, y + h + 1), ImColor(0, 0, 0), 0, 0, 1.0f);
	ImGui::GetForegroundDrawList()->AddRect(ImVec2(x + 1, y - 1), ImVec2(x + w - 1, y + h + 1), ImColor(0, 0, 0), 0, 0, 1.0f);
	ImGui::GetForegroundDrawList()->AddRect(ImVec2(x - 1, y + 1), ImVec2(x + w + 1, y + h - 1), ImColor(0, 0, 0), 0, 0, 1.0f);
	ImGui::GetForegroundDrawList()->AddRect(ImVec2(x, y), ImVec2(x + w, y + h), color, 0, 0, 1.0f);
}

static void image_rounded(ImDrawList* draw_list, ImTextureID user_texture_id, const ImVec2& p_min, const ImVec2& p_max, const ImVec2& uv_min, const ImVec2& uv_max, ImU32 col, float rounding, ImDrawFlags flags)
{
	if ((col & IM_COL32_A_MASK) == 0)
		return;

	auto fix_rect_corner_flags = [](ImDrawFlags rflags) {
		return (rflags & ImDrawFlags_RoundCornersMask_) ? rflags : (rflags | ImDrawFlags_RoundCornersAll);
		};

	draw_list->AddImageRounded(user_texture_id, p_min, p_max, uv_min, uv_max, col, rounding, fix_rect_corner_flags(flags));
}

static void draw_corner_box(float x, float y, float w, float h, ImColor color, bool outline, float thickness)
{
	float line_w = w / 4;
	float line_h = h / 4;
	auto draw_list = ImGui::GetForegroundDrawList();

	if (outline) {
		// Outline with black color and thicker lines
		ImColor outline_color = ImColor(0, 0, 0, 255);
		float outline_thickness = thickness + 2.0f;

		// Draw outline corners
		draw_list->AddLine(ImVec2(x, y), ImVec2(x, y + line_h), outline_color, outline_thickness);
		draw_list->AddLine(ImVec2(x, y), ImVec2(x + line_w, y), outline_color, outline_thickness);
		draw_list->AddLine(ImVec2(x + w - line_w, y), ImVec2(x + w, y), outline_color, outline_thickness);
		draw_list->AddLine(ImVec2(x + w, y), ImVec2(x + w, y + line_h), outline_color, outline_thickness);
		draw_list->AddLine(ImVec2(x, y + h - line_h), ImVec2(x, y + h), outline_color, outline_thickness);
		draw_list->AddLine(ImVec2(x, y + h), ImVec2(x + line_w, y + h), outline_color, outline_thickness);
		draw_list->AddLine(ImVec2(x + w - line_w, y + h), ImVec2(x + w, y + h), outline_color, outline_thickness);
		draw_list->AddLine(ImVec2(x + w, y + h - line_h), ImVec2(x + w, y + h), outline_color, outline_thickness);
	}

	// Draw main corners
	draw_list->AddLine(ImVec2(x, y), ImVec2(x, y + line_h), color, thickness);
	draw_list->AddLine(ImVec2(x, y), ImVec2(x + line_w, y), color, thickness);
	draw_list->AddLine(ImVec2(x + w - line_w, y), ImVec2(x + w, y), color, thickness);
	draw_list->AddLine(ImVec2(x + w, y), ImVec2(x + w, y + line_h), color, thickness);
	draw_list->AddLine(ImVec2(x, y + h - line_h), ImVec2(x, y + h), color, thickness);
	draw_list->AddLine(ImVec2(x, y + h), ImVec2(x + line_w, y + h), color, thickness);
	draw_list->AddLine(ImVec2(x + w - line_w, y + h), ImVec2(x + w, y + h), color, thickness);
	draw_list->AddLine(ImVec2(x + w, y + h - line_h), ImVec2(x + w, y + h), color, thickness);
}

inline ImU32 GetRainbowColor(float speed = 2.0f, int alpha = 255)
{
    // Get current time in milliseconds
    auto now = std::chrono::steady_clock::now();
    auto duration = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    
    // Create smooth rainbow transition
    float time = (millis * speed * 0.001f);
    float r = sin(time) * 0.5f + 0.5f;
    float g = sin(time + 2.09f) * 0.5f + 0.5f;  // 2*PI/3
    float b = sin(time + 4.19f) * 0.5f + 0.5f;  // 4*PI/3
    
    return IM_COL32(
        (int)(r * 255), 
        (int)(g * 255), 
        (int)(b * 255), 
        alpha
    );
}

inline void DrawRainbowFilledBox(float x, float y, float w, float h, bool drawOutline = false)
{
    auto* drawList = ImGui::GetForegroundDrawList();
    
    // Get rainbow colors with slight offset for gradient effect
    ImU32 topColor = GetRainbowColor(2.0f, 150);
    ImU32 bottomColor = GetRainbowColor(2.2f, 150);  // Slightly different speed for gradient
    
    // Draw the rainbow gradient filled box
    drawList->AddRectFilledMultiColor(
        ImVec2(x, y), 
        ImVec2(x + w, y + h),
        topColor,    // Top left
        topColor,    // Top right  
        bottomColor, // Bottom right
        bottomColor  // Bottom left
    );
    
    // Draw outline if requested
    if (drawOutline) {
        ImU32 rainbowOutline = GetRainbowColor(2.5f, 255);
        drawList->AddRect(ImVec2(x, y), ImVec2(x + w, y + h), rainbowOutline, 0.0f, 0, 2.0f);
    }
}

inline void DrawGradientFilledBox(float x, float y, float w, float h, ImU32 topColor, ImU32 bottomColor, bool drawOutline = false)
{
    auto* drawList = ImGui::GetForegroundDrawList();
    
    // Draw the gradient filled box
    drawList->AddRectFilledMultiColor(
        ImVec2(x, y), 
        ImVec2(x + w, y + h),
        topColor,    // Top left
        topColor,    // Top right  
        bottomColor, // Bottom right
        bottomColor  // Bottom left
    );
    
    // Draw outline if requested
    if (drawOutline) {
        drawList->AddRect(ImVec2(x, y), ImVec2(x + w, y + h), IM_COL32(0, 0, 0, 255), 0.0f, 0, 2.0f);
    }
}