#pragma once

#define SYS_send        0u  /* Syscall 0:  Blocking send on an endpoint                                     */
#define SYS_nbsend      1u  /* Syscall 1:  Non-blocking (polling) send                                      */
#define SYS_call        2u  /* Syscall 2:  Send then block for a reply, or invoke a non-endpoint capability */
#define SYS_recv        3u  /* Syscall 3:  Blocking receive on an endpoint                                  */
#define SYS_nbrecv      4u  /* Syscall 4:  Non-blocking (polling) receive                                   */
#define SYS_reply       5u  /* Syscall 5:  Reply via a reply capability                                     */
#define SYS_replyrecv   6u  /* Syscall 6:  Reply then receive again                                         */
#define SYS_signal      7u  /* Syscall 7:  Signal a notification                                            */
#define SYS_wait        8u  /* Syscall 8:  Wait on a notification                                           */
#define SYS_yield       9u  /* Syscall 9:  Yield the CPU                                                    */
#define SYS_thread_exit 10u /* Syscall 10: Exit the running thread                                          */
#define SYS_dbg_puts    11u /* Syscall 11: Write a string to the debug console                              */
#define SYS_max         12u /* Syscall 12: Maximum syscall number                                           */
