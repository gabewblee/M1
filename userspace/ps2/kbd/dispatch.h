#pragma once

#include "uapi/uapi.h"

/**
 * init - Initializes PS/2 keyboard server.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 init(void);

/**
 * dispatch - Dispatches @msg to the PS/2 keyboard server.
 * @msg: The IPC message to dispatch.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 dispatch(ipc_msg_s* msg);

/**
 * fini - Finalizes PS/2 keyboard server. Currently a no-op.
 */
void fini(void);