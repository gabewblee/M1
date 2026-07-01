#pragma once

#include <types.h>

#define IPC_PAYLOAD_SZ 64u            /* Message register bytes */
#define IPC_PACKET_SZ  IPC_PAYLOAD_SZ /* IPC packet size        */

typedef struct ipc_packet_s {
    u8 payload[IPC_PAYLOAD_SZ]; /* Message registers */
} ipc_packet_s;

_Static_assert(sizeof(ipc_packet_s) == IPC_PACKET_SZ, "Error: Invalid IPC packet size");

#define MSG_LEN_SHIFT        0u
#define MSG_LEN_MASK         0x7Fu
#define MSG_CAPABILITY_SHIFT 7u
#define MSG_CAPABILITY_MASK  0x3u
#define MSG_LABEL_SHIFT      12u
#define MSG_LABEL_MASK       0x7FFFFu

typedef u32 msg_info_t;

/**
 * mk_msg_info - Creates a message tag.
 * @label: The message label.
 * @capability: The capability bit.
 * @len: The message length in bytes.
 */
static inline msg_info_t mk_msg_info(u32 label, u32 capability, u32 len) {
    return ((label      & MSG_LABEL_MASK)      << MSG_LABEL_SHIFT)      |
           ((capability & MSG_CAPABILITY_MASK) << MSG_CAPABILITY_SHIFT) |
           ((len        & MSG_LEN_MASK)        << MSG_LEN_SHIFT);
}

/**
 * get_msg_label - Extracts the message label from @mi.
 * @mi: The message tag.
 * Returns: The message label.
 */
static inline u32 get_msg_label(msg_info_t mi) {
    return (mi >> MSG_LABEL_SHIFT) & MSG_LABEL_MASK;
}

/**
 * get_msg_capability - Extracts the capability bit from @mi.
 * @mi: The message tag.
 * Returns: The capability bit.
 */
static inline u32 get_msg_capability(msg_info_t mi) {
    return (mi >> MSG_CAPABILITY_SHIFT) & MSG_CAPABILITY_MASK;
}

/**
 * get_msg_len - Extracts the message length from @mi.
 * @mi: The message tag.
 * Returns: The message length in bytes.
 */
static inline u32 get_msg_len(msg_info_t mi) {
    return (mi >> MSG_LEN_SHIFT) & MSG_LEN_MASK;
}
