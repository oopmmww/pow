#define WIN32_LEAN_AND_MEAN
#include "headers/window_lifecycle.h"
#include "runtime/feature_controller.h"
#include "runtime/head_protection.h"

bool RestoreGameAndClose(HWND window, std::string &message)
{
    Strike_HardHead_SetActive(false);
    if (!Strike_Reset(message))
    {
        return false;
    }
    g_hardHead.Shutdown();
    if (!PostMessageW(window, WM_CLOSE, 0, 0))
    {
        message = "Game reset completed, but the window close request failed.";
        return false;
    }
    return true;
}
