#pragma once
#include <sdk/sdk.h>
#include <Windows.h>

namespace game
{
	inline rbx::instance_t datamodel{};
	inline rbx::visualengine_t visengine{};
	inline rbx::instance_t workspace{};
	inline rbx::instance_t players{};
	inline rbx::instance_t local_player{};
	inline rbx::instance_t local_character{};
	inline HWND wnd;
}