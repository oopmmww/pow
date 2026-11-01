// native_memory.h - Indirect NT syscalls (Halo's Gate + Tartarus trampoline)
// ไม่เรียก ReadProcessMemory / VirtualQueryEx / GetProcAddress จาก kernel32 ที่โดน hook

#pragma once
#include <windows.h>
#include <winternl.h>

namespace ds
{

typedef struct _DS_CLIENT_ID
{
    HANDLE UniqueProcess;
    HANDLE UniqueThread;
} DS_CLIENT_ID, *PDS_CLIENT_ID;

bool Initialize();
bool EnableDebugPrivilege();

extern "C"
{
    NTSTATUS NTAPI ds_NtAllocateVirtualMemory(HANDLE, PVOID *, ULONG_PTR, PSIZE_T, ULONG, ULONG);
    NTSTATUS NTAPI ds_NtFreeVirtualMemory(HANDLE, PVOID *, PSIZE_T, ULONG);
    NTSTATUS NTAPI ds_NtProtectVirtualMemory(HANDLE, PVOID *, PSIZE_T, ULONG, PULONG);
    NTSTATUS NTAPI ds_NtReadVirtualMemory(HANDLE, PVOID, PVOID, SIZE_T, PSIZE_T);
    NTSTATUS NTAPI ds_NtWriteVirtualMemory(HANDLE, PVOID, PVOID, SIZE_T, PSIZE_T);
    NTSTATUS NTAPI ds_NtOpenProcess(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, PDS_CLIENT_ID);
    NTSTATUS NTAPI ds_NtOpenThread(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, PDS_CLIENT_ID);
    NTSTATUS NTAPI ds_NtSuspendThread(HANDLE, PULONG);
    NTSTATUS NTAPI ds_NtResumeThread(HANDLE, PULONG);
    NTSTATUS NTAPI ds_NtQueueApcThread(HANDLE, PVOID, PVOID, PVOID, PVOID);
    NTSTATUS NTAPI ds_NtGetContextThread(HANDLE, PCONTEXT);
    NTSTATUS NTAPI ds_NtSetContextThread(HANDLE, PCONTEXT);
    NTSTATUS NTAPI ds_NtClose(HANDLE);
    NTSTATUS NTAPI ds_NtQueryInformationProcess(HANDLE, ULONG, PVOID, ULONG, PULONG);
    NTSTATUS NTAPI ds_NtQueryVirtualMemory(HANDLE, PVOID, ULONG, PVOID, SIZE_T, PSIZE_T);
    NTSTATUS NTAPI ds_NtFlushInstructionCache(HANDLE, PVOID, SIZE_T);
    NTSTATUS NTAPI ds_NtQuerySystemInformation(ULONG, PVOID, ULONG, PULONG);
}

inline HANDLE OpenProcessNt(DWORD access, DWORD pid)
{
    HANDLE h = nullptr;
    OBJECT_ATTRIBUTES oa = {};
    oa.Length = sizeof(oa);
    DS_CLIENT_ID cid = {};
    cid.UniqueProcess = (HANDLE)(uintptr_t)pid;
    NTSTATUS s = ds_NtOpenProcess(&h, access, &oa, &cid);
    return (s >= 0) ? h : nullptr;
}

inline HANDLE OpenThreadNt(DWORD access, DWORD tid)
{
    HANDLE h = nullptr;
    OBJECT_ATTRIBUTES oa = {};
    oa.Length = sizeof(oa);
    DS_CLIENT_ID cid = {};
    cid.UniqueThread = (HANDLE)(uintptr_t)tid;
    if (ds_NtOpenThread(&h, access, &oa, &cid) >= 0)
        return h;
    return nullptr;
}

inline DWORD GetProcessPidNt(HANDLE hProc)
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
    if (ds_NtQueryInformationProcess(hProc, 0, &pbi, sizeof(pbi), nullptr) >= 0)
        return (DWORD)pbi.UniqueProcessId;
    return 0;
}

inline BOOL CloseHandleNt(HANDLE h)
{
    return ds_NtClose(h) >= 0;
}

inline LPVOID VirtualAllocExNt(HANDLE hProc, LPVOID base, SIZE_T size, DWORD allocType, DWORD prot)
{
    PVOID addr = base;
    SIZE_T sz = size;
    return (ds_NtAllocateVirtualMemory(hProc, &addr, 0, &sz, allocType, prot) >= 0) ? addr : nullptr;
}

inline BOOL VirtualFreeExNt(HANDLE hProc, LPVOID base, SIZE_T size, DWORD freeType)
{
    PVOID addr = base;
    SIZE_T sz = size;
    return ds_NtFreeVirtualMemory(hProc, &addr, &sz, freeType) >= 0;
}

