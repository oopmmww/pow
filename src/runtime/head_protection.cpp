#include "../vendor/api_hidden.h"
#include "process_locator.h"
#include "head_protection.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <vector>

static std::string WStringToString(const std::wstring &wstr)
{
    if (wstr.empty())
        return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

static bool MatchesProbe(const uint8_t *buf, const ac_vars::HhConfig &cfg)
{
    if ((buf[0] == cfg.probe_prologue[0] && buf[1] == cfg.probe_prologue[1] &&
         buf[2] == cfg.probe_prologue[2]) ||
        (buf[0] == cfg.probe_trampoline[0] && buf[1] == cfg.probe_trampoline[1]))
    {
        return true;
    }
    return false;
}

HardHeadEngine::HardHeadEngine()
{
    EnableDebugPrivilege();
}

HardHeadEngine::~HardHeadEngine()
{
    Shutdown();
}

bool HardHeadEngine::Initialize()
{
    return EnableDebugPrivilege();
}

void HardHeadEngine::Shutdown()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_hProcess != nullptr)
    {
        if (m_caveBase != 0 && ac_vars::IsConfigured())
        {
            uint8_t zero = 0;
            WriteProcessMemorySafe(m_caveBase + ac_vars::GetConfig().cave_enable_off, &zero, 1);
        }
        CloseHandle(m_hProcess);
        m_hProcess = nullptr;
    }
    m_processId = 0;
    m_moduleBase = 0;
    m_hookBase = 0;
    m_caveBase = 0;
    m_trampolineInstalled = false;
    m_isEffectActive = false;
}

bool HardHeadEngine::EnableDebugPrivilege()
{
    HANDLE hToken = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
    {
        return false;
    }
    LUID luid;
    if (!LookupPrivilegeValue(nullptr, SE_DEBUG_NAME, &luid))
    {
        CloseHandle(hToken);
        return false;
    }
    TOKEN_PRIVILEGES tp;
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    BOOL ok = AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), nullptr, nullptr);
    CloseHandle(hToken);
    return ok != FALSE;
}

DWORD HardHeadEngine::FindFiveMProcess(std::wstring &outExeName)
{
    DWORD foundPid = 0;
    outExeName.clear();

    HWND hGrc = FindWindowW(xorstr_(L"grcWindow"), nullptr);
    if (hGrc)
    {
        DWORD winPid = 0;
        GetWindowThreadProcessId(hGrc, &winPid);
        if (winPid != 0)
        {
            HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            if (hSnap != INVALID_HANDLE_VALUE)
            {
                PROCESSENTRY32W pe;
                pe.dwSize = sizeof(pe);
                if (Process32FirstW(hSnap, &pe))
                {
                    do
                    {
                        if (pe.th32ProcessID == winPid)
                        {
                            foundPid = winPid;
                            outExeName = pe.szExeFile;
                            break;
                        }
                    } while (Process32NextW(hSnap, &pe));
                }
                CloseHandle(hSnap);
            }
            if (foundPid != 0)
            {
                m_hGameWindow = hGrc;
                return foundPid;
            }
        }
    }

    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE)
        return 0;

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(pe);

    if (Process32FirstW(hSnap, &pe))
    {
        do
        {
            std::wstring exe = pe.szExeFile;
            std::wstring lowerExe = exe;
            std::transform(lowerExe.begin(), lowerExe.end(), lowerExe.begin(), ::towlower);

            if (lowerExe.find(xorstr_(L"gtaprocess")) != std::wstring::npos ||
                lowerExe.find(xorstr_(L"fivem")) != std::wstring::npos &&
                    lowerExe.find(xorstr_(L".exe")) != std::wstring::npos)
            {
                if (lowerExe.find(xorstr_(L"fivem_")) != std::wstring::npos ||
                    lowerExe == xorstr_(L"gtaprocess.exe"))
                {
                    foundPid = pe.th32ProcessID;
                    outExeName = exe;
                    break;
                }
            }
        } while (Process32NextW(hSnap, &pe));
    }
    CloseHandle(hSnap);

    return foundPid;
}

