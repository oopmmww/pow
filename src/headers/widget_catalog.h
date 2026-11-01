#pragma once
#include "application_includes.h"
#include <unordered_map>

#define IMGUI_DEFINE_MATH_OPERATORS

inline std::string const keys[] = {"Tab",
                                   "Left",
                                   "Right",
                                   "Up",
                                   "Down",
                                   "Page Up",
                                   "Page Down",
                                   "Home",
                                   "End",
                                   "Insert",
                                   "Delete",
                                   "Backspace",
                                   "Space",
                                   "Enter",
                                   "Escape",
                                   "Ctrl",
                                   "Shift",
                                   "Alt",
                                   "Super",
                                   "Ctrl",
                                   "Shift",
                                   "Alt",
                                   "Super",
                                   "Menu",
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
                                   "Apostrophe",
                                   "Comma",
                                   "Minus",
                                   "Period",
                                   "Slash",
                                   "Semicolon",
                                   "Equal",
                                   "Left Bracket",
                                   "Backslash",
                                   "Right Bracket",
                                   "Grave Accent",
                                   "Caps Lock",
                                   "Scroll Lock",
                                   "Num Lock",
                                   "Print Screen",
                                   "Pause",
                                   "Keypad 0",
                                   "Keypad 1",
                                   "Keypad 2",
                                   "Keypad 3",
                                   "Keypad 4",
                                   "Keypad 5",
                                   "Keypad 6",
                                   "Keypad 7",
                                   "Keypad 8",
                                   "Keypad 9",
                                   "Keypad .",
                                   "Keypad /",
                                   "Keypad *",
                                   "Keypad -",
                                   "Keypad +",
                                   "Keypad Enter",
                                   "Keypad =",
                                   "App Back",
                                   "App Forward",
                                   "Gamepad Start",
                                   "Gamepad Back",
                                   "Gamepad Face Left",
                                   "Gamepad Face Right",
                                   "Gamepad Face Up",
                                   "Gamepad Face Down",
                                   "Gamepad Dpad Left",
                                   "Gamepad Dpad Right",
                                   "Gamepad Dpad Up",
                                   "Gamepad Dpad Down",
                                   "Gamepad L1",
                                   "Gamepad R1",
                                   "Gamepad L2",
                                   "Gamepad R2",
                                   "Gamepad L3",
                                   "Gamepad R3",
                                   "Gamepad L Stick Left",
                                   "Gamepad L Stick Right",
                                   "Gamepad L Stick Up",
                                   "Gamepad L Stick Down",
                                   "Gamepad R Stick Left",
                                   "Gamepad R Stick Right",
                                   "Gamepad R Stick Up",
                                   "Gamepad R Stick Down",
                                   "Mouse 1",
                                   "Mouse 2",
                                   "Mouse 3",
                                   "Mouse 4",
                                   "Mouse 5",
                                   "Mouse Wheel X",
                                   "Mouse Wheel Y",
                                   "Ctrl",
                                   "Shift",
                                   "Alt",
                                   "Super"};

inline std::string get_key_name(ImGuiKey key)
{
    if (key == ImGuiKey_None)
        return "None";

    return keys[key - ImGuiKey_NamedKey_BEGIN];
}

inline std::string utf8_safe_truncate(const std::string &str, size_t max_chars)
{
    size_t i = 0;
    size_t char_count = 0;

    while (i < str.size() && char_count < max_chars)
    {
        unsigned char c = static_cast<unsigned char>(str[i]);

        size_t char_size = 1;

        if ((c & 0x80) == 0x00)
            char_size = 1; // ASCII
        else if ((c & 0xE0) == 0xC0)
            char_size = 2; // 2 байта
        else if ((c & 0xF0) == 0xE0)
            char_size = 3; // 3 байта
        else if ((c & 0xF8) == 0xF0)
            char_size = 4; // 4 байта

        if (i + char_size > str.size())
            break;

        i += char_size;
        char_count++;
    }

    std::string result = str.substr(0, i);

    if (i < str.size())
        result += "...";

    return result;
}

inline bool need_sep()
{
    return false;
}

class c_widgets
{
  public:
    std::unordered_map<std::string, bool> popup_states;

    void open_popup(std::string name);
    void close_popup(std::string name);
    bool is_popup_open(std::string name);
    bool popup(std::string name, c_vec2 size, c_vec2 pos = {0, 0}, c_vec2 padding = {0, 0},
               c_vec2 spacing = {0, 0}, float rounding = 0.f, bool always_on_screen = false,
               window_flags flags = 0);
    void end_popup();
    void seperator();

    void load_texture(std::string path, ImTextureID *img);
    void load_texture_from_memory(const unsigned char *bytes, size_t size, ImTextureID *img);
    void child(std::string icon, std::string name, c_vec2 padding = {-1, -1});
    void end_child();
    bool checkbox(std::string name, bool *value);
    bool slider(std::string name, float *value, float min = 0.f, float max = 100.f,
                std::string format = "%.1f");
    bool dropdown(std::string name, int *current_item, const std::vector<std::string> &items);
    bool selectable(std::string name, bool is_selected);
    bool keybind_picker(std::string name, int *key, int *mode = nullptr);
    void blackout();
    void weapon_preview(ImTextureID weapon_img, c_vec2 weapon_img_size, float hitbox_val = 0.f);
};

inline std::unique_ptr<c_widgets> widgets = std::make_unique<c_widgets>();
