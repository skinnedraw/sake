#pragma once
#include <string>
#include <mutex>
#include <unordered_map>

#include <sdk/sdk.h>

namespace cache
{
	inline std::mutex mtx;

	struct entity_t final
	{
		rbx::instance_t instance;
		std::string name;

		std::uint8_t rig_type;
		
		rbx::humanoid_t humanoid;
		std::unordered_map<std::string, rbx::part_t> parts;
		std::string tool_name;
	};

	inline cache::entity_t cached_local_player;
	inline std::vector<cache::entity_t> cached_players;

	void run();
}