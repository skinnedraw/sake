#pragma once
#include <sdk/sdk.h>
#include <Windows.h>
#include <winhttp.h>
#include <string>

#pragma comment(lib, "winhttp.lib")

namespace game
{
	inline rbx::instance_t datamodel{};
	inline rbx::visualengine_t visengine{};
	inline rbx::instance_t workspace{};
	inline rbx::instance_t players{};
	inline rbx::instance_t local_player{};
	inline rbx::instance_t local_character{};
	inline HWND wnd{};

	inline std::string http_get(const std::wstring& host, const std::wstring& path)
	{
		HINTERNET session = WinHttpOpen(L"rbx",
			WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
			WINHTTP_NO_PROXY_NAME,
			WINHTTP_NO_PROXY_BYPASS,
			0);

		if (!session)
			return {};

		HINTERNET connect = WinHttpConnect(session, host.c_str(),
			INTERNET_DEFAULT_HTTPS_PORT, 0);

		if (!connect)
		{
			WinHttpCloseHandle(session);
			return {};
		}

		HINTERNET request = WinHttpOpenRequest(
			connect,
			L"GET",
			path.c_str(),
			nullptr,
			WINHTTP_NO_REFERER,
			WINHTTP_DEFAULT_ACCEPT_TYPES,
			WINHTTP_FLAG_SECURE
		);

		if (!request)
		{
			WinHttpCloseHandle(connect);
			WinHttpCloseHandle(session);
			return {};
		}

		std::string response;

		if (WinHttpSendRequest(request,
			WINHTTP_NO_ADDITIONAL_HEADERS,
			0,
			WINHTTP_NO_REQUEST_DATA,
			0,
			0,
			0))
		{
			WinHttpReceiveResponse(request, nullptr);

			DWORD size = 0;
			while (WinHttpQueryDataAvailable(request, &size) && size)
			{
				std::string buffer(size, 0);
				DWORD read = 0;

				WinHttpReadData(request, buffer.data(), size, &read);
				buffer.resize(read);
				response += buffer;
			}
		}

		WinHttpCloseHandle(request);
		WinHttpCloseHandle(connect);
		WinHttpCloseHandle(session);

		return response;
	}

	inline std::uint64_t get_universe_id(std::uint64_t place_id)
	{
		auto json = http_get(
			L"apis.roblox.com",
			L"/universes/v1/places/" + std::to_wstring(place_id) + L"/universe"
		);

		auto pos = json.find("\"universeId\":");
		if (pos == std::string::npos)
			return 0;

		return std::stoull(json.substr(pos + 13));
	}

	inline std::string get_game_name(std::uint64_t universe_id)
	{
		auto json = http_get(
			L"games.roblox.com",
			L"/v1/games?universeIds=" + std::to_wstring(universe_id)
		);

		auto pos = json.find("\"name\":\"");
		if (pos == std::string::npos)
			return "Unknown";

		pos += 8;
		auto end = json.find("\"", pos);
		return json.substr(pos, end - pos);
	}

	inline void detect_game()
	{
		std::uint64_t place_id =
			memory->read<std::uint64_t>(datamodel.address + Offsets::DataModel::PlaceId);

		auto universe_id = get_universe_id(place_id);
		auto game_name = universe_id ? get_game_name(universe_id) : "Unknown";

		printf(
			"\x1b[38;2;169;169;169m- "
			"\x1b[38;2;255;255;255mDetected game: "
			"\x1b[0;38;5;139;49m%s "
			"\x1b[38;2;169;169;169m(PlaceId: %llu)\x1b[0m\n",
			game_name.c_str(),
			place_id
		);
	}
}
