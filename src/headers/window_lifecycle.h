#pragma once

#include <windows.h>
#include <string>

// Restore through the existing feature controller before requesting window close.
// On failure, leave the window open and return a user-facing error.
bool RestoreGameAndClose(HWND window, std::string &message);
