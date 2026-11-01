#include "../runtime/vendor/api_hidden.h"
#include "../headers/application_includes.h"

static const char *GetVkName(int vk)
{
    if (vk <= 0)
        return "ไม่มี";
    if (vk == VK_HOME)
        return "Home";
    if (vk == VK_END)
        return "End";
    if (vk == VK_INSERT)
        return "Insert";
    if (vk == VK_DELETE)
        return "Delete";
    if (vk == VK_PRIOR)
        return "Page Up";
    if (vk == VK_NEXT)
        return "Page Down";
    if (vk == VK_UP)
        return "Up";
    if (vk == VK_DOWN)
        return "Down";
    if (vk == VK_LEFT)
        return "Left";
    if (vk == VK_RIGHT)
        return "Right";
    if (vk == VK_LBUTTON)
        return "Mouse 1";
    if (vk == VK_RBUTTON)
        return "Mouse 2";
    if (vk == VK_MBUTTON)
        return "Mouse 3";
    if (vk == VK_XBUTTON1)
        return "Mouse 4";
    if (vk == VK_XBUTTON2)
        return "Mouse 5";
    if (vk == VK_SHIFT || vk == VK_LSHIFT || vk == VK_RSHIFT)
        return "Shift";
    if (vk == VK_CONTROL || vk == VK_LCONTROL || vk == VK_RCONTROL)
        return "Ctrl";
    if (vk == VK_MENU || vk == VK_LMENU || vk == VK_RMENU)
        return "Alt";
    if (vk == VK_SPACE)
        return "Space";
    if (vk == VK_TAB)
        return "Tab";
    if (vk == VK_CAPITAL)
        return "Caps";
    if (vk == VK_RETURN)
        return "Enter";
    if (vk == VK_BACK)
        return "Back";
    if (vk >= 'A' && vk <= 'Z')
    {
        static char name[4] = {0};
        name[0] = (char)vk;
        name[1] = '\0';
        return name;
    }
    if (vk >= '0' && vk <= '9')
    {
        static char name[4] = {0};
        name[0] = (char)vk;
        name[1] = '\0';
        return name;
    }
    if (vk >= VK_NUMPAD0 && vk <= VK_NUMPAD9)
    {
        static char num_name[12];
        snprintf(num_name, sizeof(num_name), "Num %d", vk - VK_NUMPAD0);
        return num_name;
    }
    if (vk >= VK_F1 && vk <= VK_F12)
    {
        static char f_name[8];
        snprintf(f_name, sizeof(f_name), "F%d", vk - VK_F1 + 1);
        return f_name;
    }
    UINT scanCode = MapVirtualKeyA(vk, MAPVK_VK_TO_VSC);
    if (vk == VK_INSERT || vk == VK_DELETE || vk == VK_HOME || vk == VK_END || vk == VK_PRIOR ||
        vk == VK_NEXT || vk == VK_LEFT || vk == VK_RIGHT || vk == VK_UP || vk == VK_DOWN || vk == VK_DIVIDE ||
        vk == VK_NUMLOCK)
    {
        scanCode |= 0x100;
    }
    static char buf[64];
    if (scanCode != 0 && GetKeyNameTextA(scanCode << 16, buf, sizeof(buf)) > 0)
        return buf;
    return "Key";
}

static int DetectPressedVk()
{
    if (GetAsyncKeyState(VK_XBUTTON1) & 0x8000)
        return VK_XBUTTON1;
    if (GetAsyncKeyState(VK_XBUTTON2) & 0x8000)
        return VK_XBUTTON2;
    if (GetAsyncKeyState(VK_MBUTTON) & 0x8000)
        return VK_MBUTTON;
    if (GetAsyncKeyState(VK_RBUTTON) & 0x8000)
        return VK_RBUTTON;
    if (GetAsyncKeyState(VK_LBUTTON) & 0x8000)
        return VK_LBUTTON;
    for (int k = 1; k < 256; ++k)
    {
        if (k == VK_LBUTTON || k == VK_RBUTTON || k == VK_MBUTTON || k == VK_XBUTTON1 || k == VK_XBUTTON2)
            continue;
        if (GetAsyncKeyState(k) & 0x8000)
        {
            if (k == VK_ESCAPE)
                return 0;
            return k;
        }
    }
    return -1;
}

