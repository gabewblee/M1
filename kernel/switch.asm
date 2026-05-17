%define THREAD_ESP0_OFFSET         0
%define THREAD_ESP_OFFSET          4
%define THREAD_CR3_OFFSET          8
%define TASK_STATE_SEG_ESP0_OFFSET 4

global thread_entry_trampoline
thread_entry_trampoline:
    iret

global thread_switch
thread_switch:
    push ebx
    push esi
    push edi
    push ebp

    extern cur_running_thread
    mov eax, [cur_running_thread]
    mov [eax + THREAD_ESP_OFFSET], esp

    mov eax, [esp + 5 * 4]
    mov [cur_running_thread], eax

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
