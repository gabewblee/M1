; These offsets are susceptible to compiler padding
%define TASK_CTRL_BLK_ESP0_OFFSET  0
%define TASK_CTRL_BLK_ESP_OFFSET   4
%define TASK_CTRL_BLK_CR3_OFFSET   8
%define TASK_STATE_SEG_ESP0_OFFSET 4

global task_entry_trampoline
task_entry_trampoline:
    iret

global task_switch
task_switch:
    ; Save the current task's context
    push ebx
    push esi
    push edi
    push ebp

    extern cur_running_task
    mov eax, [cur_running_task]
    mov [eax + TASK_CTRL_BLK_ESP_OFFSET], esp

    ; Load the next task's context
    mov eax, [esp + 5 * 4]
    mov [cur_running_task], eax
    mov esi, [eax + TASK_CTRL_BLK_ESP0_OFFSET]

    extern task_state_seg
    mov [task_state_seg + TASK_STATE_SEG_ESP0_OFFSET], esi

    mov ebx, [eax + TASK_CTRL_BLK_CR3_OFFSET]
    mov ecx, cr3
    cmp ebx, ecx
    je .done
    mov cr3, ebx

.done:
    mov esp, [eax + TASK_CTRL_BLK_ESP_OFFSET]
    pop ebp
    pop edi
    pop esi
    pop ebx
    ret