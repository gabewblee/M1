bits 32

%define IDT_EXC_CNT     32
%define IDT_IRQ_CNT     16
%define IDT_STUB_STRIDE 15

extern int_handler
extern syscall_handler

isr_handler_common:
    pushad
    cld
    push esp
    call int_handler
    add esp, 4
    popad
    add esp, 8
    iret

%macro isr_no_err_stub 1
    push strict dword 0         ; 5 bytes
    push strict dword %1        ; 5 bytes
    jmp near isr_handler_common ; 5 bytes
%endmacro

%macro isr_err_stub 1
%%start_%1:
    push strict dword %1                           ; 5 bytes
    jmp near isr_handler_common                    ; 5 bytes
    times (IDT_STUB_STRIDE - ($ - %%start_%1)) nop ; Padding
%endmacro

; Vectors 8, 10-14, 17, and 30 push an error code
%macro isr_stub_for 1
    %if %1 = 8 || %1 = 10 || %1 = 11 || %1 = 12 || %1 = 13 || %1 = 14 || %1 = 17 || %1 = 30
        isr_err_stub %1
    %else
        isr_no_err_stub %1
    %endif
%endmacro

global isr_stub_base
isr_stub_base:
%assign vector 0
%rep IDT_EXC_CNT + IDT_IRQ_CNT
    isr_stub_for vector
    %assign vector vector + 1
%endrep

global syscall_stub
syscall_stub:
    push eax
    push edi
    push esi
    push edx
    push ecx
    push ebx
    push ebp
    lea eax, [esp]
    push eax
    call syscall_handler
    add esp, 4
    pop ebp
    pop ebx
    pop ecx
    pop edx
    pop esi
    pop edi
    add esp, 4
    iret