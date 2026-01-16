#include "keybind.h"
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

bool keybind::set_key()
{
    if (ImGui::IsKeyChordPressed(ImGuiKey_Escape))
    {
        key = 0;
        ImGui::ClearActiveID();
        return true;
    }

    for (int i = 1; i < 5; i++)
    {
        if (ImGui::GetIO().MouseDown[i])
        {
            switch (i)
            {
            case 1: key = VK_RBUTTON; break;
            case 2: key = VK_MBUTTON; break;
            case 3: key = VK_XBUTTON1; break;
            case 4: key = VK_XBUTTON2; break;
            }
            return true;
        }
    }

    for (int i = VK_BACK; i <= VK_RMENU; i++)
    {
        if (i == VK_LBUTTON) continue;

        if (GetAsyncKeyState(i) & 0x8000)
        {
            key = i;
            return true;
        }
    }

    std::vector<int> additional_keys = {
        VK_LWIN, VK_RWIN, VK_APPS, VK_SLEEP, VK_NUMPAD0, VK_NUMPAD1,
        VK_NUMPAD2, VK_NUMPAD3, VK_NUMPAD4, VK_NUMPAD5, VK_NUMPAD6,
        VK_NUMPAD7, VK_NUMPAD8, VK_NUMPAD9, VK_MULTIPLY, VK_ADD,
        VK_SEPARATOR, VK_SUBTRACT, VK_DECIMAL, VK_DIVIDE, VK_F1, VK_F2,
        VK_F3, VK_F4, VK_F5, VK_F6, VK_F7, VK_F8, VK_F9, VK_F10,
        VK_F11, VK_F12, VK_F13, VK_F14, VK_F15, VK_F16, VK_F17, VK_F18,
        VK_F19, VK_F20, VK_F21, VK_F22, VK_F23, VK_F24
    };

    for (int vk : additional_keys)
    {
        if (GetAsyncKeyState(vk) & 0x8000)
        {
            key = vk;
            return true;
        }
    }

    return false;
}