bool c_widgets::keybind_picker(std::string name, int *key, int *mode)
{
    c_window *window = gui->get_window();
    c_id id = window->GetID(name.data());

    struct keybind_picker_anim_t
    {
        bool binding;
        float mode_anim;
        float btn_alpha;
        float time_opened;
    };
    keybind_picker_anim_t *state = gui->anim_container<keybind_picker_anim_t>(id);

    c_vec2 pos = window->DC.CursorPos;
    float avail_w = gui->content_avail().x;
    c_vec2 size = {avail_w, SCALE(30)};
    c_rect rect = {pos, pos + size};

    gui->item_size(rect);
    gui->item_add(rect, id);

    // Label on the left
    draw->text_clipped(window->DrawList, font->get(outfit_semibold, 14), rect.Min, rect.Max,
                       draw->get_clr(clr->text), name.c_str(), 0, 0, {0.0f, 0.5f});

    // Right side: Mode toggle (Toggle / Hold) and Keybind Button
    float key_btn_w = mode ? SCALE(76) : SCALE(180);
    float mode_btn_w = mode ? SCALE(114) : 0.0f;
    float gap = mode ? SCALE(6) : 0.0f;

    c_vec2 mode_max = {rect.Max.x, rect.Max.y};
    c_vec2 mode_min = {mode_max.x - mode_btn_w, rect.Min.y};
    c_rect mode_rect = {mode_min, mode_max};

    c_vec2 key_max = {mode ? (mode_min.x - gap) : rect.Max.x, rect.Max.y};
    c_vec2 key_min = {key_max.x - key_btn_w, rect.Min.y};
    c_rect key_rect = {key_min, key_max};

    // Keybind Button Behavior
    bool key_hovered = key_rect.Contains(gui->mouse_pos());
    bool key_clicked = key_hovered && ImGui::IsMouseClicked(0);

    if (key_clicked)
    {
        state->binding = !state->binding;
        state->time_opened = (float)ImGui::GetTime();
    }
    else if (state->binding && !key_rect.Contains(gui->mouse_pos()) && ImGui::IsMouseClicked(0))
    {
        state->binding = false;
    }

    if (state->binding)
    {
        if (ImGui::GetTime() - state->time_opened > 0.15f)
        {
            int pressed_vk = DetectPressedVk();
            if (pressed_vk >= 0)
            {
                *key = pressed_vk;
                state->binding = false;
            }
        }
    }

    gui->easing(state->btn_alpha, (key_hovered || state->binding) ? 1.0f : 0.0f, 16.f, dynamic_easing);

    draw->rect_filled(window->DrawList, key_rect.Min, key_rect.Max, draw->get_clr(clr->widget), SCALE(5));
    if (state->btn_alpha > 0.01f)
    {
        draw->rect_filled(window->DrawList, key_rect.Min, key_rect.Max,
                          draw->get_clr(clr->accent, 0.20f * state->btn_alpha), SCALE(5));
    }
    draw->rect(window->DrawList, key_rect.Min, key_rect.Max,
               draw->get_clr(state->binding ? clr->accent : clr->border, 0.45f + 0.45f * state->btn_alpha),
               SCALE(5), 0, 1.0f);

    const char *key_display = state->binding ? "..." : GetVkName(*key);
    draw->text_clipped(window->DrawList, font->get(outfit_semibold, 13), key_rect.Min, key_rect.Max,
                       draw->get_clr(state->binding ? clr->accent.Value : clr->white.Value), key_display, 0,
                       0, {0.5f, 0.5f});

    if (mode)
    {
        // Mode Toggle Switch (Toggle / Hold)
        bool mode_hovered = mode_rect.Contains(gui->mouse_pos());
        if (mode_hovered && ImGui::IsMouseClicked(0))
        {
            if (gui->mouse_pos().x < mode_rect.GetCenter().x)
                *mode = 0; // Toggle
            else
                *mode = 1; // Hold
        }

        gui->easing(state->mode_anim, *mode == 0 ? 0.0f : 1.0f, 16.f, dynamic_easing);

        draw->rect_filled(window->DrawList, mode_rect.Min, mode_rect.Max, draw->get_clr(clr->widget),
                          SCALE(5));
        draw->rect(window->DrawList, mode_rect.Min, mode_rect.Max, draw->get_clr(clr->border, 0.45f),
                   SCALE(5), 0, 1.0f);

        float seg_w = (mode_rect.Max.x - mode_rect.Min.x) / 2.0f;
        c_vec2 active_min = {mode_rect.Min.x + state->mode_anim * seg_w, mode_rect.Min.y};
        c_vec2 active_max = {active_min.x + seg_w, mode_rect.Max.y};
        draw->rect_filled(window->DrawList, active_min + SCALE(2, 2), active_max - SCALE(2, 2),
                          draw->get_clr(clr->accent, 0.85f), SCALE(4));

        c_rect toggle_rect = {mode_rect.Min, {mode_rect.Min.x + seg_w, mode_rect.Max.y}};
        c_rect hold_rect = {{mode_rect.Min.x + seg_w, mode_rect.Min.y}, mode_rect.Max};

        c_vec4 toggle_txt_clr = ImLerp(clr->white.Value, clr->text.Value, state->mode_anim);
        c_vec4 hold_txt_clr = ImLerp(clr->text.Value, clr->white.Value, state->mode_anim);

        draw->text_clipped(window->DrawList, font->get(outfit_semibold, 13), toggle_rect.Min, toggle_rect.Max,
                           draw->get_clr(toggle_txt_clr), "เปิด/ปิด", 0, 0, {0.5f, 0.5f});
        draw->text_clipped(window->DrawList, font->get(outfit_semibold, 13), hold_rect.Min, hold_rect.Max,
                           draw->get_clr(hold_txt_clr), "กดค้าง", 0, 0, {0.5f, 0.5f});
    }

    return true;
}
