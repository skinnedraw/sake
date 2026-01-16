#pragma once
#include "keybind/keybind.h"

namespace settings
{
	namespace visuals
	{
		inline bool box{ false };
		inline float box_color[4]{ 1.f, 1.f, 1.f, 1.f };

		inline bool name{ false };
		inline float name_color[4]{ 1.f, 1.f, 1.f, 1.f };

		inline bool distance{ false };
		inline float distance_color[4]{ 1.f, 1.f, 1.f, 1.f };

		inline bool tool{ false };
		inline float tool_color[4]{ 1.f, 1.f, 1.f, 1.f };

		inline bool localplayer{ false };
	}

	namespace aimbot
	{
		inline bool enabled{ false };
		inline int aim_part{ 0 };
		inline int keybind{ 0 };
		inline int keybind_mode{ 0 };
		inline float fov{ 100.0f };
		inline bool show_fov{ false };
		inline float fov_color[4]{ 1.f, 1.f, 1.f, 1.f };
	}

	namespace walkspeed
	{
		inline bool enabled{ false };
		inline float speed{ 16.0f };
		inline int keybind{ 0 };
		inline int keybind_mode{ 0 };
	}

	namespace settings
	{
		inline bool hide_console{ false };
		inline bool should_unload{ false };
	}
}