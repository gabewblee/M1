#pragma once

#define CAPABILITY_INVOKE_ARGC          8u  /* Invocation argument count          */

#define CAPABILITY_LABEL_UNTYPED_RETYPE 1u  /* Untyped: retype into objects       */
#define CAPABILITY_LABEL_CNODE_COPY     2u  /* CNode: copy slot to slot           */
#define CAPABILITY_LABEL_CNODE_MINT     3u  /* CNode: mint derived slot to slot   */
#define CAPABILITY_LABEL_CNODE_MOVE     4u  /* CNode: move slot to slot           */
#define CAPABILITY_LABEL_CNODE_DELETE   5u  /* CNode: delete a slot               */
#define CAPABILITY_LABEL_CNODE_REVOKE   6u  /* CNode: revoke a slot's descendants */
#define CAPABILITY_LABEL_TCB_CONFIGURE  7u  /* TCB: set CSpace/VSpace/entry/prio  */
#define CAPABILITY_LABEL_TCB_RESUME     8u  /* TCB: make schedulable              */
#define CAPABILITY_LABEL_PAGE_MAP       9u  /* Frame: map into the address space  */
#define CAPABILITY_LABEL_IRQ_GET        10u /* IRQ control: create handler cap    */
#define CAPABILITY_LABEL_IRQ_SET_NTFN   11u /* IRQ handler: bind a notification   */
#define CAPABILITY_LABEL_PAGE_UNMAP     12u /* Frame: unmap from the address space */
#define CAPABILITY_LABEL_PAGE_TABLE_MAP 13u /* Page table: install into a VSpace   */

/* Capability types, shared by the kernel and userspace */
#define CAPABILITY_TYPE_NULL            0u  /* Empty slot                         */
#define CAPABILITY_TYPE_UNTYPED         1u  /* Untyped memory                     */
#define CAPABILITY_TYPE_CNODE           2u  /* Capability node                    */
#define CAPABILITY_TYPE_TCB             3u  /* Thread control block               */
#define CAPABILITY_TYPE_ENDPOINT        4u  /* Synchronous IPC endpoint           */
#define CAPABILITY_TYPE_NOTIFICATION    5u  /* Asynchronous signal                */
#define CAPABILITY_TYPE_FRM             6u  /* Physical memory frame              */
#define CAPABILITY_TYPE_PG_TABLE        7u  /* Page table                         */
#define CAPABILITY_TYPE_PG_DIR          8u  /* Page directory                     */
#define CAPABILITY_TYPE_IRQ_CONTROL     9u  /* IRQ allocator                      */
#define CAPABILITY_TYPE_IRQ_HANDLER     10u /* Single IRQ line                    */
#define CAPABILITY_TYPE_REPLY           11u /* Single-use reply                   */

/* Capability access rights */
#define CAPABILITY_RIGHT_READ           (1u << 0)
#define CAPABILITY_RIGHT_WRITE          (1u << 1)
#define CAPABILITY_RIGHT_GRANT          (1u << 2)
#define CAPABILITY_RIGHT_GRANTREPLY     (1u << 3)
#define CAPABILITY_RIGHTS_ALL           (CAPABILITY_RIGHT_READ | CAPABILITY_RIGHT_WRITE | CAPABILITY_RIGHT_GRANT | CAPABILITY_RIGHT_GRANTREPLY)
