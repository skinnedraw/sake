#include <cstdint>
#include <chrono>
#include <thread>

#include <memory/memory.h>
#include <sdk/offsets.h>
#include <sdk/sdk.h>
#include <game/game.h>
#include <cache/cache.h>
#include <render/render.h>
#include <features/aimbot/aimbot.h>
#include <features/walkspeed/walkspeed.h>
#include <settings.h>

std::int32_t main()
{
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD dwMode = 0;
	GetConsoleMode(hOut, &dwMode);
	SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
	printf("\x1b[38;2;169;169;169m- \x1b[38;2;255;255;255mWelcome to \x1b[0;38;5;139;49msake dev 0.0\x1b[0m\n");

	static const char* BINARY_NAME = "RobloxPlayerBeta.exe";

	if (!memory->find_process_id(BINARY_NAME))
	{
		printf("\x1b[38;2;169;169;169m- \x1b[38;2;255;255;255mWaiting for you to open \x1b[0;38;5;139;49m%s\x1b[0m\n", BINARY_NAME);
		while (!memory->find_process_id(BINARY_NAME))
			std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	printf("\x1b[38;2;169;169;169m- \x1b[38;2;255;255;255mFound \x1b[0;38;5;139;49m%s \x1b[38;2;169;169;169m@ \x1b[0;38;5;139;49m%u\x1b[0m\n", BINARY_NAME, memory->get_process_id());

	if (!memory->attach_to_process(BINARY_NAME))
	{
		printf("\x1b[38;2;169;169;169m- \x1b[38;2;255;255;255mUnable to attach to \x1b[0;38;5;139;49m%s\x1b[0m\n", BINARY_NAME);
		std::this_thread::sleep_for(std::chrono::seconds(5));
		return 1;
	}

	if (!memory->find_module_address(BINARY_NAME))
	{
		printf("\x1b[38;2;169;169;169m- \x1b[38;2;255;255;255mUnable to find \x1b[0;38;5;139;49mmain module address\x1b[0m\n");
		std::this_thread::sleep_for(std::chrono::seconds(5));
		return 1;
	}

	printf("\x1b[38;2;169;169;169m- \x1b[38;2;255;255;255mFound \x1b[0;38;5;139;49mbase \x1b[38;2;169;169;169m@ \x1b[0;38;5;139;49m0x%llx\x1b[0m\n", memory->get_module_address());

	std::uint64_t fake_datamodel{ memory->read<std::uint64_t>(memory->get_module_address() + Offsets::FakeDataModel::Pointer) };
	game::datamodel = rbx::instance_t(memory->read<std::uint64_t>(fake_datamodel + Offsets::FakeDataModel::RealDataModel));
	
	if (game::datamodel.get_name() != "Ugc")
	{
		printf("\x1b[38;2;169;169;169m- \x1b[38;2;255;255;255mWaiting for \x1b[0;38;5;139;49mdatamodel \x1b[38;2;169;169;169mto load...\x1b[0m\n");
		while (game::datamodel.get_name() != "Ugc")
		{
			std::uint64_t fake_datamodel{ memory->read<std::uint64_t>(memory->get_module_address() + Offsets::FakeDataModel::Pointer) };
			game::datamodel = rbx::instance_t(memory->read<std::uint64_t>(fake_datamodel + Offsets::FakeDataModel::RealDataModel));
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		}
	}
	
	printf("\x1b[38;2;169;169;169m- \x1b[38;2;255;255;255mFound \x1b[0;38;5;139;49mdatamodel \x1b[38;2;169;169;169m@ \x1b[0;38;5;139;49m0x%llx \x1b[38;2;169;169;169m(%s)\x1b[0m\n", game::datamodel.address, game::datamodel.get_name().c_str());

	game::visengine = { memory->read<std::uint64_t>(memory->get_module_address() + Offsets::VisualEngine::Pointer) };
	printf("\x1b[38;2;169;169;169m- \x1b[38;2;255;255;255mFound \x1b[0;38;5;139;49mvisualengine \x1b[38;2;169;169;169m@ \x1b[0;38;5;139;49m0x%llx\x1b[0m\n", game::visengine.address);

	game::workspace = { game::datamodel.find_first_child_by_class("Workspace") };
	printf("\x1b[38;2;169;169;169m- \x1b[38;2;255;255;255mFound \x1b[0;38;5;139;49mworkspace \x1b[38;2;169;169;169m@ \x1b[0;38;5;139;49m0x%llx\x1b[0m\n", game::workspace.address);

	game::players = { game::datamodel.find_first_child_by_class("Players") };
	printf("\x1b[38;2;169;169;169m- \x1b[38;2;255;255;255mFound \x1b[0;38;5;139;49mplayers \x1b[38;2;169;169;169m@ \x1b[0;38;5;139;49m0x%llx\x1b[0m\n", game::players.address);

	if (game::local_player.address == 0)
	{
		printf("\x1b[38;2;169;169;169m- \x1b[38;2;255;255;255mWaiting for \x1b[0;38;5;139;49mlocal_player \x1b[38;2;169;169;169mto load...\x1b[0m\n");
		while (game::local_player.address == 0)
		{
			game::local_player = { memory->read<std::uint64_t>(game::players.address + Offsets::Player::LocalPlayer) };
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		}
	}

	printf("\x1b[38;2;169;169;169m- \x1b[38;2;255;255;255mFound \x1b[0;38;5;139;49mlocal_player \x1b[38;2;169;169;169m@ \x1b[0;38;5;139;49m0x%llx\x1b[0m\n\n", game::local_player.address);

	game::detect_game();

	printf("\n\x1b[38;2;169;169;169m- \x1b[38;2;255;255;255mAddress \x1b[0;38;5;139;49mcache:\x1b[0m\n");
	printf("\x1b[38;2;169;169;169m- - \x1b[38;2;255;255;255mbase \x1b[38;2;169;169;169m        @ \x1b[0;38;5;139;49m0x%llx\x1b[0m\n", memory->get_module_address());
	printf("\x1b[38;2;169;169;169m- - \x1b[38;2;255;255;255mdatamodel \x1b[38;2;169;169;169m   @ \x1b[0;38;5;139;49m0x%llx\x1b[0m\n", game::datamodel.address);
	printf("\x1b[38;2;169;169;169m- - \x1b[38;2;255;255;255mvisualengine \x1b[38;2;169;169;169m@ \x1b[0;38;5;139;49m0x%llx\x1b[0m\n", game::visengine.address);
	printf("\x1b[38;2;169;169;169m- - \x1b[38;2;255;255;255mworkspace \x1b[38;2;169;169;169m   @ \x1b[0;38;5;139;49m0x%llx\x1b[0m\n", game::workspace.address);
	printf("\x1b[38;2;169;169;169m- - \x1b[38;2;255;255;255mplayers \x1b[38;2;169;169;169m     @ \x1b[0;38;5;139;49m0x%llx\x1b[0m\n", game::players.address);
	printf("\x1b[38;2;169;169;169m- - \x1b[38;2;255;255;255mlocal_player \x1b[38;2;169;169;169m@ \x1b[0;38;5;139;49m0x%llx\x1b[0m\n", game::local_player.address);

	game::wnd = FindWindowA(nullptr, "Roblox");

	std::thread(cache::run).detach();
	rbx::aimbot::initialize();
	std::thread(rbx::aimbot::run).detach();
	std::thread(walkspeed::run).detach();

	if (!render->create_window())
	{
		printf("\x1b[38;2;169;169;169m- \x1b[38;2;255;255;255mFailed to create \x1b[0;38;5;139;49mwindow\x1b[0m\n");
		std::this_thread::sleep_for(std::chrono::seconds(5));
		return 1;
	}

	if (!render->create_device())
	{
		printf("\x1b[38;2;169;169;169m- \x1b[38;2;255;255;255mFailed to create \x1b[0;38;5;139;49mdevice\x1b[0m\n");
		std::this_thread::sleep_for(std::chrono::seconds(5));
		return 1;
	}

	if (!render->create_imgui())
	{
		printf("\x1b[38;2;169;169;169m- \x1b[38;2;255;255;255mFailed to create \x1b[0;38;5;139;49mimgui\x1b[0m\n");
		std::this_thread::sleep_for(std::chrono::seconds(5));
		return 1;
	}

	while (true)
	{
		if (settings::settings::should_unload)
		{
			break;
		}

		render->start_render();

		render->render_visuals();

		if (render->running)
		{
			render->render_menu();
		}

		render->end_render();
	}

	return 0;
}