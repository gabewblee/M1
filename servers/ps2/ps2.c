#include "io/io.h"
#include "ps2.h"

#define PS2_DATA_PORT 0x60 /* PS/2 data port */

u8 ps2_read_data(void) {
    return inb(PS2_DATA_PORT);
}