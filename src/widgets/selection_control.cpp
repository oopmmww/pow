#include "../headers/application_includes.h"

struct selectable_state_t
{
    c_vec4 text;
    float alpha;
    float hover_alpha;
};

bool selectable_ex(std::string name, bool is_selected)
{
    c_window *window = gui->get_window();
    c_id id = window->GetID(name.data());
    selectable_state_t *state = gui->anim_container<selectable_state_t>(id);

    c_vec2 pos = window->DC.CursorPos;
    c_vec2 size = {gui->content_avail().x, SCALE(30)};
    c_rect rect = {pos, pos + size};

    gui->item_size(rect);
    gui->item_add(rect, id);

    bool hovered = rect.Contains(gui->mouse_pos());
    bool pressed = hovered && ImGui::IsMouseClicked(0);

    gui->easing(state->hover_alpha, hovered ? 1.0f : 0.0f, 16.f, dynamic_easing);
    gui->easing(state->alpha, is_selected ? 1.02f : -0.02f, 16.f, dynamic_easing);
    state->alpha = ImClamp(state->alpha, 0.f, 1.f);
    state->hover_alpha = ImClamp(state->hover_alpha, 0.f, 1.f);
    gui->easing(state->text, is_selected ? clr->white.Value : clr->text.Value, 16.f, dynamic_easing);

    float bg_alpha = ImMax(state->alpha, 0.35f * state->hover_alpha);
    draw->rect_filled(window->DrawList, rect.Min, rect.Max, draw->get_clr(clr->popup_widget, bg_alpha),
                      SCALE(5));

    draw->text_clipped(window->DrawList, font->get(outfit_medium, 14), rect.Min + SCALE(8, 0), rect.Max,
                       draw->get_clr(state->text), name.data(), 0, 0, {0, 0.5});

    draw->text_clipped(window->DrawList, font->get(icons, 13), rect.Min,
                       rect.Max - SCALE(8 * state->alpha, 0), draw->get_clr(clr->white, state->alpha), "T", 0,
                       0, {1, 0.5});

    return pressed;
}

bool c_widgets::selectable(std::string name, bool is_selected)
{
    return selectable_ex(name, is_selected);
}

struct dropdown_state_t
{
    c_vec4 text;
    float anim_offset;
};

bool dropdown_ex(std::string name, std::string preview)
{
    c_window *window = gui->get_window();
    c_id id = window->GetID(name.data());
    dropdown_state_t *state = gui->anim_container<dropdown_state_t>(id);

    c_vec2 pos = window->DC.CursorPos;
    c_vec2 size = {gui->content_avail().x, SCALE(52)};
    c_rect rect = {pos, pos + size};
    c_rect button = {rect.GetBL() - SCALE(0, 32), rect.Max};

    gui->item_size(rect);
    gui->item_add(rect, id);

    bool btn_hovered = button.Contains(gui->mouse_pos());
    bool btn_clicked = btn_hovered && ImGui::IsMouseClicked(0);

    std::string popup_name = name + "##popup";

    if (btn_clicked)
    {
        if (widgets->is_popup_open(popup_name))
            widgets->close_popup(popup_name);
        else
            widgets->open_popup(popup_name);
    }

    bool is_active = widgets->is_popup_open(popup_name);

    if (is_active && ImGui::IsMouseClicked(0) && !btn_hovered)
    {
        c_window *popup_win = ImGui::FindWindowByName(popup_name.c_str());
        if (popup_win)
        {
            if (!popup_win->Rect().Contains(gui->mouse_pos()))
            {
                widgets->close_popup(popup_name);
                is_active = false;
            }
        }
    }

    gui->easing(state->text, is_active ? clr->white.Value : clr->text.Value, 16.f, dynamic_easing);
    gui->easing(state->anim_offset, is_active ? 1.02f : -0.02f, 16.f, dynamic_easing);
    state->anim_offset = ImClamp(state->anim_offset, 0.f, 1.f);

    draw->text_clipped(var->widget.draw_list, font->get(outfit_medium, 14), rect.Min - SCALE(0, 2), rect.Max,
                       draw->get_clr(state->text), name.data(), 0, 0, {0, 0});

    draw->rect_filled(var->widget.draw_list, button.Min, button.Max, draw->get_clr(clr->widget), SCALE(7));

    preview = utf8_safe_truncate(preview, 50);
    draw->text_clipped(var->widget.draw_list, font->get(outfit_medium, 14), button.Min + SCALE(10, 0),
                       button.Max, draw->get_clr(clr->white), preview.data(), 0, 0, {0, 0.5});

    c_rect arrow = {button.GetTR() - SCALE(32, 0), button.Max};

    draw->rotate_start(var->widget.draw_list);
    draw->text_clipped(var->widget.draw_list, font->get(icons, 13), arrow.Min, arrow.Max,
                       draw->get_clr(clr->white), "N", 0, 0, {0.5, 0.5});
    draw->rotate_end(var->widget.draw_list, 180.f * state->anim_offset, arrow.GetCenter());

    if (need_sep())
    {
        widgets->seperator();
    }

    if (widgets->popup(popup_name, {button.GetWidth(), 0}, button.GetBL() + SCALE(0, 4), SCALE(6, 6),
                       SCALE(6, 6), SCALE(7)))
    {
        return true;
    }
    return false;
}

bool c_widgets::dropdown(std::string name, int *current_item, const std::vector<std::string> &items)
{
    if (items.empty() || !current_item)
        return false;

    if (*current_item < 0 || *current_item >= (int)items.size())
        *current_item = 0;

    bool value_changed = false;

    if (dropdown_ex(name, items[*current_item]))
    {
        for (int i = 0; i < (int)items.size(); i++)
        {
            if (widgets->selectable(items[i], *current_item == i))
            {
                *current_item = i;
                value_changed = true;
            }
        }

        widgets->end_popup();
    }

    if (value_changed)
    {
        widgets->close_popup(name + "##popup");
    }

    return value_changed;
}

void c_widgets::open_popup(std::string name)
{
    popup_states[name] = true;
}

void c_widgets::close_popup(std::string name)
{
    popup_states[name] = false;
}

bool c_widgets::is_popup_open(std::string name)
{
    auto it = popup_states.find(name);
    return it != popup_states.end() && it->second;
}

bool c_widgets::popup(std::string name, c_vec2 size, c_vec2 pos, c_vec2 padding, c_vec2 spacing,
                      float rounding, bool always_on_screen, window_flags flags)
{
    if (!is_popup_open(name))
        return false;

    gui->set_next_window_pos(pos);
    gui->set_next_window_size(size);
    gui->push_var(style_var_window_padding, padding);
    gui->push_var(style_var_item_spacing, spacing);
    gui->push_var(style_var_window_rounding, rounding);

    window_flags wflags = window_flags_no_title_bar | window_flags_no_resize | window_flags_no_move |
                          window_flags_no_scrollbar | window_flags_no_collapse |
                          window_flags_always_auto_resize;
    bool opened = gui->begin(name, nullptr, wflags);

    c_window *window = gui->get_window();
    ImGui::BringWindowToDisplayFront(window);

    draw->rect_filled(window->DrawList, window->Pos, window->Pos + window->Size, draw->get_clr(clr->popup),
                      rounding);
    draw->rect(window->DrawList, window->Pos, window->Pos + window->Size, draw->get_clr(clr->border),
               rounding, 0, 1.0f);

    return opened;
}

void c_widgets::end_popup()
{
    c_window *window = gui->get_window();
    ImGui::BringWindowToDisplayFront(window);
    gui->end();
    gui->pop_var(3);
}

void c_widgets::seperator() {}