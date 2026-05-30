#pragma once

#define KERNEL_TASK_ID    0u /* Kernel task ID                                        */

#define SYS_ipc_send      0u /* Syscall 0: Sends an IPC message                       */
#define SYS_ipc_recv      1u /* Syscall 1: Receives an IPC message                    */
#define SYS_ipc_call      2u /* Syscall 2: Sends an IPC message and waits for a reply */
#define SYS_ipc_reply     3u /* Syscall 3: Replies to an IPC message                  */
#define SYS_thread_create 4u /* Syscall 4: Creates a new thread                       */
#define SYS_thread_yield  5u /* Syscall 5: Yields the CPU                             */
#define SYS_thread_exit   6u /* Syscall 6: Exits the running thread                   */
#define SYS_map_pg        7u /* Syscall 7: Maps a page                                */
#define SYS_unmap_pg      8u /* Syscall 8: Unmaps a page                              */
#define SYS_max           9u /* Syscall 9: Maximum syscall number                     */