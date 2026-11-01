#include "../headers/application_includes.h"
#include "../../thirdparty/imgui/imgui_freetype.h"
#include "imgui_impl_dx11.h"

void c_font::update()
{
    if (var->gui.dpi_changed)
    {
        var->gui.dpi = var->gui.stored_dpi / 100.f;

        ImFontConfig cfg;
        cfg.FontBuilderFlags = ImGuiFreeTypeBuilderFlags_ForceAutoHint;
        cfg.FontDataOwnedByAtlas = false;

        ImGuiIO &io = ImGui::GetIO();
        io.Fonts->Clear();

        static const ImWchar glyph_ranges_latin_cyrillic[] = {
            0x0020, 0x00FF, // Basic Latin + Latin Supplement
            0x0400, 0x052F, // Cyrillic + Cyrillic Supplement
            0x2DE0, 0x2DFF, // Cyrillic Extended-A
            0xA640, 0xA69F, // Cyrillic Extended-B
            0,
        };

        static const ImWchar glyph_ranges_thai[] = {
            0x0E00,
            0x0E7F, // Thai Unicode range
            0,
        };

        for (auto &font_t : data)
        {
            font_t.font =
                io.Fonts->AddFontFromMemoryTTF((void *)font_t.data.data, (int)font_t.data.size,
                                               SCALE(font_t.size), &cfg, glyph_ranges_latin_cyrillic);

            if (lineseed_th.data && font_t.data.data != icons.data)
            {
                ImFontConfig cfg_thai;
                cfg_thai.MergeMode = true;
                cfg_thai.FontBuilderFlags = ImGuiFreeTypeBuilderFlags_ForceAutoHint;
                cfg_thai.FontDataOwnedByAtlas = false;
                cfg_thai.GlyphOffset.y = -2.0f;
                io.Fonts->AddFontFromMemoryTTF((void *)lineseed_th.data, (int)lineseed_th.size,
                                               SCALE(font_t.size), &cfg_thai, glyph_ranges_thai);
            }
        }

        io.Fonts->Build();
        ImGui_ImplDX11_CreateDeviceObjects();

        var->gui.dpi_changed = false;
    }
}

ImFont *c_font::get(const font_blob &font_data, float size)
{
    for (auto &font_item : data)
    {
        if (font_item.data == font_data && font_item.size == size)
        {
            if (font_item.font)
                return font_item.font;
            if (ImGui::GetCurrentContext())
                return ImGui::GetDefaultFont();
            return nullptr;
        }
    }

    add(font_data, size);

    var->gui.dpi_changed = true;

    if (ImGui::GetCurrentContext())
        return ImGui::GetDefaultFont();

    return nullptr;
}

void c_font::add(const font_blob &font_data, float size)
{
    data.push_back({font_data, size, nullptr});
}