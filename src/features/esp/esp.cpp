#define NOMINMAX
#include "esp.h"

#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <settings.h>
#include <game/game.h>
#include <cache/cache.h>

namespace helper
{
	__forceinline void box(ImVec2& c1, ImVec2& c2, ImU32 color)
	{
		c1.x = std::round(c1.x);
		c1.y = std::round(c1.y);
		c2.x = std::round(c2.x);
		c2.y = std::round(c2.y);

		ImDrawList* draw = ImGui::GetBackgroundDrawList();
		draw->Flags &= ImDrawListFlags_AntiAliasedLines;

		ImRect rect(c1.x, c1.y, c1.x + c2.x, c1.y + c2.y);
		ImVec2 shadow = { cosf(0.f) * 2.f, sinf(0.f) * 2.f };

		draw->AddRect(rect.Min, rect.Max, IM_COL32(0, 0, 0, color >> 24));
		draw->AddRect({ rect.Min.x - 1.f, rect.Min.y - 1.f }, { rect.Max.x + 1.f, rect.Max.y + 1.f }, color);
		draw->AddRect({ rect.Min.x - 2.f, rect.Min.y - 2.f }, { rect.Max.x + 2.f, rect.Max.y + 2.f }, IM_COL32(0, 0, 0, color >> 24));
	}
}

