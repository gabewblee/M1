#pragma once

#include "uapi/uapi.h"

/**
 * init - Initializes the VGA server. Maps VGA memory.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 init(void);

/**
 * dispatch - Dispatches @msg to the VGA server.
 * @msg: The IPC message to dispatch.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 dispatch(ipc_msg_s* msg);

/**
 * fini - Finalizes the VGA server. Unmaps VGA memory.
 */
void fini(void);