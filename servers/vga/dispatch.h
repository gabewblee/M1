#pragma once

#include "uapi.h"

/**
 * init - Initializes the VGA server.
 * Returns: E_OK on success, -E_FAULT on failure.
 */
i32 init(void);

/**
 * dispatch - Dispatches @msg to the VGA server.
 * @msg: The IPC message to dispatch.
 * Returns: E_OK on success, -E_INVAL on invalid argument, 
 *          -E_NOSYS on invalid operation.
 */
i32 dispatch(ipc_msg_t* msg);

/**
 * fini - Finalizes the VGA server. Currently no-op.
 */
void fini(void);