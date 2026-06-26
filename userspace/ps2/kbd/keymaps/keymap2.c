#include <userspace/ps2/kbd/keymaps/keymap2.h>

/* ASCII table size */
#define MAP_SZ                  128u

/* Decoder states */
#define IDLE                    0u
#define BRK                     1u
#define EXT                     2u
#define XBRK                    3u
#define P7                      4u
#define P6                      5u
#define P5                      6u
#define P4                      7u
#define P3                      8u
#define P2                      9u
#define P1                      10u

/* Transition flags */
#define EMIT                    0x10u
#define EXTENDED                0x20u
#define RELEASE                 0x40u

/* Transition values */
#define TRANSITION(state, flag) ((u8)(((state) & 0x0F) | (flag)))
#define NONE                    TRANSITION(IDLE, 0)
#define KEY                     TRANSITION(IDLE, EMIT)
#define REL                     TRANSITION(IDLE, EMIT | RELEASE)
#define XKEY                    TRANSITION(IDLE, EMIT | EXTENDED)
#define XREL                    TRANSITION(IDLE, EMIT | EXTENDED | RELEASE)

/* Transition table */
#define ROW(name, def, ...)  \
    static u8 name[256] = {  \
        [0 ... 255] = (def), \
        __VA_ARGS__          \
    }

#define IGN8                                                    \
    [0x00] = NONE, [0xAA] = NONE, [0xEE] = NONE, [0xFA] = NONE, \
    [0xFC] = NONE, [0xFD] = NONE, [0xFE] = NONE, [0xFF] = NONE

ROW(idle, KEY, IGN8, [0xF0] = TRANSITION(BRK, 0), [0xE0] = TRANSITION(EXT, 0), [0xE1] = TRANSITION(P7, 0));
ROW(brk, REL, IGN8);
ROW(ext, XKEY, IGN8, [0xF0] = TRANSITION(XBRK, 0));
ROW(xbrk, XREL, IGN8);
ROW(p7, TRANSITION(P6, 0));
ROW(p6, TRANSITION(P5, 0));
ROW(p5, TRANSITION(P4, 0));
ROW(p4, TRANSITION(P3, 0));
ROW(p3, TRANSITION(P2, 0));
ROW(p2, TRANSITION(P1, 0));
ROW(p1, NONE);

static u8* table[] = {
    [IDLE] = idle,
    [BRK]  = brk,
    [EXT]  = ext,
    [XBRK] = xbrk,
    [P7]   = p7,
    [P6]   = p6,
    [P5]   = p5,
    [P4]   = p4,
    [P3]   = p3,
    [P2]   = p2,
    [P1]   = p1
};

static char keymap2_to_unshifted[MAP_SZ] = {
    [0x0D] = '\t', [0x0E] = '`',  [0x15] = 'q',
    [0x16] = '1',  [0x1A] = 'z',  [0x1B] = 's',
    [0x1C] = 'a',  [0x1D] = 'w',  [0x1E] = '2',
    [0x21] = 'c',  [0x22] = 'x',  [0x23] = 'd',
    [0x24] = 'e',  [0x25] = '4',  [0x26] = '3',
    [0x29] = ' ',  [0x2A] = 'v',  [0x2B] = 'f',
    [0x2C] = 't',  [0x2D] = 'r',  [0x2E] = '5',
    [0x31] = 'n',  [0x32] = 'b',  [0x33] = 'h',
    [0x34] = 'g',  [0x35] = 'y',  [0x36] = '6',
    [0x3A] = 'm',  [0x3B] = 'j',  [0x3C] = 'u',
    [0x3D] = '7',  [0x3E] = '8',  [0x41] = ',',
    [0x42] = 'k',  [0x43] = 'i',  [0x44] = 'o',
    [0x45] = '0',  [0x46] = '9',  [0x49] = '.',
    [0x4A] = '/',  [0x4B] = 'l',  [0x4C] = ';',
    [0x4D] = 'p',  [0x4E] = '-',  [0x52] = '\'',
    [0x54] = '[',  [0x55] = '=',  [0x5A] = '\n',
    [0x5B] = ']',  [0x5D] = '\\', [0x66] = '\b'
};

static char keymap2_to_shifted[MAP_SZ] = {
    [0x0D] = '\t', [0x0E] = '~',  [0x15] = 'Q',
    [0x16] = '!',  [0x1A] = 'Z',  [0x1B] = 'S',
    [0x1C] = 'A',  [0x1D] = 'W',  [0x1E] = '@',
    [0x21] = 'C',  [0x22] = 'X',  [0x23] = 'D',
    [0x24] = 'E',  [0x25] = '$',  [0x26] = '#',
    [0x29] = ' ',  [0x2A] = 'V',  [0x2B] = 'F',
    [0x2C] = 'T',  [0x2D] = 'R',  [0x2E] = '%',
    [0x31] = 'N',  [0x32] = 'B',  [0x33] = 'H',
    [0x34] = 'G',  [0x35] = 'Y',  [0x36] = '^',
    [0x3A] = 'M',  [0x3B] = 'J',  [0x3C] = 'U',
    [0x3D] = '&',  [0x3E] = '*',  [0x41] = '<',
    [0x42] = 'K',  [0x43] = 'I',  [0x44] = 'O',
    [0x45] = ')',  [0x46] = '(',  [0x49] = '>',
    [0x4A] = '?',  [0x4B] = 'L',  [0x4C] = ':',
    [0x4D] = 'P',  [0x4E] = '_',  [0x52] = '"',
    [0x54] = '{',  [0x55] = '+',  [0x5A] = '\n',
    [0x5B] = '}',  [0x5D] = '|',  [0x66] = '\b'
};

bool decode2(u8 code, kbd_event_s* event) {
    static u8 state = IDLE;

    u8 nxt = table[state][code];
    state = nxt & 0x0F;
    if (!(nxt & EMIT))
        return false;

    event->code     = code;
    event->extended = (nxt & EXTENDED) >> 5;
    event->pressed  = !(nxt & RELEASE);
    return true;
}

char ascii2(u8 code, bool shifted) {
    return shifted ? keymap2_to_shifted[code] : keymap2_to_unshifted[code];
}