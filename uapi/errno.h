#pragma once

#define E_OK       0u  /* Success                   */
#define E_INVAL    1u  /* Invalid argument          */
#define E_NOMEM    2u  /* Allocation failure        */
#define E_NOSYS    3u  /* Unsupported operation     */
#define E_AGAIN    4u  /* Temporarily unavailable   */
#define E_FAULT    5u  /* Invalid address           */
#define E_PERM     6u  /* Invalid permissions       */
#define E_NOENT    7u  /* No such file or directory */
#define E_EXIST    8u  /* File exists               */
#define E_NOTDIR   9u  /* Not a directory           */
#define E_ISDIR    10u /* Is a directory            */
#define E_NOTEMPTY 11u /* Directory not empty       */
#define E_BADF     12u /* Bad file descriptor       */
#define E_MFILE    13u /* Too many open files       */
#define E_LOOP     14u /* Too many symbolic links   */
#define E_NAMELONG 15u /* Name too long             */
#define E_XDEV     16u /* Cross-device link         */
#define E_NODEV    17u /* No such device            */
#define E_BUSY     18u /* Resource busy             */
#define E_NOSPC    19u /* No space left on device   */
#define E_IO       20u /* Input/output error        */