uintptr_t HardHeadEngine::GetModuleBase(DWORD pid, const std::wstring &moduleName)
{
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (hSnap == INVALID_HANDLE_VALUE)
        return 0;

    MODULEENTRY32W me;
    me.dwSize = sizeof(me);
    uintptr_t baseAddr = 0;

    if (Module32FirstW(hSnap, &me))
    {
        do
        {
            std::wstring mod = me.szModule;
            std::wstring lowerMod = mod;
            std::transform(lowerMod.begin(), lowerMod.end(), lowerMod.begin(), ::towlower);

            std::wstring target = moduleName;
            std::transform(target.begin(), target.end(), target.begin(), ::towlower);

            if (lowerMod == target || lowerMod.find(xorstr_(L"gtaprocess")) != std::wstring::npos)
            {
                baseAddr = (uintptr_t)me.modBaseAddr;
                break;
            }
        } while (Module32NextW(hSnap, &me));
    }
    CloseHandle(hSnap);
    return baseAddr;
}

bool HardHeadEngine::CheckGameWindow()
{
    HWND hForeground = GetForegroundWindow();
    if (!hForeground)
        return false;

    wchar_t className[256];
    if (GetClassNameW(hForeground, className, sizeof(className) / sizeof(wchar_t)))
    {
        if (wcscmp(className, xorstr_(L"grcWindow")) == 0)
        {
            return true;
        }
    }

    if (m_processId != 0)
    {
        DWORD fgPid = 0;
        GetWindowThreadProcessId(hForeground, &fgPid);
        return (fgPid == m_processId);
    }

    return false;
}

bool HardHeadEngine::WriteProcessMemorySafe(uintptr_t address, const void *buffer, size_t size)
{
    if (!m_hProcess || address == 0 || buffer == nullptr || size == 0)
        return false;

    DWORD oldProtect = 0;
    VirtualProtectEx(m_hProcess, (LPVOID)address, size, PAGE_EXECUTE_READWRITE, &oldProtect);

    SIZE_T bytesWritten = 0;
    BOOL ok = WriteProcessMemory(m_hProcess, (LPVOID)address, buffer, size, &bytesWritten);

    if (oldProtect != 0)
    {
        DWORD dummy = 0;
        VirtualProtectEx(m_hProcess, (LPVOID)address, size, oldProtect, &dummy);
    }
    return (ok != FALSE && bytesWritten == size);
}

bool HardHeadEngine::ReadProcessMemorySafe(uintptr_t address, void *buffer, size_t size)
{
    if (!m_hProcess || address == 0 || buffer == nullptr || size == 0)
        return false;

    SIZE_T bytesRead = 0;
    BOOL ok = ReadProcessMemory(m_hProcess, (LPCVOID)address, buffer, size, &bytesRead);
    return (ok != FALSE && bytesRead == size);
}

uintptr_t HardHeadEngine::ResolvePointerRva(uint32_t *outBuildId)
{
    if (outBuildId)
        *outBuildId = 0;
    if (!ac_vars::IsConfigured())
        return 0;

    const ac_vars::HhConfig &cfg = ac_vars::GetConfig();
    std::wstring lowerExe = m_exeName;
    std::transform(lowerExe.begin(), lowerExe.end(), lowerExe.begin(), ::towlower);

    size_t bPos = lowerExe.find(L"_b");
    if (bPos != std::wstring::npos)
    {
        size_t numStart = bPos + 2;
        size_t numEnd = lowerExe.find(L"_", numStart);
        if (numEnd != std::wstring::npos)
        {
            std::wstring bNumStr = lowerExe.substr(numStart, numEnd - numStart);
            try
            {
                int buildNum = std::stoi(bNumStr);
                if (buildNum >= cfg.fivem_build_min)
                {
                    const AntiCritBuildEntry *latest = ac_vars::GetLatestBuild();
                    if (latest)
                    {
                        if (outBuildId)
                            *outBuildId = latest->buildId;
                        return latest->pointerRva;
                    }
                }
            }
            catch (...)
            {
            }
        }
    }

    const AntiCritBuildEntry *latest = ac_vars::GetLatestBuild();
    if (latest)
    {
        if (outBuildId)
            *outBuildId = latest->buildId;
        return latest->pointerRva;
    }
    return 0;
}

