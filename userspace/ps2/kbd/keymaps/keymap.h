#pragma once

#include <stdbool.h>

#include "uapi/kbd.h"

typedef struct keymap_s {
    bool (*decode)(u8 code, kbd_event_s* event); /* Converts a scancode to a keyboard event   */
    char (*ascii)(u8 code, bool shifted);               /* Converts a scancode to an ASCII character */
} keymap_s;

extern const keymap_s keymap1;
extern const keymap_s keymap2;
extern const keymap_s keymap3;