#pragma once

#include "uapi/kbd.h"

/**
 * ps2_kbd_read - Blocks until the next keyboard event is available.
 * @event: The output keyboard event.
 */
void ps2_kbd_read(kbd_event_s* event);

/**
 * ps2_kbd_init - Initializes the PS/2 keyboard server. Registers the
 *                keyboard IRQ.
 */
void ps2_kbd_init(void);