#pragma once

#include <userspace/fs/lib/fs.h>

/**
 * fs_server_main - Runs a filesystem server speaking the FS wire protocol,
 *                  translating requests into @driver callbacks. Never returns.
 * @driver: The filesystem driver to serve.
 */
void fs_server_main(const fs_driver_s* driver);
