#include <userspace/libc/syscall.h>
#include <userspace/ps2/kbd/driver.h>
#include <userspace/ps2/kbd/keymaps/keymap.h>
#include <userspace/ps2/ps2.h>

#define PS2_KBD_BUF_CNT  32u
#define PS2_KBD_BUF_MASK (PS2_KBD_BUF_CNT - 1u)
#define PS2_KBD_IRQ      1u

static keymap_s*   keymap = &keymap1;
static kbd_event_s events[PS2_KBD_BUF_CNT];
static u32         head;
static u32         tail;
static u32         cnt;

static void process(void) {
    while (ps2_data_ready()) {
        kbd_event_s event;
        if (!keymap->decode(ps2_read_data(), &event))
            continue;

        if (cnt == PS2_KBD_BUF_CNT)
            continue;

        events[head] = event;
        head = (head + 1) & PS2_KBD_BUF_MASK;
        cnt++;
    }
}

void ps2_kbd_read(kbd_event_s* event) {
    for (;;) {
        process();
        if (cnt != 0)
            break;

        sys_irq_wait_for(PS2_KBD_IRQ);
    }

    *event = events[tail];
    tail = (tail + 1) & PS2_KBD_BUF_MASK;
    cnt--;
}

void ps2_kbd_init(void) {
    sys_irq_register_handler(PS2_KBD_IRQ);
}