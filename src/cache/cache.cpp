#include "cache.h"
#include <thread>
#include <game/game.h>

void cache::run()
{
	while (true)
	{
		rbx::player_t local_player_obj = { game::local_player.address };
		game::local_character = { local_player_obj.get_model_instance().address };

		std::vector<rbx::player_t> players = game::players.get_children<rbx::player_t>();

		std::vector<cache::entity_t> temp_cache;
		
		for (rbx::player_t& player : players)
		{
			cache::entity_t entity{};

			entity.instance = { player.address };
			entity.name = player.get_name();
			
			rbx::model_instance_t model_instance = player.get_model_instance();

			for (rbx::part_t& part : model_instance.get_children<rbx::part_t>())
			{
				std::string part_class = part.get_class_name();
				if (part_class.find("Part") != std::string::npos)
				{
					entity.parts[part.get_name()] = part;
				}
			}

			entity.humanoid = { model_instance.find_first_child("Humanoid").address };
			entity.rig_type = entity.humanoid.get_rig_type();

			rbx::instance_t character_model = { model_instance.address };
			
			entity.tool_name = "";
			for (rbx::instance_t& child : character_model.get_children<rbx::instance_t>())
			{
				std::string child_class = child.get_class_name();
				if (child_class == "Tool" || child_class == "HopperBin")
				{
					entity.tool_name = child.get_name();
					break;
				}
			}

			temp_cache.push_back(entity);
		}

		{
			std::lock_guard<std::mutex> lock(mtx);
			cached_players = std::move(temp_cache);
			
			for (cache::entity_t& entity : cached_players)
			{
				if (entity.instance.address == game::local_player.address)
				{
					cached_local_player = entity;
					break;
				}
			}
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}