uintptr_t HardHeadEngine::ResolveHookFromSlotRva(uintptr_t slotRva)
{
    if (m_moduleBase == 0 || slotRva == 0 || !ac_vars::IsConfigured())
        return 0;

    const ac_vars::HhConfig &cfg = ac_vars::GetConfig();
    uintptr_t hookRva = m_moduleBase + ac_vars::GetHookBaseRva();
    uint8_t checkBuf[3] = {0};
    if (ReadProcessMemorySafe(hookRva, checkBuf, sizeof(checkBuf)))
    {
        if (MatchesProbe(checkBuf, cfg))
            return hookRva;
    }

    uintptr_t slotAddr = m_moduleBase + slotRva;
    std::vector<uint8_t> slotBuf(cfg.slot_scan_bytes);
    if (ReadProcessMemorySafe(slotAddr, slotBuf.data(), slotBuf.size()))
    {
        if (MatchesProbe(slotBuf.data(), cfg))
            return slotAddr;
    }

    return hookRva;
}

uintptr_t HardHeadEngine::ResolveChainRootRva(uint32_t buildId)
{
    return ac_vars::ResolveChainRootRva(buildId);
}

bool HardHeadEngine::CheckAndAttachProcess()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_hProcess != nullptr)
    {
        DWORD exitCode = 0;
        if (GetExitCodeProcess(m_hProcess, &exitCode) && exitCode == STILL_ACTIVE)
        {
            return true;
        }
        CloseHandle(m_hProcess);
        m_hProcess = nullptr;
        m_processId = 0;
        m_moduleBase = 0;
        m_hookBase = 0;
        m_caveBase = 0;
        m_trampolineInstalled = false;
        m_isEffectActive = false;
    }

    if (!game_process::ResolveOnce() || !game_process::IsAlive())
    {
        m_lastError = xorstr_("Waiting for target process...");
        return false;
    }

    const game_process::Info *info = game_process::Get();
    HANDLE shared = game_process::GetHandle();
    if (!info || !shared)
    {
        m_lastError = xorstr_("Waiting for target process...");
        return false;
    }

    HANDLE hProc = nullptr;
    if (!DuplicateHandle(GetCurrentProcess(), shared, GetCurrentProcess(), &hProc, 0, FALSE,
                         DUPLICATE_SAME_ACCESS))
    {
        m_lastError = xorstr_("Failed to attach to target process (Error: ") +
                      std::to_string(GetLastError()) + xorstr_(")");
        return false;
    }

    m_hProcess = hProc;
    m_processId = info->pid;
    m_exeName = info->name;
    m_moduleBase = info->modBase;
    m_lastError = xorstr_("Attached to ") + WStringToString(m_exeName) + xorstr_(" (PID: ") +
                  std::to_string(info->pid) + xorstr_(")");

    return true;
}

