#pragma once
#include "../headers/application_includes.h"
#include "../headers/ui_types.h"
#include <memory>

class c_colors
{
  public:
    c_col layout = {21, 22, 26};
    c_col white = {255, 255, 255};
    c_col black = {0, 0, 0};
    c_col child = {27, 28, 32};
    c_col text = {215, 218, 230};
    c_col widget = {32, 34, 39};
    c_col accent = {24, 235, 117}; // Emerald green
    c_col border = {68, 72, 90};
    c_col tab_layout = {81, 83, 93};
    c_col popup_widget = {42, 43, 53};
    c_col popup = {33, 35, 41};
    c_col red = {255, 82, 85};
};

inline std::unique_ptr<c_colors> clr = std::make_unique<c_colors>();
