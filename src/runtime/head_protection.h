#pragma once
// ============================================================================
// head_protection.h
// HARD HEAD (Anti-Critical) Core Engine & Process Memory Controller
// ============================================================================

#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <cstdint>
#include <string>
#include <vector>
#include <atomic>
#include <mutex>
#include "protection_builds.h"

enum class ActivationMode
{
    Toggle = 0,
    Hold = 1
};

enum class EngineState
{
    WaitingForFiveM,
    Attaching,
    Installed_Disabled,
    Installed_Active,
    Error
};

struct EngineStatus
{
    EngineState state = EngineState::WaitingForFiveM;
    DWORD processId = 0;
    std::wstring processExeName = L"";
    uintptr_t moduleBase = 0;
    uintptr_t hookBaseAddress = 0;
    uintptr_t patchAddress = 0;
    uintptr_t caveBaseAddress = 0;
    uint32_t resolvedBuildId = 0;
    uintptr_t resolvedPointerRva = 0;
    bool isTrampolineInstalled = false;
    bool isEffectActive = false;
    bool isFiveMFocused = false;
    bool isInGameWindow = false;
    std::string lastErrorMessage = "";
};

class HardHeadEngine
{
  public:
    HardHeadEngine();
    ~HardHeadEngine();

    bool Initialize();
    void Shutdown();

    // Process & Memory Hook Operations
    bool CheckAndAttachProcess();
    bool EnsureInstalled();
    void DetachProcess();

    // Protection control
    void SetActive(bool active);
    bool IsActive() const
    {
        return m_isEffectActive.load();
    }
    void ToggleActive();

    // Configuration
    void SetActivationMode(ActivationMode mode)
    {
        m_activationMode = mode;
    }
    ActivationMode GetActivationMode() const
    {
        return m_activationMode;
    }

    void SetHotkey(int vKey)
    {
        m_hotkey = vKey;
    }
    int GetHotkey() const
    {
        return m_hotkey;
    }
    std::string GetHotkeyName() const;

    void SetPercent(int pct);
    int GetPercent() const
    {
        return m_percent.load();
    }

    void SetOnlyWhenFiveMFocused(bool enabled)
    {
        m_onlyWhenFocused = enabled;
    }
    bool GetOnlyWhenFiveMFocused() const
    {
        return m_onlyWhenFocused;
    }

    void SetAudioFeedback(bool enabled)
    {
        m_audioFeedback = enabled;
    }
    bool GetAudioFeedback() const
    {
        return m_audioFeedback;
    }

    // Status
    EngineStatus GetStatus();

    // Maintenance tick (called periodically in background)
    void MaintenanceTick();

  private:
    bool EnableDebugPrivilege();
    DWORD FindFiveMProcess(std::wstring &outExeName);
    uintptr_t GetModuleBase(DWORD pid, const std::wstring &moduleName);
    bool CheckGameWindow();

    uintptr_t ResolvePointerRva(uint32_t *outBuildId = nullptr);
    uintptr_t ResolveHookFromSlotRva(uintptr_t slotRva);
    uintptr_t ResolveChainRootRva(uint32_t buildId);

    bool WriteProcessMemorySafe(uintptr_t address, const void *buffer, size_t size);
    bool ReadProcessMemorySafe(uintptr_t address, void *buffer, size_t size);

  private:
    HANDLE m_hProcess = nullptr;
    DWORD m_processId = 0;
    std::wstring m_exeName = L"";
    uintptr_t m_moduleBase = 0;
    HWND m_hGameWindow = nullptr;

    uintptr_t m_hookBase = 0;
    uintptr_t m_patchAddr = 0;
    uintptr_t m_caveBase = 0;
    uint32_t m_buildId = 0;
    uintptr_t m_pointerRva = 0;
    bool m_trampolineInstalled = false;

    std::atomic<bool> m_isEffectActive{false};
    std::atomic<int> m_percent{100};
    ActivationMode m_activationMode = ActivationMode::Toggle;
    int m_hotkey = VK_F4; // Default: F4
    bool m_onlyWhenFocused = false;
    bool m_audioFeedback = true;

    mutable std::mutex m_mutex;
    std::string m_lastError = "";
};

extern HardHeadEngine g_hardHead;

void Strike_HardHead_SetActive(bool active);
bool Strike_HardHead_IsActive();
void Strike_HardHead_Tick();
