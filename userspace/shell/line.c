#include <uapi/kbd.h>
#include <uapi/servers.h>
#include <userspace/libc/stdio.h>
#include <userspace/libc/string.h>
#include <userspace/libc/syscall.h>
#include <userspace/ps2/kbd/keymaps/keymap1.h>
#include <userspace/shell/line.h>

#define KEY_LSHIFT 0x2Au
#define KEY_RSHIFT 0x36u
#define KEY_CTRL   0x1Du
#define KEY_CAPS   0x3Au

typedef struct modifiers_s {
    u8 shift; /* Held shift key count      */
    u8 ctrl;  /* Whether control is held   */
    u8 caps;  /* Whether caps lock latches */
} modifiers_s;

static modifiers_s mods;

static i32 kbd_read(kbd_event_s* event) {
    ipc_msg_s msg;
    i32 mi = sys_call(SERVICE_CPTR_KBD, mk_msg_info(KBD_SERVER_OP_read, 0, 0), &msg, 0);
    if (mi < 0)
        return mi;

    kbd_server_reply_s reply;
    if (get_msg_len((msg_info_t)mi) < sizeof(reply))
        return -(i32)E_FAULT;

    memcpy(&reply, msg.payload, sizeof(reply));
    if (reply.ret != E_OK)
        return reply.ret;

    *event = reply.event;
    return E_OK;
}

static int track_modifiers(const kbd_event_s* event) {
    switch (event->code) {
        case KEY_LSHIFT:
        case KEY_RSHIFT:
            if (event->pressed)
                mods.shift++;
            else if (mods.shift)
                mods.shift--;
            return 1;

        case KEY_CTRL:
            mods.ctrl = event->pressed;
            return 1;

        case KEY_CAPS:
            if (event->pressed)
                mods.caps ^= 1u;
            return 1;

        default:
            return 0;
    }
}

static char event_to_ascii(const kbd_event_s* event) {
    if (event->extended)
        return 0;

    char c = ascii1(event->code, mods.shift != 0);
    if (mods.caps && !mods.shift && c >= 'a' && c <= 'z')
        c = (char)(c - 'a' + 'A');
    else if (mods.caps && mods.shift && c >= 'A' && c <= 'Z')
        c = (char)(c - 'A' + 'a');

    return c;
}

i32 line_read(char* buffer, u32 size) {
    u32 len = 0;
    for (;;) {
        kbd_event_s event;
        i32 ret = kbd_read(&event);
        if (ret != E_OK)
            return ret;

        if (track_modifiers(&event) || !event.pressed)
            continue;

        char c = event_to_ascii(&event);
        if (!c)
            continue;

        if (mods.ctrl) {
            if ((c == 'u' || c == 'U') && len) {
                while (len) {
                    write("\b \b", 3);
                    len--;
                }
            }

            continue;
        }

        if (c == '\b') {
            if (len) {
                write("\b \b", 3);
                len--;
            }

            continue;
        }

        if (c == '\n') {
            putc('\n');
            buffer[len] = '\0';
            return (i32)len;
        }

        if (c == '\t')
            c = ' ';

        if (len + 1u < size) {
            buffer[len++] = c;
            putc(c);
        }
    }
}
