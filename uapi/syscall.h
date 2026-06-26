#pragma once

#define SYS_ipc_send             0u  /* Syscall 0:  Sends an IPC message                       */
#define SYS_ipc_recv             1u  /* Syscall 1:  Receives an IPC message                    */
#define SYS_ipc_call             2u  /* Syscall 2:  Sends an IPC message and waits for a reply */
#define SYS_ipc_reply            3u  /* Syscall 3:  Replies to an IPC message                  */
#define SYS_thread_create        4u  /* Syscall 4:  Creates a new thread                       */
#define SYS_thread_yield         5u  /* Syscall 5:  Yields the CPU                             */
#define SYS_thread_exit          6u  /* Syscall 6:  Exits the running thread                   */
#define SYS_map_pg               7u  /* Syscall 7:  Maps a page                                */
#define SYS_unmap_pg             8u  /* Syscall 8:  Unmaps a page                              */
#define SYS_vm_alloc_range       9u  /* Syscall 9:  Allocates a virtual range                  */
#define SYS_vm_free_range        10u /* Syscall 10: Frees a virtual range                      */
#define SYS_log_read             11u /* Syscall 11: Reads the kernel log                       */
#define SYS_server_lookup        12u /* Syscall 12: Looks up a server by ID                    */
#define SYS_irq_register_handler 13u /* Syscall 13: Registers an IRQ handler                   */
#define SYS_irq_wait_for         14u /* Syscall 14: Waits for an IRQ to fire                   */
#define SYS_vm_copy              15u /* Syscall 15: Transfers bytes across address spaces      */
#define SYS_max                  16u /* Syscall 16: Maximum syscall number                     */

/* VM_COPY_* constants for SYS_vm_copy */
#define VM_COPY_FROM_PEER        0u  /* Copy from peer into caller                             */
#define VM_COPY_TO_PEER          1u  /* Copy from caller into peer                             */
