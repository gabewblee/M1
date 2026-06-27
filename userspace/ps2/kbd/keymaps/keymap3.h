#pragma once

#include <stdbool.h>
#include <uapi/kbd.h>
#include <uapi/types.h>

/**
 * decode3 - Decodes @code into @event.
 * @code: The keymap 3 code.
 * @event: The output keyboard event.
 * Returns: true if @event is valid, false otherwise.
 */
bool decode3(u8 code, kbd_event_s* event);

/**
 * ascii3 - Returns the ASCII character for @code.
 * @code: The keymap 3 code.
 * @shifted: Whether the shift key is pressed.
 * Returns: The ASCII character for @code.
 */
char ascii3(u8 code, bool shifted);