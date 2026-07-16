#pragma once

#define CAPABILITY_INVOKE_ARGC          8u  /* Invocation argument count           */

/* Capability labels */
#define CAPABILITY_LABEL_UNTYPED_RETYPE 1u  /* Untyped: retype into objects        */
#define CAPABILITY_LABEL_CNODE_CP       2u  /* CNode: copy a slot                  */
#define CAPABILITY_LABEL_CNODE_MINT     3u  /* CNode: mint a slot                  */
#define CAPABILITY_LABEL_CNODE_MV       4u  /* CNode: move a slot                  */
#define CAPABILITY_LABEL_CNODE_DEL      5u  /* CNode: delete a slot                */
#define CAPABILITY_LABEL_CNODE_RVK      6u  /* CNode: revoke a slot's descendants  */
#define CAPABILITY_LABEL_TCB_CONFIGURE  7u  /* TCB: set CSpace/VSpace/entry/prio   */
#define CAPABILITY_LABEL_TCB_RESUME     8u  /* TCB: make schedulable               */
#define CAPABILITY_LABEL_IRQ_GET        9u  /* IRQ control: create handler cap     */
#define CAPABILITY_LABEL_IRQ_SET_NTFN   10u /* IRQ handler: bind a notification    */
#define CAPABILITY_LABEL_MAP_PG         11u /* Frame: map into the address space   */
#define CAPABILITY_LABEL_MAP_PG_TABLE   12u /* Page table: install into a VSpace   */
#define CAPABILITY_LABEL_UNMAP_PG       13u /* Frame: unmap from the address space */

/* Capability types  */
#define CAPABILITY_TYPE_NULL            0u  /* Empty slot                          */
#define CAPABILITY_TYPE_UNTYPED         1u  /* Untyped memory                      */
#define CAPABILITY_TYPE_CNODE           2u  /* Capability node                     */
#define CAPABILITY_TYPE_TCB             3u  /* Thread control block                */
#define CAPABILITY_TYPE_ENDPOINT        4u  /* Synchronous IPC endpoint            */
#define CAPABILITY_TYPE_NTFN            5u  /* Asynchronous signal                 */
#define CAPABILITY_TYPE_FRM             6u  /* Physical memory frame               */
#define CAPABILITY_TYPE_PG_TABLE        7u  /* Page table                          */
#define CAPABILITY_TYPE_PG_DIR          8u  /* Page directory                      */
#define CAPABILITY_TYPE_IRQ_CTRL        9u  /* IRQ allocator                       */
#define CAPABILITY_TYPE_IRQ_HANDLER     10u /* Single IRQ line                     */
#define CAPABILITY_TYPE_REPLY           11u /* Single-use reply                    */

/* Capability rights */
#define CAPABILITY_RIGHT_READ           (1u << 0)
#define CAPABILITY_RIGHT_WRITE          (1u << 1)
#define CAPABILITY_RIGHT_GRANT          (1u << 2)
#define CAPABILITY_RIGHT_GRANTREPLY     (1u << 3)
#define CAPABILITY_RIGHTS_ALL           (CAPABILITY_RIGHT_READ | CAPABILITY_RIGHT_WRITE | CAPABILITY_RIGHT_GRANT | CAPABILITY_RIGHT_GRANTREPLY)
