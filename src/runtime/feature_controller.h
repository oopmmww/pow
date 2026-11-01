#pragma once

#include <string>

bool StrikeUiRun();

// Bridge API for GUI Integration
void Strike_Initialize();
bool Strike_ApplyLow(std::string &outMsg);
bool Strike_ApplyMedium(std::string &outMsg);
bool Strike_ApplyHigh(std::string &outMsg);
bool Strike_ApplyCustom(float val, std::string &outMsg, float area = 0.15f);
bool Strike_SetRbp38(bool enable, std::string &outMsg);
bool Strike_GetRbp38();
bool Strike_SetRunSpeed(bool enable, std::string &outMsg, float speedVal = 1.07f);
bool Strike_GetRunSpeed();
bool Strike_Reset(std::string &outMsg);

int Strike_GetMenuToggleVk();
void Strike_SetMenuToggleVk(int vk);

const char *Strike_GetLastStatus();
void Strike_Shutdown();
