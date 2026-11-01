#pragma once
#include <string>
#include "imgui.h"
#include "../runtime/vendor/xor.h"

class c_elements
{
  public:
    struct
    {
        std::string name{xorstr_("FREE FOR YOU")};
        c_vec2 size{430, 720};
        c_vec2 padding = {10, 10};
        float rounding{16};
    } window;

    struct
    {
        c_vec2 size = {0, 0};
    } nav;

    struct
    {
        c_vec2 size = {610, 500};
        c_vec2 padding = {10, 10};
    } content;

    struct
    {
        float width;
        float rounding = 10;
        c_vec2 padding = {10, 10};
    } child;
};

inline std::unique_ptr<c_elements> elements = std::make_unique<c_elements>();
