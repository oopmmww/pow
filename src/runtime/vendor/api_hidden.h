#pragma once
#include <windows.h>
#include <sddl.h>
#include <tlhelp32.h>
#include <d3d11.h>
#include "xor.h"
#include "../native_memory.h"

namespace ds {
    uintptr_t GetLocalExport(const wchar_t* module_name, const char* func_name);
}

namespace hidden {

    template<typename T>
    inline T GetApi(const wchar_t* mod, const char* func) {
        uintptr_t base = ds::GetLocalModuleBase(mod);
        if (!base) {
            LoadLibraryW(mod); // Need to load if not already in memory (e.g. d3d11)
            base = ds::GetLocalModuleBase(mod);
        }
        if (base) {
            return (T)ds::GetLocalExport(mod, func);
        }
        return nullptr;
    }

    inline HANDLE WINAPI CreateToolhelp32Snapshot(DWORD dwFlags, DWORD th32ProcessID) {
        static auto p = GetApi<decltype(&::CreateToolhelp32Snapshot)>(xorstr_(L"kernel32.dll"), xorstr_("CreateToolhelp32Snapshot"));
        return p ? p(dwFlags, th32ProcessID) : INVALID_HANDLE_VALUE;
    }

    inline HRESULT WINAPI D3D11CreateDeviceAndSwapChain(IDXGIAdapter* pAdapter, D3D_DRIVER_TYPE DriverType, HMODULE Software, UINT Flags, const D3D_FEATURE_LEVEL* pFeatureLevels, UINT FeatureLevels, UINT SDKVersion, const DXGI_SWAP_CHAIN_DESC* pSwapChainDesc, IDXGISwapChain** ppSwapChain, ID3D11Device** ppDevice, D3D_FEATURE_LEVEL* pFeatureLevel, ID3D11DeviceContext** ppImmediateContext) {
        static auto p = GetApi<decltype(&::D3D11CreateDeviceAndSwapChain)>(xorstr_(L"d3d11.dll"), xorstr_("D3D11CreateDeviceAndSwapChain"));
        return p ? p(pAdapter, DriverType, Software, Flags, pFeatureLevels, FeatureLevels, SDKVersion, pSwapChainDesc, ppSwapChain, ppDevice, pFeatureLevel, ppImmediateContext) : E_FAIL;
    }

    inline SHORT WINAPI GetAsyncKeyState(int vKey) {
        static auto p = GetApi<decltype(&::GetAsyncKeyState)>(xorstr_(L"user32.dll"), xorstr_("GetAsyncKeyState"));
        return p ? p(vKey) : 0;
    }

    inline BOOL WINAPI Module32FirstW(HANDLE hSnapshot, LPMODULEENTRY32W lpme) {
        static auto p = GetApi<decltype(&::Module32FirstW)>(xorstr_(L"kernel32.dll"), xorstr_("Module32FirstW"));
        return p ? p(hSnapshot, lpme) : FALSE;
    }

    inline BOOL WINAPI Module32NextW(HANDLE hSnapshot, LPMODULEENTRY32W lpme) {
        static auto p = GetApi<decltype(&::Module32NextW)>(xorstr_(L"kernel32.dll"), xorstr_("Module32NextW"));
        return p ? p(hSnapshot, lpme) : FALSE;
    }

    inline HANDLE WINAPI OpenProcess(DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwProcessId) {
        static auto p = GetApi<decltype(&::OpenProcess)>(xorstr_(L"kernel32.dll"), xorstr_("OpenProcess"));
        return p ? p(dwDesiredAccess, bInheritHandle, dwProcessId) : NULL;
    }

    inline BOOL WINAPI Process32FirstW(HANDLE hSnapshot, LPPROCESSENTRY32W lppe) {
        static auto p = GetApi<decltype(&::Process32FirstW)>(xorstr_(L"kernel32.dll"), xorstr_("Process32FirstW"));
        return p ? p(hSnapshot, lppe) : FALSE;
    }

    inline BOOL WINAPI Process32NextW(HANDLE hSnapshot, LPPROCESSENTRY32W lppe) {
        static auto p = GetApi<decltype(&::Process32NextW)>(xorstr_(L"kernel32.dll"), xorstr_("Process32NextW"));
        return p ? p(hSnapshot, lppe) : FALSE;
    }

