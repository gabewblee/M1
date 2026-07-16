#pragma once

#define SYS_send        0u  /* Syscall 0:  Blocking send                       */
#define SYS_nbsend      1u  /* Syscall 1:  Non-blocking send                   */
#define SYS_call        2u  /* Syscall 2:  Send then wait for reply            */
#define SYS_recv        3u  /* Syscall 3:  Blocking receive                    */
#define SYS_nbrecv      4u  /* Syscall 4:  Non-blocking receive                */
#define SYS_reply       5u  /* Syscall 5:  Reply                               */
#define SYS_replyrecv   6u  /* Syscall 6:  Reply then receive                  */
#define SYS_signal      7u  /* Syscall 7:  Signal a notification               */
#define SYS_wait        8u  /* Syscall 8:  Wait on a notification              */
#define SYS_yield       9u  /* Syscall 9:  Yield the CPU                       */
#define SYS_thread_exit 10u /* Syscall 10: Exit the running thread             */
#define SYS_dbg_puts    11u /* Syscall 11: Write a string to the debug console */
#define SYS_max         12u /* Syscall 12: Maximum syscall number              */
