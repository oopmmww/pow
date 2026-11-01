#include "../headers/application_includes.h"

void c_widgets::child(std::string icon, std::string name, c_vec2 padding)
{
    gui->dummy({elements->child.width, SCALE(16)});

    draw->text_clipped(var->widget.draw_list, font->get(icons, 13), GetItemRectMin(), GetItemRectMax(),
                       draw->get_clr(clr->text), icon.data(), 0, 0, {0, 0.5});
    draw->text_clipped(var->widget.draw_list, font->get(outfit_semibold, 14), GetItemRectMin() + SCALE(24, 0),
                       GetItemRectMax(), draw->get_clr(clr->text), name.data(), 0, 0, {0, 0.5});

    gui->begin_content(name, {elements->child.width, 0}, padding.x != -1 ? padding : SCALE(10, 10),
                       padding.x != -1 ? padding : SCALE(10, 10),
                       window_flags_no_scrollbar | window_flags_no_scroll_with_mouse | window_flags_no_move,
                       child_flags_always_auto_resize | child_flags_auto_resize_y);

    draw->rect_filled(var->widget.draw_list, gui->get_window()->Rect().Min, gui->get_window()->Rect().Max,
                      draw->get_clr(clr->child, 1.0f), SCALE(elements->child.rounding));
    draw->rect(var->widget.draw_list, gui->get_window()->Rect().Min, gui->get_window()->Rect().Max,
               draw->get_clr(clr->border, 0.5f), SCALE(elements->child.rounding), 0, 1.0f);
}

void c_widgets::end_child()
{
    gui->end_content();
}

void c_widgets::weapon_preview(ImTextureID weapon_img, c_vec2 weapon_img_size, float hitbox_val)
{
    c_window *window = gui->get_window();
    c_vec2 pos = window->DC.CursorPos;
    float avail_w = gui->content_avail().x;

    float aspect = (weapon_img_size.x > 0.0f && weapon_img_size.y > 0.0f)
                       ? (weapon_img_size.y / weapon_img_size.x)
                       : 0.56f;
    float preview_h = avail_w * aspect;
    if (preview_h > SCALE(135.0f))
        preview_h = SCALE(135.0f);

    c_vec2 size = {avail_w, preview_h};
    c_rect rect = {pos, pos + size};

    gui->item_size(rect);
    gui->item_add(rect, window->GetID("##weapon_preview"));

    draw->rect_filled(window->DrawList, rect.Min, rect.Max, draw->get_clr(clr->widget), SCALE(6.0f));
    draw->rect(window->DrawList, rect.Min, rect.Max, draw->get_clr(clr->border, 0.45f), SCALE(6.0f), 0, 1.0f);

    if (weapon_img)
    {
        float pad = SCALE(10.0f);
        c_vec2 img_min = rect.Min + c_vec2(pad, pad);
        c_vec2 img_max = rect.Max - c_vec2(pad, pad);
        window->DrawList->AddImage(weapon_img, img_min, img_max);
    }

    if (hitbox_val > 0.0f)
    {
        c_vec2 center = rect.GetCenter();
        float factor = hitbox_val;
        float base_w = rect.GetWidth() - SCALE(34.0f);
        float base_h = SCALE(10.0f);

        float expand_w = factor * SCALE(16.0f);
        float expand_h = factor * SCALE(68.0f);

        c_vec2 box_size = {ImMin(base_w + expand_w, rect.GetWidth() - SCALE(10.0f)),
                           ImMin(base_h + expand_h, rect.GetHeight() - SCALE(10.0f))};

        c_vec2 box_min = center - box_size * 0.5f;
        c_vec2 box_max = center + box_size * 0.5f;

        ImU32 box_color = draw->get_clr(clr->white, 0.90f);
        draw->rect_filled(window->DrawList, box_min, box_max, draw->get_clr(clr->white, 0.08f), SCALE(2.0f));
        draw->rect(window->DrawList, box_min, box_max, box_color, SCALE(2.0f), 0, 1.5f);
    }
}