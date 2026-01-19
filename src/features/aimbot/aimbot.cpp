#define NOMINMAX
#include <Windows.h>
#include <thread>
#include <vector>
#include <algorithm>
#include <cmath>
#include <limits>
#include <mutex>

#include <memory/memory.h>
#include <sdk/sdk.h>
#include <sdk/offsets.h>
#include <cache/cache.h>
#include <game/game.h>
#include <settings.h>
#include "aimbot.h"

// Static variable to store the locked target
static cache::entity_t locked_target{};
static bool was_aimbot_active = false;

static math::vector3 normalize(const math::vector3& vec)
{
	float length = std::sqrt(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
	return (length != 0) ? math::vector3{ vec.x / length, vec.y / length, vec.z / length } : vec;
}

static math::vector3 cross_product(const math::vector3& vec1, const math::vector3& vec2)
{
	return {
		vec1.y * vec2.z - vec1.z * vec2.y,
		vec1.z * vec2.x - vec1.x * vec2.z,
		vec1.x * vec2.y - vec1.y * vec2.x
	};
}

static math::matrix3 look_at_to_matrix(const math::vector3& cameraPosition, const math::vector3& targetPosition)
{
	math::vector3 forward = normalize(math::vector3{
		targetPosition.x - cameraPosition.x,
		targetPosition.y - cameraPosition.y,
		targetPosition.z - cameraPosition.z
		});
	math::vector3 right = normalize(cross_product({ 0, 1, 0 }, forward));
	math::vector3 up = cross_product(forward, right);

	math::matrix3 lookAtMatrix{};
	lookAtMatrix.m[0] = -right.x;  lookAtMatrix.m[1] = up.x;  lookAtMatrix.m[2] = -forward.x;
	lookAtMatrix.m[3] = right.y;  lookAtMatrix.m[4] = up.y;  lookAtMatrix.m[5] = -forward.y;
	lookAtMatrix.m[6] = -right.z;  lookAtMatrix.m[7] = up.z;  lookAtMatrix.m[8] = -forward.z;

	return lookAtMatrix;
}

static rbx::part_t get_target_part(cache::entity_t& player, int aimpart)
{
	rbx::part_t target_part{};

	if (aimpart == 0)
	{
		auto part_it = player.parts.find("Head");
		if (part_it != player.parts.end())
			target_part = part_it->second;
	}
	else if (aimpart == 1)
	{
		auto torso_it = player.parts.find("UpperTorso");
		if (torso_it != player.parts.end())
			target_part = torso_it->second;
		else
		{
			auto torso_r6 = player.parts.find("Torso");
			if (torso_r6 != player.parts.end())
				target_part = torso_r6->second;
		}
	}
	else if (aimpart == 2)
	{
		POINT cursor_point;
		HWND rblxWnd = FindWindowA(nullptr, "Roblox");
		if (rblxWnd && GetCursorPos(&cursor_point) && ScreenToClient(rblxWnd, &cursor_point))
		{
			math::vector2 cursor = { static_cast<float>(cursor_point.x), static_cast<float>(cursor_point.y) };
			math::vector2 dimensions = game::visengine.get_dimensions();
			math::matrix4 viewmatrix = game::visengine.get_viewmatrix();

			float shortest_distance = (std::numeric_limits<float>::max)();

			for (auto& part_pair : player.parts)
			{
				rbx::primitive_t prim = part_pair.second.get_primitive();
				if (!prim.address)
					continue;

				math::vector3 part_position = prim.get_position();
				math::vector2 part_screen{};
				if (!game::visengine.world_to_screen(part_position, part_screen, dimensions, viewmatrix))
					continue;

				if (part_screen.x < 0 || part_screen.y < 0)
					continue;

				float distance = std::sqrt(
					(part_screen.x - cursor.x) * (part_screen.x - cursor.x) +
					(part_screen.y - cursor.y) * (part_screen.y - cursor.y)
				);
				if (distance < shortest_distance)
				{
					shortest_distance = distance;
					target_part = part_pair.second;
				}
			}
		}
	}

	return target_part;
}

static cache::entity_t get_closest_player()
{
	cache::entity_t closest_player{};
	float shortest_distance = FLT_MAX;

	HWND rblxWnd = FindWindowA(nullptr, "Roblox");
	if (!rblxWnd)
		return closest_player;

	math::vector2 fov_center{};

	// Dynamic FOV center based on aimbot type
	if (settings::aimbot::aim_type == 0)  // Camera mode - center of screen
	{
		RECT windowRect;
		GetClientRect(rblxWnd, &windowRect);
		fov_center = {
			static_cast<float>((windowRect.right - windowRect.left) / 2),
			static_cast<float>((windowRect.bottom - windowRect.top) / 2)
		};
	}
	else if (settings::aimbot::aim_type == 1)  // Mouse mode - cursor position
	{
		POINT cursor_point;
		GetCursorPos(&cursor_point);
		ScreenToClient(rblxWnd, &cursor_point);
		fov_center = { static_cast<float>(cursor_point.x), static_cast<float>(cursor_point.y) };
	}

	math::vector2 dimensions = game::visengine.get_dimensions();
	math::matrix4 viewMatrix = game::visengine.get_viewmatrix();

	std::lock_guard<std::mutex> lock(cache::mtx);

	for (auto& player : cache::cached_players)
	{
		if (player.instance.address == 0 || player.instance.address == cache::cached_local_player.instance.address)
			continue;

		rbx::part_t root_part = get_target_part(player, 1);
		if (!root_part.address)
			continue;

		rbx::primitive_t primitive = root_part.get_primitive();
		math::vector3 part_world_pos = primitive.get_position();
		math::vector2 screen_pos{};

		if (!game::visengine.world_to_screen(part_world_pos, screen_pos, dimensions, viewMatrix))
			continue;

		// Calculate distance from FOV center (dynamic based on mode)
		float distance_from_center = std::sqrt(
			(screen_pos.x - fov_center.x) * (screen_pos.x - fov_center.x) +
			(screen_pos.y - fov_center.y) * (screen_pos.y - fov_center.y)
		);

		// Only consider players within FOV radius
		if (distance_from_center > settings::aimbot::fov)
			continue;

		// Find closest player within FOV
		if (distance_from_center < shortest_distance)
		{
			shortest_distance = distance_from_center;
			closest_player = player;
		}
	}

	return closest_player;
}

static bool is_locked_target_valid()
{
	if (locked_target.instance.address == 0)
		return false;

	// Check if the locked target still exists in cached players
	std::lock_guard<std::mutex> lock(cache::mtx);
	for (auto& player : cache::cached_players)
	{
		if (player.instance.address == locked_target.instance.address)
		{
			// Update the locked target with fresh data
			locked_target = player;
			return true;
		}
	}

	return false;
}

void rbx::aimbot::run()
{
	for (;;)
	{
		Sleep(1);

		bool key_pressed = false;
		if (settings::aimbot::keybind_mode == 0)
		{
			key_pressed = GetAsyncKeyState(settings::aimbot::keybind) & 0x8000;
		}
		else if (settings::aimbot::keybind_mode == 1)
		{
			static bool key_was_pressed = false;
			static bool toggle_state = false;
			bool current_key_state = GetAsyncKeyState(settings::aimbot::keybind) & 0x8000;
			if (current_key_state && !key_was_pressed)
			{
				toggle_state = !toggle_state;
				key_was_pressed = true;
			}
			else if (!current_key_state)
			{
				key_was_pressed = false;
			}
			key_pressed = toggle_state;
		}
		else if (settings::aimbot::keybind_mode == 2)
		{
			key_pressed = true;
		}

		bool is_aimbot_active = settings::aimbot::enabled && key_pressed;

		if (!is_aimbot_active && was_aimbot_active)
		{
			locked_target = cache::entity_t{};
		}

		was_aimbot_active = is_aimbot_active;

		if (!is_aimbot_active)
			continue;

		if (!game::workspace.address)
			continue;

		cache::entity_t target{};

		if (settings::aimbot::sticky_aim)
		{
			if (is_locked_target_valid())
			{
				target = locked_target;
			}
			else
			{
				target = get_closest_player();
				if (target.instance.address != 0)
				{
					locked_target = target;
				}
			}
		}
		else
		{
			target = get_closest_player();
			locked_target = cache::entity_t{};
		}

		if (!target.instance.address)
			continue;

		rbx::part_t target_part = get_target_part(target, settings::aimbot::aim_part);
		if (!target_part.address)
			continue;

		rbx::primitive_t primitive = target_part.get_primitive();
		math::vector3 target_pos = primitive.get_position();

		rbx::instance_t camera_instance = game::workspace.find_first_child_by_class("Camera");
		if (!camera_instance.address)
			continue;

		rbx::camera_t camera{ camera_instance.address };
		math::vector3 camera_pos = camera.get_position();

		// Calculate target rotation matrix
		math::matrix3 target_rotation = look_at_to_matrix(camera_pos, target_pos);

		// Camera mode - always use camera rotation
		if (settings::aimbot::aim_type == 0)
		{
			// Apply smoothing if enabled
			if (settings::aimbot::smoothing < 1.0f)
			{
				math::matrix3 current_rotation = camera.get_rotation();
				math::matrix3 smoothed_rotation{};
				for (int i = 0; i < 9; i++)
				{
					smoothed_rotation.m[i] = current_rotation.m[i] + (target_rotation.m[i] - current_rotation.m[i]) * settings::aimbot::smoothing;
				}
				camera.write_rotation(smoothed_rotation);
			}
			else
			{
				camera.write_rotation(target_rotation);
			}
		}
		// Mouse mode - try mouse movement first, fall back to SendInput
		else if (settings::aimbot::aim_type == 1)
		{
			HWND rblxWnd = FindWindowA(nullptr, "Roblox");
			if (!rblxWnd)
				continue;

			math::vector2 dimensions = game::visengine.get_dimensions();
			math::matrix4 viewmatrix = game::visengine.get_viewmatrix();
			math::vector2 screen_pos{};

			if (!game::visengine.world_to_screen(target_pos, screen_pos, dimensions, viewmatrix))
				continue;

			// Get current mouse position
			POINT current_pos;
			GetCursorPos(&current_pos);
			POINT client_pos = current_pos;
			ScreenToClient(rblxWnd, &client_pos);

			// Calculate distance from target
			float delta_x = screen_pos.x - static_cast<float>(client_pos.x);
			float delta_y = screen_pos.y - static_cast<float>(client_pos.y);
			float distance = std::sqrt(delta_x * delta_x + delta_y * delta_y);

			// Skip if already very close
			if (distance < 2.0f)
				continue;

			// Check if mouse is locked to center (first person indicator)
			RECT windowRect;
			GetClientRect(rblxWnd, &windowRect);
			float center_x = static_cast<float>((windowRect.right - windowRect.left) / 2);
			float center_y = static_cast<float>((windowRect.bottom - windowRect.top) / 2);
			float dist_from_center = std::sqrt(
				(client_pos.x - center_x) * (client_pos.x - center_x) +
				(client_pos.y - center_y) * (client_pos.y - center_y)
			);

			// If mouse is stuck at center (within 5 pixels), we're in first person
			if (dist_from_center < 5.0f)
			{
				// First person - use SendInput with relative movement (works in first person!)
				int move_x = static_cast<int>(delta_x * settings::aimbot::smoothing);
				int move_y = static_cast<int>(delta_y * settings::aimbot::smoothing);

				INPUT input = { 0 };
				input.type = INPUT_MOUSE;
				input.mi.dx = move_x;
				input.mi.dy = move_y;
				input.mi.dwFlags = MOUSEEVENTF_MOVE;
				input.mi.time = 0;
				input.mi.dwExtraInfo = 0;

				SendInput(1, &input, sizeof(INPUT));
			}
			else
			{
				// Third person - use SetCursorPos (absolute positioning works better here)
				float move_x = delta_x * settings::aimbot::smoothing;
				float move_y = delta_y * settings::aimbot::smoothing;

				// Calculate smoothed target position in screen coordinates
				POINT target_screen;
				target_screen.x = static_cast<LONG>(current_pos.x + move_x);
				target_screen.y = static_cast<LONG>(current_pos.y + move_y);

				// Move mouse
				SetCursorPos(target_screen.x, target_screen.y);
			}
		}
	}
}

void rbx::aimbot::initialize()
{
}