bool HardHeadEngine::EnsureInstalled()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_hProcess || m_moduleBase == 0)
    {
        m_lastError = xorstr_("Not attached to FiveM");
        return false;
    }

    if (!ac_vars::IsConfigured())
    {
        m_lastError = xorstr_("Anti-crit globals missing or incomplete.");
        return false;
    }

    if (m_trampolineInstalled && m_caveBase != 0)
    {
        return true;
    }

    const ac_vars::HhConfig &cfg = ac_vars::GetConfig();

    m_pointerRva = ResolvePointerRva(&m_buildId);
    m_hookBase = ResolveHookFromSlotRva(m_pointerRva);
    if (m_hookBase == 0)
    {
        m_lastError = xorstr_("Failed to resolve hook address for build.");
        return false;
    }
    m_patchAddr = m_hookBase + cfg.patch_offset;

    uint8_t probeBuf[16] = {0};
    if (!ReadProcessMemorySafe(m_hookBase, probeBuf, 16))
    {
        m_lastError = xorstr_("Failed to read target bytes at hookBase.");
        return false;
    }

    std::vector<uint8_t> savedPrologue(cfg.prologue_save_bytes);
    memcpy(savedPrologue.data(), probeBuf, cfg.prologue_save_bytes);

    if (m_caveBase == 0)
    {
        LPVOID cavePtr = VirtualAllocEx(m_hProcess, nullptr, cfg.cave_alloc_size, MEM_COMMIT | MEM_RESERVE,
                                        PAGE_EXECUTE_READWRITE);
        if (!cavePtr)
        {
            m_lastError = xorstr_("Allocation failed in target process.");
            return false;
        }
        m_caveBase = (uintptr_t)cavePtr;
    }

    uint32_t ringCap = cfg.cave_ring_cap;
    WriteProcessMemorySafe(m_caveBase + cfg.cave_ring_cap_off, &ringCap, sizeof(ringCap));
    uint8_t initialEnable = 0;
    WriteProcessMemorySafe(m_caveBase + cfg.cave_enable_off, &initialEnable, sizeof(initialEnable));

    const uint8_t *caveTemplate = ac_vars::GetCaveTemplate();
    const size_t caveTemplateSize = ac_vars::GetCaveTemplateSize();
    if (!caveTemplate || caveTemplateSize == 0)
    {
        m_lastError = xorstr_("Cave template not loaded.");
        return false;
    }
    if (caveTemplateSize > cfg.cave_template_size)
    {
        m_lastError = xorstr_("Cave template too large.");
        return false;
    }

    std::vector<uint8_t> caveShellcode(cfg.cave_template_size, 0);
    memcpy(caveShellcode.data(), caveTemplate, caveTemplateSize);

    uintptr_t chainRoot = m_moduleBase + ResolveChainRootRva(m_buildId);
    uintptr_t resumeAddr = m_hookBase + cfg.prologue_save_bytes;

    memcpy(&caveShellcode[cfg.tpl_off_counter], &m_caveBase, sizeof(uint64_t));
    memcpy(&caveShellcode[cfg.tpl_off_chain], &chainRoot, sizeof(uint64_t));
    memcpy(&caveShellcode[cfg.tpl_off_prologue], savedPrologue.data(), cfg.prologue_save_bytes);
    memcpy(&caveShellcode[cfg.tpl_off_resume], &resumeAddr, sizeof(uint64_t));

    uintptr_t caveExecAddr = m_caveBase + cfg.cave_exec_off;
    if (!WriteProcessMemorySafe(caveExecAddr, caveShellcode.data(), cfg.cave_template_size))
    {
        m_lastError = xorstr_("Failed to write to cave.");
        return false;
    }

    std::vector<uint8_t> trampoline(cfg.trampoline_size, 0);
    trampoline[0] = cfg.probe_trampoline[0];
    trampoline[1] = cfg.probe_trampoline[1];
    memcpy(&trampoline[6], &caveExecAddr, sizeof(uint64_t));

    if (!WriteProcessMemorySafe(m_hookBase, trampoline.data(), cfg.trampoline_size))
    {
        m_lastError = xorstr_("Failed to write trampoline at hookBase.");
        return false;
    }

    uint8_t nopByte = cfg.install_byte;
    WriteProcessMemorySafe(m_patchAddr, &nopByte, 1);

    m_trampolineInstalled = true;
    m_lastError = xorstr_("Hook & Code Cave successfully installed!");
    return true;
}

void HardHeadEngine::SetActive(bool active)
{
    if (m_isEffectActive.load() == active)
        return;

    if (m_audioFeedback)
    {
        if (active)
        {
            Beep(880, 70);
            Beep(1200, 70);
        }
        else
        {
            Beep(600, 70);
        }
    }

    m_isEffectActive.store(active);

    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_hProcess && m_caveBase != 0 && ac_vars::IsConfigured())
    {
        const ac_vars::HhConfig &cfg = ac_vars::GetConfig();
        uint8_t enableByte = active ? 1 : 0;
        WriteProcessMemorySafe(m_caveBase + cfg.cave_enable_off, &enableByte, 1);

        if (active && m_patchAddr != 0)
        {
            uint8_t nopByte = cfg.install_byte;
            WriteProcessMemorySafe(m_patchAddr, &nopByte, 1);
        }
    }
}

void HardHeadEngine::ToggleActive()
{
    SetActive(!m_isEffectActive.load());
}

void HardHeadEngine::SetPercent(int pct)
{
    if (pct < 0)
        pct = 0;
    if (pct > 100)
        pct = 100;
    m_percent.store(pct);
}

void HardHeadEngine::MaintenanceTick()
{
    if (!CheckAndAttachProcess())
        return;
    if (!EnsureInstalled())
        return;

    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_hProcess || m_caveBase == 0 || !ac_vars::IsConfigured())
        return;

    const ac_vars::HhConfig &cfg = ac_vars::GetConfig();

    if (m_onlyWhenFocused && !CheckGameWindow())
    {
        uint8_t zero = 0;
        WriteProcessMemorySafe(m_caveBase + cfg.cave_enable_off, &zero, 1);
        return;
    }

    bool active = m_isEffectActive.load();
    if (active)
    {
        int pct = m_percent.load();
        bool allow = true;
        if (pct <= 0)
            allow = false;
        else if (pct < 100)
        {
            allow = ((rand() % 100) < pct);
        }

        uint8_t enableVal = allow ? 1 : 0;
        WriteProcessMemorySafe(m_caveBase + cfg.cave_enable_off, &enableVal, 1);

        if (allow && m_patchAddr != 0)
        {
            uint8_t currentByte = 0;
            ReadProcessMemorySafe(m_patchAddr, &currentByte, 1);
            if (currentByte != cfg.install_byte)
            {
                uint8_t nopByte = cfg.install_byte;
                WriteProcessMemorySafe(m_patchAddr, &nopByte, 1);
            }
        }
    }
    else
    {
        uint8_t zero = 0;
        WriteProcessMemorySafe(m_caveBase + cfg.cave_enable_off, &zero, 1);
    }
}