inline BOOL VirtualProtectExNt(HANDLE hProc, LPVOID addr, SIZE_T size, DWORD newProt, PDWORD oldProt)
{
    PVOID a = addr;
    SIZE_T sz = size;
    ULONG op = 0;
    NTSTATUS s = ds_NtProtectVirtualMemory(hProc, &a, &sz, newProt, &op);
    if (oldProt)
        *oldProt = op;
    return s >= 0;
}

inline BOOL ReadProcessMemoryNt(HANDLE hProc, LPCVOID addr, LPVOID buf, SIZE_T size, SIZE_T *pRead)
{
    SIZE_T r = 0;
    NTSTATUS s = ds_NtReadVirtualMemory(hProc, (PVOID)addr, buf, size, &r);
    if (pRead)
        *pRead = r;
    return s >= 0;
}

inline BOOL WriteProcessMemoryNt(HANDLE hProc, LPVOID addr, LPCVOID buf, SIZE_T size, SIZE_T *pWritten)
{
    SIZE_T w = 0;
    NTSTATUS s = ds_NtWriteVirtualMemory(hProc, addr, (PVOID)buf, size, &w);
    if (pWritten)
        *pWritten = w;
    return s >= 0;
}

inline SIZE_T VirtualQueryExNt(HANDLE hProc, LPCVOID addr, PMEMORY_BASIC_INFORMATION mbi, SIZE_T cb)
{
    SIZE_T ret = 0;
    return (ds_NtQueryVirtualMemory(hProc, (PVOID)addr, 0, mbi, cb, &ret) >= 0) ? ret : 0;
}

inline BOOL FlushInstructionCacheNt(HANDLE hProc, LPCVOID addr, SIZE_T size)
{
    return ds_NtFlushInstructionCache(hProc, (PVOID)addr, size) >= 0;
}

struct DS_UNICODE_STRING
{
    USHORT Length;
    USHORT MaximumLength;
    PWSTR Buffer;
};

struct DS_SYSTEM_PROCESS_INFORMATION
{
    ULONG NextEntryOffset;
    ULONG NumberOfThreads;
    LARGE_INTEGER Reserved[3];
    LARGE_INTEGER CreateTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER KernelTime;
    DS_UNICODE_STRING ImageName;
    LONG BasePriority;
    HANDLE UniqueProcessId;
    HANDLE InheritedFromUniqueProcessId;
};

template <typename Fn> inline bool EnumProcessesNt(Fn callback)
{
    SIZE_T buf_size = 0x10000;
    PVOID buf = nullptr;
    NTSTATUS s = (NTSTATUS)0xC0000004;

    auto free_buf = [&]()
    {
        if (buf)
        {
            SIZE_T zero = 0;
            ds_NtFreeVirtualMemory(GetCurrentProcess(), &buf, &zero, MEM_RELEASE);
            buf = nullptr;
        }
    };

    for (int retry = 0; retry < 5; ++retry)
    {
        free_buf();
        SIZE_T sz = buf_size;
        s = ds_NtAllocateVirtualMemory(GetCurrentProcess(), &buf, 0, &sz, MEM_COMMIT | MEM_RESERVE,
                                       PAGE_READWRITE);
        if (s < 0)
            return false;
        ULONG ret = 0;
        s = ds_NtQuerySystemInformation(5, buf, (ULONG)buf_size, &ret);
        if (s == 0)
            break;
        if (s == (NTSTATUS)0xC0000004)
        {
            buf_size = (ret + 0x10000) & ~(SIZE_T)0xFFF;
            continue;
        }
        free_buf();
        return false;
    }
    if (s < 0 || !buf)
    {
        free_buf();
        return false;
    }

    auto *p = (DS_SYSTEM_PROCESS_INFORMATION *)buf;
    while (true)
    {
        if (callback(p))
            break;
        if (p->NextEntryOffset == 0)
            break;
        p = (DS_SYSTEM_PROCESS_INFORMATION *)((BYTE *)p + p->NextEntryOffset);
    }
    free_buf();
    return true;
}

uintptr_t GetLocalModuleBase(const wchar_t *dll_name);
uintptr_t GetLocalExport(const wchar_t *module_name, const char *func_name);
uintptr_t GetRemoteModuleBaseNt(HANDLE hProc, const wchar_t *dll_name);
uintptr_t GetRemoteExportNt(HANDLE hProc, const wchar_t *mod, const char *export_name);

} // namespace ds