void esp::run()
{
	static math::vector3 corners[8] =
	{
		{-1, -1, -1}, {1, -1, -1}, {-1, 1, -1},{1, 1, -1},
		{-1, -1, 1}, {1, -1, 1}, {-1, 1, 1}, {1, 1, 1}
	};

	math::vector2 dims = game::visengine.get_dimensions();
	math::matrix4 view = game::visengine.get_viewmatrix();

	std::vector<cache::entity_t> snapshot;
	{
		std::lock_guard<std::mutex> lock(cache::mtx);
		snapshot = cache::cached_players;
	}

	for (cache::entity_t& entity : snapshot)
	{
		bool valid = false;
		float left = FLT_MAX, top = FLT_MAX;
		float right = -FLT_MAX, bottom = -FLT_MAX;

		if (entity.instance.address == 0)
		{
			continue;
		}

		if (!settings::visuals::localplayer && entity.instance.address == cache::cached_local_player.instance.address)
		{
			continue;
		}

		for (auto& parts : entity.parts)
		{
			rbx::primitive_t prim = parts.second.get_primitive();
			auto size = prim.get_size();
			auto pos = prim.get_position();
			auto rot = prim.get_rotation();

			if (size.x == 0 || size.y == 0 || size.z == 0)
			{
				continue;
			}

			for (auto& corner : corners)
			{
				math::vector3 world = pos + rot * math::vector3
				{
					corner.x * size.x * 0.5f,
					corner.y * size.y * 0.5f,
					corner.z * size.z * 0.5f
				};

				math::vector2 out{};
				if (game::visengine.world_to_screen(world, out, dims, view))
				{
					valid = true;
					left = (std::min)(left, out.x);
					top = (std::min)(top, out.y);
					right = (std::max)(right, out.x);
					bottom = (std::max)(bottom, out.y);
				}
			}
		}

		if (!valid || left >= right || top >= bottom)
		{
			continue;
		}

		ImVec2 c1(left, top);
		ImVec2 c2(right - left, bottom - top);

		ImVec2 boxPos = c1;
		ImVec2 boxBR = ImVec2(c1.x + c2.x, c1.y + c2.y);

		if (settings::visuals::box)
		{
			helper::box(c1, c2, ImGui::ColorConvertFloat4ToU32
			(
				{
					settings::visuals::box_color[0],
					settings::visuals::box_color[1],
					settings::visuals::box_color[2],
					settings::visuals::box_color[3]
				}
			));
		}

		if (settings::visuals::name && Visualize.font)
		{
			std::string player_name = entity.name;
			
			ImFont* font = Visualize.font;
			float font_size = 13.0f;
			float char_spacing = 1.0f;
			
			float total_width = 0.0f;
			float max_height = 0.0f;
			for (size_t i = 0; i < player_name.length(); i++) {
				char c = player_name[i];
				char char_str[2] = { c, '\0' };
				ImVec2 char_size = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, char_str);
				total_width += char_size.x + (i < player_name.length() - 1 ? char_spacing : 0.0f);
				max_height = std::max(max_height, char_size.y);
			}
			
			float centerX = (boxPos.x + boxBR.x) * 0.5f;
			float textX = centerX - (total_width * 0.5f);
			float textY = boxPos.y - max_height - 2.0f;
			
			ImVec2 textPos = ImVec2(std::floor(textX + 0.5f), std::floor(textY + 0.5f));
			
			ImU32 nameColor = ImGui::ColorConvertFloat4ToU32(ImVec4(
				settings::visuals::name_color[0],
				settings::visuals::name_color[1],
				settings::visuals::name_color[2],
				settings::visuals::name_color[3]
			));
			
			ImDrawList* draw = ImGui::GetBackgroundDrawList();
			Visualize.DrawTextWithSpacingAndOutline(draw, font, font_size, textPos, nameColor, IM_COL32(0, 0, 0, 255), player_name, char_spacing);
		}

		if (settings::visuals::distance && Visualize.font)
		{
			auto hrp_it = entity.parts.find("HumanoidRootPart");
			if (hrp_it != entity.parts.end() && game::local_character.address != 0)
			{
				rbx::part_t local_hrp = { game::local_character.find_first_child("HumanoidRootPart").address };
				if (local_hrp.address != 0)
				{
					math::vector3 local_pos = local_hrp.get_primitive().get_position();
					math::vector3 enemy_pos = hrp_it->second.get_primitive().get_position();
					
					float dx = enemy_pos.x - local_pos.x;
					float dy = enemy_pos.y - local_pos.y;
					float dz = enemy_pos.z - local_pos.z;
					float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
					
					char distanceStr[32];
					snprintf(distanceStr, sizeof(distanceStr), "[%.1fM]", dist);
					
					ImFont* font = Visualize.font;
					float font_size = 13.0f;
					float char_spacing = 1.0f;
					
					float total_width = 0.0f;
					float max_height = 0.0f;
					for (size_t i = 0; i < strlen(distanceStr); i++) {
						char c = distanceStr[i];
						char char_str[2] = { c, '\0' };
						ImVec2 char_size = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, char_str);
						total_width += char_size.x + (i < strlen(distanceStr) - 1 ? char_spacing : 0.0f);
						max_height = (std::max)(max_height, char_size.y);
					}
					
					float centerX = (boxPos.x + boxBR.x) * 0.5f;
					float textX = centerX - (total_width * 0.5f);
					float textY = boxBR.y + 2.0f;
					
					ImVec2 textPos = ImVec2(std::floor(textX + 0.5f), std::floor(textY + 0.5f));
					
					ImU32 distColor = ImGui::ColorConvertFloat4ToU32(ImVec4(
						settings::visuals::distance_color[0],
						settings::visuals::distance_color[1],
						settings::visuals::distance_color[2],
						settings::visuals::distance_color[3]
					));
					
					ImDrawList* draw = ImGui::GetBackgroundDrawList();
					Visualize.DrawTextWithSpacingAndOutline(draw, font, font_size, textPos, distColor, IM_COL32(0, 0, 0, 255), std::string(distanceStr), char_spacing);
				}
			}
		}

		if (settings::visuals::tool && Visualize.font)
		{
			std::string toolName = entity.tool_name;
			
			if (!toolName.empty())
			{
				ImFont* font = Visualize.font;
				float font_size = 13.0f;
				float char_spacing = 1.0f;
				
				float total_width = 0.0f;
				float max_height = 0.0f;
				for (size_t i = 0; i < toolName.length(); i++) {
					char c = toolName[i];
					char char_str[2] = { c, '\0' };
					ImVec2 char_size = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, char_str);
					total_width += char_size.x + (i < toolName.length() - 1 ? char_spacing : 0.0f);
					max_height = (std::max)(max_height, char_size.y);
				}
				
				float centerX = (boxPos.x + boxBR.x) * 0.5f;
				float textX = centerX - (total_width * 0.5f);
				float textY = boxBR.y + 2.0f;
				
				if (settings::visuals::distance)
					textY += 18.0f;
				
				ImVec2 textPos = ImVec2(std::floor(textX + 0.5f), std::floor(textY + 0.5f));
				
				ImU32 toolColor = ImGui::ColorConvertFloat4ToU32(ImVec4(
					settings::visuals::tool_color[0],
					settings::visuals::tool_color[1],
					settings::visuals::tool_color[2],
					settings::visuals::tool_color[3]
				));
				
				ImDrawList* draw = ImGui::GetBackgroundDrawList();
				Visualize.DrawTextWithSpacingAndOutline(draw, font, font_size, textPos, toolColor, IM_COL32(0, 0, 0, 255), toolName, char_spacing);
			}
		}
	}
}