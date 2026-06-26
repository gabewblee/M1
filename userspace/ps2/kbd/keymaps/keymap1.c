#include <userspace/ps2/kbd/keymaps/keymap1.h>

/* ASCII table size */
#define MAP_SZ                  128u

/* Decoder states */
#define IDLE                    0u
#define EXT                     1u
#define P5                      2u
#define P4                      3u
#define P3                      4u
#define P2                      5u
#define P1                      6u

/* Transition flags */
#define BRK                     0x80u
#define EMIT                    0x08u
#define EXTENDED                0x10u

/* Transition values */
#define TRANSITION(state, flag) ((u8)((state) | (flag)))
#define NONE                    TRANSITION(IDLE, 0)
#define KEY                     TRANSITION(IDLE, EMIT)
#define XKEY                    TRANSITION(IDLE, EMIT | EXTENDED)

/* Transition table */
#define ROW(name, def, ...)  \
    static u8 name[256] = {  \
        [0 ... 255] = (def), \
        __VA_ARGS__          \
    }

#define IGN8                                                    \
    [0x00] = NONE, [0xAA] = NONE, [0xEE] = NONE, [0xFA] = NONE, \
    [0xFC] = NONE, [0xFD] = NONE, [0xFE] = NONE, [0xFF] = NONE

ROW(idle, KEY, IGN8, [0xE0] = TRANSITION(EXT, 0), [0xE1] = TRANSITION(P5, 0));
ROW(ext, XKEY, IGN8);
ROW(p5, TRANSITION(P4, 0));
ROW(p4, TRANSITION(P3, 0));
ROW(p3, TRANSITION(P2, 0));
ROW(p2, TRANSITION(P1, 0));
ROW(p1, NONE);

static u8* table[] = {
    [IDLE] = idle,
    [EXT]  = ext,
    [P5]   = p5,
    [P4]   = p4,
    [P3]   = p3,
    [P2]   = p2,
    [P1]   = p1
};

static char keymap1_to_unshifted[MAP_SZ] = {
    [0x02] = '1',  [0x03] = '2',  [0x04] = '3',
    [0x05] = '4',  [0x06] = '5',  [0x07] = '6',
    [0x08] = '7',  [0x09] = '8',  [0x0A] = '9',
    [0x0B] = '0',  [0x0C] = '-',  [0x0D] = '=',
    [0x0E] = '\b', [0x0F] = '\t', [0x10] = 'q',
    [0x11] = 'w',  [0x12] = 'e',  [0x13] = 'r',
    [0x14] = 't',  [0x15] = 'y',  [0x16] = 'u',
    [0x17] = 'i',  [0x18] = 'o',  [0x19] = 'p',
    [0x1A] = '[',  [0x1B] = ']',  [0x1C] = '\n',
    [0x1E] = 'a',  [0x1F] = 's',  [0x20] = 'd',
    [0x21] = 'f',  [0x22] = 'g',  [0x23] = 'h',
    [0x24] = 'j',  [0x25] = 'k',  [0x26] = 'l',
    [0x27] = ';',  [0x28] = '\'', [0x29] = '`',
    [0x2B] = '\\', [0x2C] = 'z',  [0x2D] = 'x',
    [0x2E] = 'c',  [0x2F] = 'v',  [0x30] = 'b',
    [0x31] = 'n',  [0x32] = 'm',  [0x33] = ',',
    [0x34] = '.',  [0x35] = '/',  [0x39] = ' '
};

static char keymap1_to_shifted[MAP_SZ] = {
    [0x02] = '!',  [0x03] = '@',  [0x04] = '#',
    [0x05] = '$',  [0x06] = '%',  [0x07] = '^',
    [0x08] = '&',  [0x09] = '*',  [0x0A] = '(',
    [0x0B] = ')',  [0x0C] = '_',  [0x0D] = '+',
    [0x0E] = '\b', [0x0F] = '\t', [0x10] = 'Q',
    [0x11] = 'W',  [0x12] = 'E',  [0x13] = 'R',
    [0x14] = 'T',  [0x15] = 'Y',  [0x16] = 'U',
    [0x17] = 'I',  [0x18] = 'O',  [0x19] = 'P',
    [0x1A] = '{',  [0x1B] = '}',  [0x1C] = '\n',
    [0x1E] = 'A',  [0x1F] = 'S',  [0x20] = 'D',
    [0x21] = 'F',  [0x22] = 'G',  [0x23] = 'H',
    [0x24] = 'J',  [0x25] = 'K',  [0x26] = 'L',
    [0x27] = ':',  [0x28] = '"',  [0x29] = '~',
    [0x2B] = '|',  [0x2C] = 'Z',  [0x2D] = 'X',
    [0x2E] = 'C',  [0x2F] = 'V',  [0x30] = 'B',
    [0x31] = 'N',  [0x32] = 'M',  [0x33] = '<',
    [0x34] = '>',  [0x35] = '?',  [0x39] = ' '
};

bool decode1(u8 code, kbd_event_s* event) {
    static u8 state = IDLE;

    u8 nxt = table[state][code];
    state = nxt & 0x07;
    if (!(nxt & EMIT))
        return false;

    event->code     = code & (BRK - 1u);
    event->extended = (nxt & EXTENDED) >> 4;
    event->pressed  = code < BRK;
    return true;
}

char ascii1(u8 code, bool shifted) {
    return shifted ? keymap1_to_shifted[code] : keymap1_to_unshifted[code];
}