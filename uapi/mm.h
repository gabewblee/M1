#pragma once

#define PG_SZ           4096u  /* Page size               */
#define PG_SHIFT        12u    /* log2(page size)         */
#define PG_PRESENT_FLAG 0x001u /* Page is present         */
#define PG_RW_FLAG      0x002u /* Page is writable        */
#define PG_USER_FLAG    0x004u /* Page is user-accessible */
#define PG_GLOBAL_FLAG  0x100u /* Page is global          */
#define PG_ENTRY_CNT    1024u  /* Page table entry count  */
