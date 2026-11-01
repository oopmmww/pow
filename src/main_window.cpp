#include "headers/application_includes.h"
#include "headers/window_lifecycle.h"
#include "runtime/feature_controller.h"
#include "runtime/head_protection.h"
#include <thread>
#include "head_visualization.h"

static float custom_f14 = 0.45f;
static float custom_area = 0.15f;
static bool rbp38_enabled = false;
static int keybind_vk = 0;
static int keybind_mode = 0;
static bool keybind_active = false;
static bool prev_key_down = false;
static std::string status_msg = "Ready";
static bool status_success = true;
static int prev_keybind_vk = 0;
static int prev_keybind_mode = 0;
static int head_keybind_vk = 0;
static int head_keybind_mode = 0;
static bool head_keybind_active = false;
static bool head_prev_key_down = false;
static int head_prev_keybind_vk = 0;
static int head_prev_keybind_mode = 0;

static void apply_current_values()
{
    status_success = Strike_ApplyCustom(keybind_active ? 1.22f : custom_f14, status_msg,
                                        keybind_active ? .80f : custom_area);
}
static bool set_hitbox_override(bool enabled)
{
    status_success =
        Strike_ApplyCustom(enabled ? 1.22f : custom_f14, status_msg, enabled ? .80f : custom_area);
    if (status_success)
        keybind_active = enabled;
    return status_success;
}
static void PlayBeepSound(DWORD freq, DWORD duration)
{
    std::thread([freq, duration]() { Beep(freq, duration); }).detach();
}

void c_gui::update_hotkeys()
{
    if (keybind_vk != prev_keybind_vk || keybind_mode != prev_keybind_mode)
    {
        // Rebinding, clearing or changing mode ends a temporary override.
        if (keybind_active)
            set_hitbox_override(false);
        prev_keybind_vk = keybind_vk;
        prev_keybind_mode = keybind_mode;
        // Ignore the key press that was used to capture the binding.
        prev_key_down = keybind_vk > 0 && (GetAsyncKeyState(keybind_vk) & 0x8000) != 0;
    }

    if (keybind_vk > 0)
    {
        const bool is_down = (GetAsyncKeyState(keybind_vk) & 0x8000) != 0;
        if (keybind_mode == 0 && is_down && !prev_key_down)
        {
            const bool enable = !keybind_active;
            if (set_hitbox_override(enable))
                PlayBeepSound(enable ? 1300 : 450, 100);
        }
        else if (keybind_mode == 1)
        {
            if (is_down && !prev_key_down)
                set_hitbox_override(true);
            else if (!is_down && prev_key_down && keybind_active)
                set_hitbox_override(false);
        }
        prev_key_down = is_down;
    }
    else
    {
        prev_key_down = false;
    }
    static DWORD last_hard_head_tick = 0;
    DWORD now = GetTickCount();
    if (now - last_hard_head_tick >= 50)
    {
        last_hard_head_tick = now;
        Strike_HardHead_Tick();
    }

    if (head_keybind_vk != head_prev_keybind_vk || head_keybind_mode != head_prev_keybind_mode)
    {
        head_keybind_active = false;
        Strike_HardHead_SetActive(false);
        head_prev_keybind_vk = head_keybind_vk;
        head_prev_keybind_mode = head_keybind_mode;
    }

    if (head_keybind_vk > 0 && !ac_vars::IsConfigured())
    {
        head_keybind_active = false;
        status_msg = "Hard Head configuration incomplete";
        status_success = false;
    }
    else if (head_keybind_vk > 0)
    {
        bool is_down = (GetAsyncKeyState(head_keybind_vk) & 0x8000) != 0;
        if (head_keybind_mode == 0)
        {
            if (is_down && !head_prev_key_down)
            {
                head_keybind_active = !head_keybind_active;
                Strike_HardHead_SetActive(head_keybind_active);
                status_msg = head_keybind_active ? "Hard Head enabled" : "Hard Head disabled";
                status_success = true;
            }
        }
        else if (head_keybind_mode == 1)
        {
            if (is_down && !head_prev_key_down)
            {
                head_keybind_active = true;
                Strike_HardHead_SetActive(true);
                status_msg = "Hard Head enabled (hold)";
                status_success = true;
            }
            else if (!is_down && head_prev_key_down)
            {
                head_keybind_active = false;
                Strike_HardHead_SetActive(false);
                status_msg = "Hard Head disabled";
                status_success = true;
            }
        }
        head_prev_key_down = is_down;
    }
}

