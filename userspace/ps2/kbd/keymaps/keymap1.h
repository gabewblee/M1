#pragma once

#include <stdbool.h>

#include "uapi/kbd.h"

/**
 * decode1 - Decodes @code into @event.
 * @code: The keymap 1 code.
 * @event: The output keyboard event.
 * Returns: true if @event is valid, false otherwise.
 */
bool decode1(u8 code, kbd_event_s* event);

/**
 * ascii1 - Returns the ASCII character for @code.
 * @code: The keymap 1 code.
 * @shifted: Whether the shift key is pressed.
 * Returns: The ASCII character for @code.
 */
char ascii1(u8 code, bool shifted);