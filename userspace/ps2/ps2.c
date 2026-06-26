#include <uapi/io.h>
#include <userspace/ps2/ps2.h>

#define PS2_DATA_PORT           0x60 /* PS/2 data port             */
#define PS2_STATUS_PORT         0x64 /* PS/2 status port           */
#define PS2_STATUS_OUT_BUF_FULL 0x01 /* PS/2 output buffer is full */

bool ps2_data_ready(void) {
    return (inb(PS2_STATUS_PORT) & PS2_STATUS_OUT_BUF_FULL) != 0;
}

u8 ps2_read_data(void) {
    return inb(PS2_DATA_PORT);
}