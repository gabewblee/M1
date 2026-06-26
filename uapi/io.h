#pragma once

#include <compiler.h>
#include <types.h>

/**
 * inb - Reads a byte from an I/O port.
 * @port: The port to read from.
 * Returns: The byte read.
 */
static __always_inline u8 inb(u16 port) {
    u8 ret;
    __asm__ volatile ("inb %w1, %b0" : "=a"(ret) : "Nd"(port) : "memory");
    return ret;
}

/**
 * outb - Writes a byte to an I/O port.
 * @port: The port to write to.
 * @val: The byte to write.
 */
static __always_inline void outb(u16 port, u8 val) {
    __asm__ volatile ("outb %b0, %w1" : : "a"(val), "Nd"(port) : "memory");
}

/**
 * inw - Reads a word from an I/O port.
 * @port: The port to read from.
 * Returns: The word read.
 */
static __always_inline u16 inw(u16 port) {
    u16 ret;
    __asm__ volatile ("inw %w1, %w0" : "=a"(ret) : "Nd"(port) : "memory");
    return ret;
}

/**
 * outw - Writes a word to an I/O port.
 * @port: The port to write to.
 * @val: The word to write.
 */
static __always_inline void outw(u16 port, u16 val) {
    __asm__ volatile ("outw %w0, %w1" : : "a"(val), "Nd"(port) : "memory");
}

/**
 * io_wait - Waits for an I/O operation to complete.
 */
static __always_inline void io_wait(void) {
    outb(0x80, 0);
}