void HardHeadEngine::DetachProcess()
{
    Shutdown();
}

EngineStatus HardHeadEngine::GetStatus()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    EngineStatus s;
    s.processId = m_processId;
    s.processExeName = m_exeName;
    s.moduleBase = m_moduleBase;
    s.hookBaseAddress = m_hookBase;
    s.patchAddress = m_patchAddr;
    s.caveBaseAddress = m_caveBase;
    s.resolvedBuildId = m_buildId;
    s.resolvedPointerRva = m_pointerRva;
    s.isTrampolineInstalled = m_trampolineInstalled;
    s.isEffectActive = m_isEffectActive.load();
    s.isFiveMFocused = CheckGameWindow();
    s.isInGameWindow = (FindWindowW(xorstr_(L"grcWindow"), nullptr) != nullptr);
    s.lastErrorMessage = m_lastError;

    if (m_hProcess == nullptr)
    {
        s.state = EngineState::WaitingForFiveM;
    }
    else if (!m_trampolineInstalled)
    {
        s.state = EngineState::Attaching;
    }
    else if (s.isEffectActive)
    {
        s.state = EngineState::Installed_Active;
    }
    else
    {
        s.state = EngineState::Installed_Disabled;
    }

    return s;
}

std::string HardHeadEngine::GetHotkeyName() const
{
    switch (m_hotkey)
    {
    case VK_LBUTTON:
        return "Left Mouse";
    case VK_RBUTTON:
        return "Right Mouse";
    case VK_MBUTTON:
        return "Middle Mouse";
    case VK_XBUTTON1:
        return "Mouse 4";
    case VK_XBUTTON2:
        return "Mouse 5";
    case VK_SHIFT:
    case VK_LSHIFT:
        return "Left Shift";
    case VK_RSHIFT:
        return "Right Shift";
    case VK_CONTROL:
    case VK_LCONTROL:
        return "Left Ctrl";
    case VK_RCONTROL:
        return "Right Ctrl";
    case VK_MENU:
    case VK_LMENU:
        return "Left Alt";
    case VK_RMENU:
        return "Right Alt";
    case VK_CAPITAL:
        return "Caps Lock";
    case VK_SPACE:
        return "Spacebar";
    case VK_TAB:
        return "Tab";
    case VK_RETURN:
        return "Enter";
    case VK_ESCAPE:
        return "Escape";
    case VK_F1:
        return "F1";
    case VK_F2:
        return "F2";
    case VK_F3:
        return "F3";
    case VK_F4:
        return "F4";
    case VK_F5:
        return "F5";
    case VK_F6:
        return "F6";
    case VK_F7:
        return "F7";
    case VK_F8:
        return "F8";
    case VK_F9:
        return "F9";
    case VK_F10:
        return "F10";
    case VK_F11:
        return "F11";
    case VK_F12:
        return "F12";
    default:
    {
        if (m_hotkey >= 'A' && m_hotkey <= 'Z')
        {
            return std::string(1, (char)m_hotkey);
        }
        if (m_hotkey >= '0' && m_hotkey <= '9')
        {
            return std::string(1, (char)m_hotkey);
        }
        return "Key (0x" + ([&](){
                std::stringstream ss;
                ss << std::hex << std::uppercase << m_hotkey;
                return ss.str();
            })() + ")";
    }
    }
}

HardHeadEngine g_hardHead;

void Strike_HardHead_SetActive(bool active)
{
    g_hardHead.SetActive(active);
}

bool Strike_HardHead_IsActive()
{
    return g_hardHead.IsActive();
}

void Strike_HardHead_Tick()
{
    g_hardHead.MaintenanceTick();
}
