#define NOMINMAX
#include <Windows.h>
#include <memory/memory.h>
#include <sdk/sdk.h>
#include <sdk/offsets.h>
#include <game/game.h>
#include <settings.h>
#include "walkspeed.h"

namespace walkspeed
{
	void run()
	{
		for (;;)
		{
			Sleep(1);
			static bool original_speed_set = false;
			static float original_speed = 16.0f;
			static bool key_was_pressed = false;
			static bool toggle_state = false;

			bool key_pressed = false;
			if (settings::walkspeed::keybind_mode == 0)
			{
				key_pressed = GetAsyncKeyState(settings::walkspeed::keybind) & 0x8000;
			}
			else if (settings::walkspeed::keybind_mode == 1)
			{
				bool current_key_state = GetAsyncKeyState(settings::walkspeed::keybind) & 0x8000;
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
			else if (settings::walkspeed::keybind_mode == 2)
			{
				key_pressed = true;
			}

			if (!settings::walkspeed::enabled || !key_pressed)
			{
				original_speed_set = false;
				toggle_state = false;

				if (game::local_player.address != 0)
				{
					rbx::player_t local_player_obj = { game::local_player.address };
					rbx::model_instance_t model_instance = local_player_obj.get_model_instance();
					if (model_instance.address != 0)
					{
						rbx::humanoid_t humanoid = { model_instance.find_first_child("Humanoid").address };
						if (humanoid.address != 0 && original_speed_set)
						{
							memory->write<float>(humanoid.address + Offsets::Humanoid::Walkspeed, original_speed);
							memory->write<float>(humanoid.address + Offsets::Humanoid::WalkspeedCheck, original_speed);
						}
					}
				}
				continue;
			}

			rbx::player_t local_player_obj = { game::local_player.address };
			if (local_player_obj.address == 0)
				continue;

			rbx::model_instance_t model_instance = local_player_obj.get_model_instance();
			if (model_instance.address == 0)
				continue;

			rbx::humanoid_t humanoid = { model_instance.find_first_child("Humanoid").address };
			if (humanoid.address == 0)
				continue;

			if (!original_speed_set)
			{
				original_speed = memory->read<float>(humanoid.address + Offsets::Humanoid::Walkspeed);
				original_speed_set = true;
			}

			float current_speed = memory->read<float>(humanoid.address + Offsets::Humanoid::Walkspeed);
			if (current_speed != settings::walkspeed::speed)
			{
				for (int i = 0; i < 25000; i++)
				{
					memory->write<float>(humanoid.address + Offsets::Humanoid::Walkspeed, settings::walkspeed::speed);
					memory->write<float>(humanoid.address + Offsets::Humanoid::WalkspeedCheck, settings::walkspeed::speed);
				}
			}
		}
	}
}

