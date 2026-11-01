#pragma once
#include "application_includes.h"
#include "../data/font_registry.h"

class c_font
{
  public:
    void update();

    ImFont *get(const font_blob &font_data, float size);

  private:
    struct font_data
    {
        font_blob data;
        float size;
        ImFont *font;

        bool operator==(const font_data &other) const
        {
            return data == other.data && size == other.size;
        }
    };

    void add(const font_blob &font_data, float size);

    std::vector<font_data> data;
};

inline std::unique_ptr<c_font> font = std::make_unique<c_font>();