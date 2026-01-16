#pragma once
#include <Windows.h>
#include <algorithm>
#include <string>
#include <vector>
#include <unordered_map>

const char* const key_namess[] = { "Unknown",
                                  "LBUTTON",
                                  "RBUTTON",
                                  "CANCEL",
                                  "MBUTTON",
                                  "XBUTTON1",
                                  "XBUTTON2",
                                  "Unknown",
                                  "BACK",
                                  "TAB",
                                  "Unknown",
                                  "Unknown",
                                  "CLEAR",
                                  "RETURN",
                                  "Unknown",
                                  "Unknown",
                                  "SHIFT",
                                  "CONTROL",
                                  "MENU",
                                  "PAUSE",
                                  "CAPITAL",
                                  "KANA",
                                  "Unknown",
                                  "JUNJA",
                                  "FINAL",
                                  "KANJI",
                                  "Unknown",
                                  "ESCAPE",
                                  "CONVERT",
                                  "NONCONVERT",
                                  "ACCEPT",
                                  "MODECHANGE",
                                  "SPACE",
                                  "PRIOR",
                                  "NEXT",
                                  "END",
                                  "HOME",
                                  "LEFT",
                                  "UP",
                                  "RIGHT",
                                  "DOWN",
                                  "SELECT",
                                  "PRINT",
                                  "EXECUTE",
                                  "SNAPSHOT",
                                  "INSERT",
                                  "DELETE",
                                  "HELP",
                                  "0",
                                  "1",
                                  "2",
                                  "3",
                                  "4",
                                  "5",
                                  "6",
                                  "7",
                                  "8",
                                  "9",
                                  "Unknown",
                                  "Unknown",
                                  "Unknown",
                                  "Unknown",
                                  "Unknown",
                                  "Unknown",
                                  "Unknown",
                                  "A",
                                  "B",
                                  "C",
                                  "D",
                                  "E",
                                  "F",
                                  "G",
                                  "H",
                                  "I",
                                  "J",
                                  "K",
                                  "L",
                                  "M",
                                  "N",
                                  "O",
                                  "P",
                                  "Q",
                                  "R",
                                  "S",
                                  "T",
                                  "U",
                                  "V",
                                  "W",
                                  "X",
                                  "Y",
                                  "Z",
                                  "LWIN",
                                  "RWIN",
                                  "APPS",
                                  "Unknown",
                                  "SLEEP",
                                  "NUMPAD0",
                                  "NUMPAD1",
                                  "NUMPAD2",
                                  "NUMPAD3",
                                  "NUMPAD4",
                                  "NUMPAD5",
                                  "NUMPAD6",
                                  "NUMPAD7",
                                  "NUMPAD8",
                                  "NUMPAD9",
                                  "MULTIPLY",
                                  "ADD",
                                  "SEPARATOR",
                                  "SUBTRACT",
                                  "DECIMAL",
                                  "DIVIDE",
                                  "F1",
                                  "F2",
                                  "F3",
                                  "F4",
                                  "F5",
                                  "F6",
                                  "F7",
                                  "F8",
                                  "F9",
                                  "F10",
                                  "F11",
                                  "F12",
                                  "F13",
                                  "F14",
                                  "F15",
                                  "F16",
                                  "F17",
                                  "F18",
                                  "F19",
                                  "F20",
                                  "F21",
                                  "F22",
                                  "F23",
                                  "F24",
                                  "Unknown",
                                  "Unknown",
                                  "Unknown",
                                  "Unknown",
                                  "Unknown",
                                  "Unknown",
                                  "Unknown",
                                  "Unknown",
                                  "NUMLOCK",
                                  "SCROLL",
                                  "OEM_NEC_EQUAL",
                                  "OEM_FJ_MASSHOU",
                                  "OEM_FJ_TOUROKU",
                                  "OEM_FJ_LOYA",
                                  "OEM_FJ_ROYA",
                                  "Unknown",
                                  "Unknown",
                                  "Unknown",
                                  "Unknown",
                                  "Unknown",
                                  "Unknown",
                                  "Unknown",
                                  "Unknown",
                                  "Unknown",
                                  "LSHIFT",
                                  "RSHIFT",
                                  "LCONTROL",
                                  "RCONTROL",
                                  "LMENU",
                                  "RMENU" };


class keybind
{
private:
    bool last_key_state = false;

public:
    static enum c_keybind_type : int {
        TOGGLE,
        HOLD,
        ALWAYS
    };

    int key = 0;
    c_keybind_type type = TOGGLE;
    const char* name = "[None]";
    bool enabled = false;
    bool waiting_for_input = false;

    keybind(const char* _name)
    {
        this->name = _name;
        this->type = HOLD;
    }

    void update()
    {
        if (key == 0) {
            if (type != ALWAYS) enabled = false;
            return;
        }

        bool current_key_state = (GetAsyncKeyState(key) & 0x8000) != 0;
        bool key_just_pressed = current_key_state && !last_key_state;

        switch (type)
        {
        case TOGGLE:
            if (key_just_pressed) {
                enabled = !enabled;
            }
            break;
        case HOLD:
            enabled = current_key_state;
            break;
        case ALWAYS:
            enabled = true;
            break;
        }

        last_key_state = current_key_state;
    }

    std::string get_key_name()
    {
        if (!key)
            return "[None]";

        if (key < 0 || key >= 256)
            return "[Invalid]";

        std::string key_str = key_namess[key];
        std::string result = "[" + key_str + "]";
        std::transform(result.begin(), result.end(), result.begin(),
            [](unsigned char c) { return std::tolower(c); });
        return result;
    }

    std::string get_name()
    {
        return name;
    }

    std::string get_type()
    {
        switch (type)
        {
        case TOGGLE: return "toggle";
        case HOLD: return "hold";
        case ALWAYS: return "always";
        default: return "none";
        }
    }

    bool set_key();
};

