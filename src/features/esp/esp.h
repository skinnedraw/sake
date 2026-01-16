#pragma once
#include <string>
#include <cfloat>
#include <imgui/imgui.h>

struct esp_render_t
{
	ImFont* font = nullptr;

	void DrawTextWithSpacing(ImDrawList* draw, ImFont* font, float font_size, ImVec2 pos, ImU32 col, const std::string& text, float char_spacing = 1.0f) {
		if (!font || text.empty()) return;
		
		float x_offset = 0.0f;
		for (size_t i = 0; i < text.length(); i++) {
			char c = text[i];
			char char_str[2] = { c, '\0' };
			
			ImVec2 char_pos = ImVec2(pos.x + x_offset, pos.y);
			draw->AddText(font, font_size, char_pos, col, char_str);
			
			ImVec2 char_size = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, char_str);
			x_offset += char_size.x + char_spacing;
		}
	}

	void DrawTextWithSpacingAndOutline(ImDrawList* draw, ImFont* font, float font_size, ImVec2 pos, ImU32 col, ImU32 outline_col, const std::string& text, float char_spacing = 1.0f) {
		if (!font || text.empty()) return;
		
		for (int dx = -1; dx <= 1; dx++) {
			for (int dy = -1; dy <= 1; dy++) {
				if (dx != 0 || dy != 0) {
					DrawTextWithSpacing(draw, font, font_size, ImVec2(pos.x + dx, pos.y + dy), outline_col, text, char_spacing);
				}
			}
		}
		DrawTextWithSpacing(draw, font, font_size, pos, col, text, char_spacing);
	}
};

inline esp_render_t Visualize;

namespace esp
{
	void run();
}