    inline BOOL WINAPI ReadProcessMemory(HANDLE hProcess, LPCVOID lpBaseAddress, LPVOID lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesRead) {
        static auto p = GetApi<decltype(&::ReadProcessMemory)>(xorstr_(L"kernel32.dll"), xorstr_("ReadProcessMemory"));
        return p ? p(hProcess, lpBaseAddress, lpBuffer, nSize, lpNumberOfBytesRead) : FALSE;
    }

    inline LPVOID WINAPI VirtualAllocEx(HANDLE hProcess, LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect) {
        static auto p = GetApi<decltype(&::VirtualAllocEx)>(xorstr_(L"kernel32.dll"), xorstr_("VirtualAllocEx"));
        return p ? p(hProcess, lpAddress, dwSize, flAllocationType, flProtect) : NULL;
    }

    inline BOOL WINAPI VirtualProtectEx(HANDLE hProcess, LPVOID lpAddress, SIZE_T dwSize, DWORD flNewProtect, PDWORD lpflOldProtect) {
        static auto p = GetApi<decltype(&::VirtualProtectEx)>(xorstr_(L"kernel32.dll"), xorstr_("VirtualProtectEx"));
        return p ? p(hProcess, lpAddress, dwSize, flNewProtect, lpflOldProtect) : FALSE;
    }

    inline BOOL WINAPI WriteProcessMemory(HANDLE hProcess, LPVOID lpBaseAddress, LPCVOID lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesWritten) {
        static auto p = GetApi<decltype(&::WriteProcessMemory)>(xorstr_(L"kernel32.dll"), xorstr_("WriteProcessMemory"));
        return p ? p(hProcess, lpBaseAddress, lpBuffer, nSize, lpNumberOfBytesWritten) : FALSE;
    }

    inline BOOL WINAPI OpenProcessToken(HANDLE ProcessHandle, DWORD DesiredAccess, PHANDLE TokenHandle) {
        static auto p = GetApi<decltype(&::OpenProcessToken)>(xorstr_(L"advapi32.dll"), xorstr_("OpenProcessToken"));
        return p ? p(ProcessHandle, DesiredAccess, TokenHandle) : FALSE;
    }

    inline BOOL WINAPI AdjustTokenPrivileges(HANDLE TokenHandle, BOOL DisableAllPrivileges, PTOKEN_PRIVILEGES NewState, DWORD BufferLength, PTOKEN_PRIVILEGES PreviousState, PDWORD ReturnLength) {
        static auto p = GetApi<decltype(&::AdjustTokenPrivileges)>(xorstr_(L"advapi32.dll"), xorstr_("AdjustTokenPrivileges"));
        return p ? p(TokenHandle, DisableAllPrivileges, NewState, BufferLength, PreviousState, ReturnLength) : FALSE;
    }

    inline BOOL WINAPI LookupPrivilegeValueW(LPCWSTR lpSystemName, LPCWSTR lpName, PLUID lpLuid) {
        static auto p = GetApi<decltype(&::LookupPrivilegeValueW)>(xorstr_(L"advapi32.dll"), xorstr_("LookupPrivilegeValueW"));
        return p ? p(lpSystemName, lpName, lpLuid) : FALSE;
    }

    inline BOOL WINAPI ConvertSidToStringSidA(PSID Sid, LPSTR* StringSid) {
        static auto p = GetApi<decltype(&::ConvertSidToStringSidA)>(xorstr_(L"advapi32.dll"), xorstr_("ConvertSidToStringSidA"));
        return p ? p(Sid, StringSid) : FALSE;
    }

} // namespace hidden

#define ConvertSidToStringSidA hidden::ConvertSidToStringSidA
#define AdjustTokenPrivileges hidden::AdjustTokenPrivileges
#define LookupPrivilegeValueW hidden::LookupPrivilegeValueW

#define CreateToolhelp32Snapshot hidden::CreateToolhelp32Snapshot
#define D3D11CreateDeviceAndSwapChain hidden::D3D11CreateDeviceAndSwapChain
#define GetAsyncKeyState hidden::GetAsyncKeyState
#define Module32FirstW hidden::Module32FirstW
#define Module32NextW hidden::Module32NextW
#define OpenProcess hidden::OpenProcess
#define OpenProcessToken hidden::OpenProcessToken
#define Process32FirstW hidden::Process32FirstW
#define Process32NextW hidden::Process32NextW
#define ReadProcessMemory hidden::ReadProcessMemory
#define VirtualAllocEx hidden::VirtualAllocEx
#define VirtualProtectEx hidden::VirtualProtectEx
#define WriteProcessMemory hidden::WriteProcessMemory


