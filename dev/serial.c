#include <dev/serial.h>
#include <stddef.h>
#include <uapi/io.h>

#define COM1            0x3F8u /* COM1 base I/O port              */
#define COM1_DATA       COM1   /* Data register                   */
#define COM1_INT_ENABLE COM1+1 /* Interrupt enable register       */
#define COM1_FIFO_CTRL  COM1+2 /* FIFO control register           */
#define COM1_LINE_CTRL  COM1+3 /* Line control register           */
#define COM1_MODEM_CTRL COM1+4 /* Modem control register          */
#define COM1_LINE_STAT  COM1+5 /* Line status register            */
#define LINE_STAT_THRE  0x20u  /* Transmit holding register empty */

static void wait_tx_ready(void) {
    while ((inb(COM1_LINE_STAT) & LINE_STAT_THRE) == 0);
}

void serial_putc(char c) {
    if (c == '\n') {
        wait_tx_ready();
        outb(COM1_DATA, '\r');
    }

    wait_tx_ready();
    outb(COM1_DATA, (u8)c);
}

void serial_puts(char* str) {
    for (size_t i = 0; str[i]; i++)
        serial_putc(str[i]);
}

void serial_write(char* buf, size_t len) {
    for (size_t i = 0; i < len; i++)
        serial_putc(buf[i]);
}

void serial_clear(void) {
    ;
}

void serial_init(void) {
    outb(COM1_INT_ENABLE, 0x00);
    outb(COM1_LINE_CTRL,  0x80);
    outb(COM1_DATA,       0x03);
    outb(COM1_INT_ENABLE, 0x00);
    outb(COM1_LINE_CTRL,  0x03);
    outb(COM1_FIFO_CTRL,  0xC7);
    outb(COM1_MODEM_CTRL, 0x0B);
}
