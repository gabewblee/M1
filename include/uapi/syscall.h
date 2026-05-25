#pragma once

#define KERNEL_TASK_ID    0 /* Kernel task ID          */

#define SYS_ipc_send      0 /* System call 0           */
#define SYS_ipc_recv      1 /* System call 1           */
#define SYS_ipc_call      2 /* System call 2           */
#define SYS_ipc_reply     3 /* System call 3           */
#define SYS_thread_create 4 /* System call 4           */
#define SYS_thread_yield  5 /* System call 5           */
#define SYS_thread_exit   6 /* System call 6           */
#define SYS_map_pg        7 /* System call 7           */
#define SYS_unmap_pg      8 /* System call 8           */
#define SYS_max           9 /* System call 9           */

#define E_OK              0 /* Success                 */
#define E_INVAL           1 /* Invalid argument        */
#define E_NOMEM           2 /* Allocation failure      */
#define E_NOSYS           3 /* Unsupported operation   */
#define E_AGAIN           4 /* Temporarily unavailable */
#define E_FAULT           5 /* Invalid address         */
#define E_PERM            6 /* Invalid permissions     */