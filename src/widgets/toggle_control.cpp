#include "../headers/application_includes.h"

bool checkbox_ex(std::string name, bool *enabled)
{
    struct checkbox_state_t
    {
        c_vec4 text;
        c_vec4 line;
        float line_offset;
        float bg_alpha;
        float circle_offset;
    };

    c_window *window = gui->get_window();
    c_id id = window->GetID(name.data());
    checkbox_state_t *state = gui->anim_container<checkbox_state_t>(id);

    c_vec2 pos = window->DC.CursorPos;
    c_vec2 size = {gui->content_avail().x, SCALE(24)};
    c_rect rect = {pos, pos + size};
    c_rect button = {rect.GetTR() - SCALE(35, 0), rect.Max};

    gui->item_size(rect);
    gui->item_add(rect, id);

    bool hover, held, pressed = gui->button_behavior(rect, id, &hover, &held);

    if (pressed)
    {
        *enabled = !*enabled;
    }

    gui->easing(state->text, *enabled ? clr->white.Value : clr->text.Value, 16.f, dynamic_easing);
    gui->easing(state->line, *enabled ? clr->accent.Value : clr->border.Value, 16.f, dynamic_easing);
    gui->easing(state->line_offset, *enabled ? 0.f : 13.f, 16.f, dynamic_easing);
    gui->easing(state->circle_offset, *enabled ? 15.f : 0.f, 16.f, dynamic_easing);
    gui->easing(state->bg_alpha, *enabled ? 1.f : 0.f, 7.f, static_easing);

    draw->text_clipped(var->widget.draw_list, font->get(outfit_medium, 14), rect.Min, rect.Max,
                       draw->get_clr(state->text), name.data(), 0, 0, {0, 0.5});

    draw->rect_filled(var->widget.draw_list, button.Min, button.Max, draw->get_clr(clr->widget), SCALE(999));
    draw->rect_filled(var->widget.draw_list, button.Min, button.Max,
                      draw->get_clr(clr->accent, 0.15f * state->bg_alpha), SCALE(999));
    draw->rect_filled(var->widget.draw_list, button.Min + SCALE(10 + state->line_offset, 6),
                      button.Min + SCALE(12 + state->line_offset, 14),
                      draw->get_clr(state->line, 1.f - (0.5f * state->bg_alpha)), SCALE(999));
    if (!*enabled)
        draw->circle_filled(var->widget.draw_list, button.Min + SCALE(10 + state->circle_offset, 10),
                            SCALE(7), draw->get_clr(state->line), 32);
    else
        draw->circle(var->widget.draw_list, button.Min + SCALE(10 + state->circle_offset, 10), SCALE(5),
                     draw->get_clr(state->line), 32, SCALE(4));

    return pressed;
}

bool c_widgets::checkbox(std::string name, bool *value)
{
    return checkbox_ex(name, value);
}