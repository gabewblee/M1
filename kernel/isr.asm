bits 32

extern exception_handler
extern irq_handler

%macro isr_no_err_stub 1
isr_stub_%+%1:
    push dword 0  ; Dummy error code
    push dword %1 ; Interrupt number
    jmp isr_handler_common
%endmacro

%macro isr_err_stub 1
isr_stub_%+%1:
    push dword %1 ; Interrupt number
    jmp isr_handler_common
%endmacro

%macro irq_stub 2
irq_stub_%+%1:
    push dword 0  ; Dummy error code
    push dword %2 ; Interrupt vector number
    jmp irq_handler_common
%endmacro

isr_handler_common:
    pushad
    cld
    push esp
    call exception_handler
    add esp, 4
    popad
    add esp, 8
    iret

irq_handler_common:
    pushad
    cld
    push esp
    call irq_handler
    add esp, 4
    popad
    add esp, 8
    iret

isr_no_err_stub 0
isr_no_err_stub 1
isr_no_err_stub 2
isr_no_err_stub 3
isr_no_err_stub 4
isr_no_err_stub 5
isr_no_err_stub 6
isr_no_err_stub 7
isr_err_stub    8
isr_no_err_stub 9
isr_err_stub    10
isr_err_stub    11
isr_err_stub    12
isr_err_stub    13
isr_err_stub    14
isr_no_err_stub 15
isr_no_err_stub 16
isr_err_stub    17
isr_no_err_stub 18
isr_no_err_stub 19
isr_no_err_stub 20
isr_no_err_stub 21
isr_no_err_stub 22
isr_no_err_stub 23
isr_no_err_stub 24
isr_no_err_stub 25
isr_no_err_stub 26
isr_no_err_stub 27
isr_no_err_stub 28
isr_no_err_stub 29
isr_err_stub    30
isr_no_err_stub 31

%assign i 0
%rep 16
    irq_stub i, i + 32
%assign i i+1
%endrep

global isr_stub_table
isr_stub_table:
%assign i 0
%rep 32
    dd isr_stub_%+i
%assign i i+1
%endrep

global irq_stub_table
irq_stub_table:
%assign i 0
%rep 16
    dd irq_stub_%+i
%assign i i+1
%endrep