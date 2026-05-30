#pragma once

#include <stdbool.h>

#include "uapi/kbd.h"

/**
 * decode2 - Decodes @code into @event.
 * @code: The keymap 2 code.
 * @event: The output keyboard event.
 * Returns: true if @event is valid, false otherwise.
 */
bool decode2(u8 code, kbd_event_s* event);

/**
 * ascii2 - Returns the ASCII character for @code.
 * @code: The keymap 2 code.
 * @shifted: Whether the shift key is pressed.
 * Returns: The ASCII character for @code.
 */
char ascii2(u8 code, bool shifted);