void c_gui::render()
{
    update_hotkeys();
    gui->initialize();
    gui->set_next_window_pos(ImVec2(0, 0));
    gui->set_next_window_size(SCALE(elements->window.size));
    gui->begin(elements->window.name, nullptr,
               window_flags_no_scrollbar | window_flags_no_scroll_with_mouse |
                   window_flags_no_bring_to_front_on_focus | window_flags_no_focus_on_appearing |
                   window_flags_no_background | window_flags_no_decoration);
    gui->set_style();
    ImDrawList *dl = ImGui::GetWindowDrawList();
    const ImVec2 origin = ImGui::GetWindowPos();
    auto pos = [&](float x, float y) { return ImVec2(origin.x + SCALE(x), origin.y + SCALE(y)); };
    auto at = [&](float x, float y) { ImGui::SetCursorScreenPos(pos(x, y)); };
    const ImU32 border = IM_COL32(64, 72, 80, 210);
    dl->AddRectFilled(pos(0, 0), pos(430, 720), IM_COL32(16, 21, 25, 255), SCALE(12));
    dl->AddRectFilled(pos(1, 1), pos(429, 49), IM_COL32(27, 33, 39, 255), SCALE(12),
                      ImDrawFlags_RoundCornersTop);
    dl->AddRect(pos(1, 1), pos(429, 719), border, SCALE(12));
    auto separator = [&](float y) { dl->AddLine(pos(20, y), pos(410, y), border); };
    dl->AddLine(pos(0, 50), pos(430, 50), border);
    ImGui::PushFont(font->get(outfit_semibold, 15));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, SCALE(7));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, SCALE(8, 7));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(.10f, .125f, .15f, 1));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(.14f, .21f, .19f, 1));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(.08f, .36f, .23f, 1));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(.25f, .29f, .33f, .8f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(.91f, .94f, .96f, 1));
    auto button = [&](const char *label, float x, float y, float width, float height, bool selected = false)
    {
        at(x, y);
        if (selected)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(.02f, .94f, .46f, 1));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(.025f, .08f, .055f, 1));
        }
        bool clicked = ImGui::Button(label, SCALE(width, height));
        if (selected)
            ImGui::PopStyleColor(2);
        return clicked;
    };
    // All key pickers retain the same VK storage used by the existing engine.
    static int binding = -1;
    static double bindingStarted = 0;
    static bool mouseBindingArmed = false;
    auto keyButton = [&](int id, int *key, float x, float y, float width, bool eye = false)
    {
        char name[64] = "None";
        if (*key == VK_HOME)
            strcpy_s(name, "Home");
        else if (*key == VK_LBUTTON)
            strcpy_s(name, "Mouse 1");
        else if (*key == VK_RBUTTON)
            strcpy_s(name, "Mouse 2");
        else if (*key == VK_MBUTTON)
            strcpy_s(name, "Mouse 3");
        else if (*key == VK_XBUTTON1)
            strcpy_s(name, "Mouse 4");
        else if (*key == VK_XBUTTON2)
            strcpy_s(name, "Mouse 5");
        else if (*key > 0)
        {
            UINT scan = MapVirtualKeyA(*key, MAPVK_VK_TO_VSC);
            if (*key == VK_INSERT || *key == VK_DELETE || *key == VK_END || *key == VK_PRIOR ||
                *key == VK_NEXT || (*key >= VK_LEFT && *key <= VK_DOWN))
                scan |= 0x100;
            if (!GetKeyNameTextA(scan << 16, name, sizeof(name)))
                snprintf(name, sizeof(name), "Key %d", *key);
        }
        ImGui::PushID(id);
        const char *shown = binding == id ? "Press key" : name;
        if (button(shown, x, y, width, 32) && binding != id)
        {
            binding = id;
            bindingStarted = ImGui::GetTime();
            mouseBindingArmed = false;
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip(eye ? "Bind a keyboard key or Mouse 1-5 to show / hide UI."
                                  : "Click and release, then press a keyboard key or Mouse 1-5. Escape "
                                    "cancels, Delete clears.");
        ImGui::PopID();
        if (binding == id)
        {
            const int mouseButtons[] = {VK_LBUTTON, VK_RBUTTON, VK_MBUTTON, VK_XBUTTON1, VK_XBUTTON2};
            int pressedMouse = 0;
            for (int vk : mouseButtons)
                if (GetAsyncKeyState(vk) & 0x8000)
                {
                    pressedMouse = vk;
                    break;
                }
            // Require release of the opening click before accepting a new press.
            if (!pressedMouse)
                mouseBindingArmed = true;
            if (ImGui::GetTime() - bindingStarted <= .2)
                return;
            if (mouseBindingArmed && pressedMouse)
            {
                *key = pressedMouse;
                binding = -1;
                return;
            }
            for (int vk = 1; vk < 256; ++vk)
            {
                if (vk == VK_LBUTTON || vk == VK_RBUTTON || vk == VK_MBUTTON || vk == VK_XBUTTON1 ||
                    vk == VK_XBUTTON2)
                    continue;
                if (GetAsyncKeyState(vk) & 0x8000)
                {
                    if (vk != VK_ESCAPE)
                    {
                        if (vk == VK_DELETE)
                        {
                            if (!eye)
                                *key = 0;
                        }
                        else
                            *key = vk;
                    }
                    binding = -1;
                    break;
                }
            }
        }
    };
    at(20, 15);
    ImGui::PushFont(font->get(outfit_semibold, 18));
    ImGui::TextUnformatted("FREE FOR YOU");
    ImGui::PopFont();
    static int menu_keybind_vk = VK_HOME;
    keyButton(10, &menu_keybind_vk, 282, 9, 89, true);
    Strike_SetMenuToggleVk(menu_keybind_vk);
    // Small eye glyph in the header shortcut pill.
    ImVec2 eye = pos(294, 25);
    dl->AddEllipse(eye, SCALE(6, 3.5f), IM_COL32(210, 222, 228, 255), 0, 20, 1.f);
    dl->AddCircleFilled(eye, SCALE(1.5f), IM_COL32(210, 222, 228, 255), 16);
    at(384, 9);
    if (ImGui::InvisibleButton("##close", SCALE(30, 32)))
        PostMessageW(var->winapi.hwnd, WM_CLOSE, 0, 0);
    dl->AddCircle(pos(399, 25), SCALE(14), border, 32);
    dl->AddLine(pos(395, 21), pos(403, 29), IM_COL32(220, 230, 235, 255), 1.5f);
    dl->AddLine(pos(403, 21), pos(395, 29), IM_COL32(220, 230, 235, 255), 1.5f);

    DrawHeadPreview(dl, pos(24, 62), pos(406, 361), keybind_active ? .80f : custom_area,
                    keybind_active ? 1.22f : custom_f14, 0, 1.5f);

    auto slider = [&](const char *label, float y, float *value, float low, float high)
    {
        dl->AddText(pos(20, y), IM_COL32(230, 236, 241, 255), label);
        float cy = y + 29;
        at(20, cy - 13);
        ImGui::InvisibleButton(label, SCALE(310, 26));
        bool changed = false;
        if (ImGui::IsItemActive() && ImGui::IsMouseDown(0))
        {
            const float fraction = ImClamp((ImGui::GetIO().MousePos.x - pos(20, 0).x) / SCALE(310), 0.f, 1.f);
            const int level = 1 + int(fraction * 99.f + .5f);
            float next = low + ((level - 1) / 99.f) * (high - low);
            changed = next != *value;
            *value = next;
        }
        float x = 20 + 310 * (*value - low) / (high - low);
        dl->AddLine(pos(20, cy), pos(330, cy), IM_COL32(38, 45, 54, 255), SCALE(8));
        dl->AddLine(pos(20, cy), pos(x, cy), IM_COL32(14, 239, 131, 255), SCALE(8));
        dl->AddCircleFilled(pos(x, cy), SCALE(9), IM_COL32(239, 255, 246, 255), 32);
        dl->AddRectFilled(pos(348, cy - 18), pos(410, cy + 18), IM_COL32(26, 32, 39, 255), SCALE(7));
        dl->AddRect(pos(348, cy - 18), pos(410, cy + 18), border, SCALE(7));
        const int displayLevel = 1 + int(ImClamp((*value - low) / (high - low), 0.f, 1.f) * 99.f + .5f);
        char text[24];
        snprintf(text, sizeof(text), "%d", displayLevel);
        ImVec2 ts = ImGui::CalcTextSize(text);
        dl->AddText({pos(379, cy).x - ts.x * .5f, pos(379, cy).y - ts.y * .5f}, IM_COL32(230, 239, 244, 255),
                    text);
        return changed;
    };
    slider("Size", 378, &custom_f14, .10f, 1.22f);
    slider("AREA", 440, &custom_area, .01f, .80f);
    separator(500);
    dl->AddText(pos(20, 520), IM_COL32(230, 239, 244, 255), "Prop");
    at(354, 514);
    if (ImGui::InvisibleButton("##prop", SCALE(56, 30)))
    {
        rbp38_enabled = !rbp38_enabled;
        status_success = Strike_SetRbp38(rbp38_enabled, status_msg);
    }
    dl->AddRectFilled(pos(354, 514), pos(410, 544),
                      rbp38_enabled ? IM_COL32(13, 209, 113, 255) : IM_COL32(35, 42, 51, 255), SCALE(15));
    dl->AddCircleFilled(pos(rbp38_enabled ? 395 : 369, 529), SCALE(11), IM_COL32(147, 160, 177, 255), 32);
    separator(558);
    auto hotkeyRow = [&](const char *label, int id, int *key, int *mode, float y)
    {
        dl->AddText(pos(20, y + 8), IM_COL32(230, 239, 244, 255), label);
        keyButton(id, key, 134, y, 103);
        ImGui::PushID(id + 100);
        if (button("Toggle", 245, y, 83, 32, *mode == 0))
            *mode = 0;
        if (button("Hold", 336, y, 74, 32, *mode == 1))
            *mode = 1;
        ImGui::PopID();
    };
    hotkeyRow("Hitbox", 11, &keybind_vk, &keybind_mode, 571);
    hotkeyRow("Hard Head", 12, &head_keybind_vk, &head_keybind_mode, 614);
    dl->AddLine(pos(0, 660), pos(430, 660), border);
    if (button("Reset", 20, 673, 76, 34))
    {
        custom_f14 = .45f;
        custom_area = .15f;
        rbp38_enabled = false;
        head_keybind_active = false;
        keybind_active = false;
        Strike_HardHead_SetActive(false);
        status_success = Strike_Reset(status_msg);
    }
    if (button("Clear", 106, 673, 76, 34))
    {
        // Stop shortcut activation before restoring, then close only on success.
        keybind_active = false;
        head_keybind_active = false;
        status_success = RestoreGameAndClose(var->winapi.hwnd, status_msg);
        if (status_success)
        {
            rbp38_enabled = false;
        }
        else
        {
            ImGui::OpenPopup("Restore failed");
        }
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Restore game changes, then close the program.");
    if (ImGui::BeginPopupModal("Restore failed", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextUnformatted("Reset and close could not finish. The program remains open.");
        ImGui::TextWrapped("%s", status_msg.c_str());
        if (ImGui::Button("OK"))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    if (button("Apply", 296, 673, 114, 34, true))
        apply_current_values();
    if (ImGui::IsItemHovered() && !status_msg.empty())
        ImGui::SetTooltip("%s", status_msg.c_str());
    ImGui::PopStyleColor(5);
    ImGui::PopStyleVar(3);
    ImGui::PopFont();
    widgets->blackout();
    gui->end();
}
