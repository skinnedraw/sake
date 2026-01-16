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

	POINT cursor_point;
	GetCursorPos(&cursor_point);
	HWND rblxWnd = FindWindowA(nullptr, "Roblox");
	if (!rblxWnd || !ScreenToClient(rblxWnd, &cursor_point))
		return closest_player;

	math::vector2 cursor = { static_cast<float>(cursor_point.x), static_cast<float>(cursor_point.y) };
	math::vector2 dimensions = game::visengine.get_dimensions();
	math::matrix4 viewMatrix = game::visengine.get_viewmatrix();

	std::lock_guard<std::mutex> lock(cache::mtx);
	
	for (auto& player : cache::cached_players)
	{
		if (player.instance.address == 0 || player.instance.address == cache::cached_local_player.instance.address)
			continue;

		POINT cursor_point_temp;
		GetCursorPos(&cursor_point_temp);
		HWND rblxWnd_temp = FindWindowA(nullptr, "Roblox");
		if (rblxWnd_temp)
			ScreenToClient(rblxWnd_temp, &cursor_point_temp);
		rbx::part_t root_part = get_target_part(player, 1);
		if (!root_part.address)
			continue;

		rbx::primitive_t primitive = root_part.get_primitive();
		math::vector3 part_world_pos = primitive.get_position();
		math::vector2 screen_pos{};
		
		if (!game::visengine.world_to_screen(part_world_pos, screen_pos, dimensions, viewMatrix))
			continue;

		float cursor_dist = std::sqrt(
			(screen_pos.x - cursor.x) * (screen_pos.x - cursor.x) +
			(screen_pos.y - cursor.y) * (screen_pos.y - cursor.y)
		);

		if (cursor_dist < shortest_distance)
		{
			shortest_distance = cursor_dist;
			closest_player = player;
		}
	}

	return closest_player;
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
		
		if (!settings::aimbot::enabled || !key_pressed)
			continue;

		if (!game::workspace.address)
			continue;

		cache::entity_t target = get_closest_player();
		if (!target.instance.address)
			continue;

		POINT cursor_point;
		GetCursorPos(&cursor_point);
		HWND rblxWnd = FindWindowA(nullptr, "Roblox");
		if (rblxWnd)
			ScreenToClient(rblxWnd, &cursor_point);

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
		math::matrix3 targetMatrix = look_at_to_matrix(camera_pos, target_pos);
		camera.write_rotation(targetMatrix);
	}
}

void rbx::aimbot::initialize()
{
}

