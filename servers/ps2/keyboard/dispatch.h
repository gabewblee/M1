#pragma once

#include "uapi/uapi.h"

/**
 * init - Initializes the PS/2 keyboard server.
 * Returns: E_OK on success, -E_FAULT on failure.
 */
i32 init(void);

/**
 * dispatch - Dispatches @msg to the PS/2 keyboard server.
 * @msg: The IPC message to dispatch.
 * Returns: E_OK on success, -E_INVAL on invalid argument, 
 *          -E_NOSYS on invalid operation.
 */
i32 dispatch(ipc_msg_t* msg);

/**
 * fini - Finalizes the PS/2 keyboard server. Currently no-op.
 */
void fini(void);