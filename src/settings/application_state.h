#pragma once
#include "../runtime/vendor/xor.h"
#include <string>
#include <vector>
#include "imgui.h"
#include "../headers/ui_types.h"
#include <d3dx11.h>
#include <windows.h>

struct subtab_t
{
    std::string name{};
    std::vector<std::string> weapons_list{};
};

struct tab_t
{
    std::string icon{};
    std::string name{};
    std::vector<subtab_t> subtabs_list{};
};

class c_variables
{
  public:
    struct
    {
        float dpi = 1.0f;
        int stored_dpi = 100;
        bool dpi_changed = true;
    } gui;

    struct
    {
        std::vector<tab_t> list = {
            tab_t{xorstr_("]"), xorstr_("NoRecoil"),
                  std::vector<subtab_t>{
                      subtab_t{xorstr_("Assault Rifles"),
                               std::vector<std::string>{xorstr_("AK-47"), xorstr_("M4A1"), xorstr_("M4A1-S"),
                                                        xorstr_("Famas")}},
                      subtab_t{xorstr_("SMGs"),
                               std::vector<std::string>{xorstr_("MP5A4"), xorstr_("Thompson"),
                                                        xorstr_("Custom SMG"), xorstr_("Prototype 17")}},
                      subtab_t{xorstr_("Pistols"),
                               std::vector<std::string>{xorstr_("USP-S"), xorstr_("Glock-18"),
                                                        xorstr_("TEC-9"), xorstr_("Desert Eagle")}},
                      subtab_t{xorstr_("Shotguns"),
                               std::vector<std::string>{xorstr_("SPAS-12"), xorstr_("MAG7"),
                                                        xorstr_("XM1014"), xorstr_("Sawed-Off")}},
                      subtab_t{xorstr_("Bows / Primitive")},
                      subtab_t{xorstr_("Heavy"),
                               std::vector<std::string>{xorstr_("AWP"), xorstr_("SCAR20"), xorstr_("G3SG1")}},
                  }},
            tab_t{xorstr_("C"), xorstr_("Config")},
            tab_t{xorstr_("["), xorstr_("Hotkeys"),
                  std::vector<subtab_t>{subtab_t{xorstr_("Miscellaneous")}, subtab_t{xorstr_("Weapons")},
                                        subtab_t{xorstr_("Config")}}},
            tab_t{xorstr_("R"), xorstr_("Miscellaneous"),
                  std::vector<subtab_t>{subtab_t{xorstr_("Appearance")}, subtab_t{xorstr_("HUD")},
                                        subtab_t{xorstr_("Changelog")}}}};

        int tab_current = 0;
        int tab_stored = 0;

        int subtab_current = 0;
        int subtab_stored = 0;

        int item_current = 0;
        int item_stored = 0;

        float alpha = 0.f;

        ImDrawList *draw_list;

    } tab;

    struct
    {
        ImDrawList *draw_list;
    } widget;

    struct
    {
        float alpha;
        bool active;
    } blackout;

    struct
    {
        ImTextureID background;
        ImTextureID user_logo;
        ImTextureID logo;
        ImTextureID ak47_weapon;
        ImTextureID thompson_weapon;
        ImTextureID m249_lmg_weapon;
        ImTextureID l96_rifle_weapon;
        ImTextureID semi_automatic_rifle_weapon;
        ImTextureID thompson_custom_weapon;
        ImTextureID bolt_action_weapon;
        ImTextureID spas_12_weapon;
        ImTextureID mp5_weapon;
        ImTextureID pool_cue;
    } img;

    struct
    {
        ID3D11Device *device;
        ID3D11DeviceContext *ctx;
        IDXGISwapChain *swap_chain;
    } dx;

    struct
    {
        HWND hwnd;
        RECT rc;
    } winapi;

    gui_style style;
};

inline std::unique_ptr<c_variables> var = std::make_unique<c_variables>();
