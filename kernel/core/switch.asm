%define THREAD_ESP0_OFFSET         0
%define THREAD_ESP_OFFSET          4
%define THREAD_CR3_OFFSET          8
%define TASK_STATE_SEG_ESP0_OFFSET 4

extern gdt_start
extern gdt_user_data_seg_desc

global kernel_thread_entry_trampoline
kernel_thread_entry_trampoline:
    iret

global user_thread_entry_trampoline
user_thread_entry_trampoline:
    mov eax, gdt_user_data_seg_desc
    sub eax, gdt_start
    or  al, 3
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    iret

global thread_switch
thread_switch:
    push ebx
    push esi
    push edi
    push ebp

    extern running
    mov eax, [running]
    mov [eax + THREAD_ESP_OFFSET], esp

    mov eax, [esp + 5 * 4]
    mov [running], eax

    extern task_state_seg
    mov esi, [eax + THREAD_ESP0_OFFSET]
    mov [task_state_seg + TASK_STATE_SEG_ESP0_OFFSET], esi

    mov ebx, [eax + THREAD_CR3_OFFSET]
    mov ecx, cr3
    cmp ebx, ecx
    je .done
    mov cr3, ebx

.done:
    mov esp, [eax + THREAD_ESP_OFFSET]
    pop ebp
    pop edi
    pop esi
    pop ebx
    ret
