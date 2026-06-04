#pragma once

#include "uapi/uapi.h"

/**
 * init - Initializes ATA server.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 init(void);

/**
 * dispatch - Dispatches @msg to the ATA server.
 * @msg: The IPC message to dispatch.
 * Returns: E_OK on success, or a negative error code on failure.
 */
i32 dispatch(ipc_msg_s* msg);

/**
 * fini - Finalizes ATA server. Currently a no-op.
 */
void fini(void);