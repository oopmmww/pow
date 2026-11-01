#pragma once

#include <windows.h>
#include <cstdint>

namespace game_process
{

struct Info
{
    DWORD pid = 0;
    uintptr_t modBase = 0;
    uint32_t build = 0;
    wchar_t name[64] = {};
};

// Scan + open target at most once until Invalidate(). Returns false if not found.
bool ResolveOnce();

// Clear cached target (also on Restore / process exit).
void Invalidate();

// Cached target from the single lookup round.
const Info *Get();

// Persistent process handle from ResolveOnce(); nullptr if unresolved.
HANDLE GetHandle();

bool IsAlive();

} // namespace game_process
