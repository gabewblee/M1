#include <userspace/ps2/kbd/keymaps/keymap.h>
#include <userspace/ps2/kbd/keymaps/keymap1.h>
#include <userspace/ps2/kbd/keymaps/keymap2.h>
#include <userspace/ps2/kbd/keymaps/keymap3.h>

keymap_s keymap1 = {
    .decode = decode1,
    .ascii  = ascii1
};

keymap_s keymap2 = {
    .decode = decode2,
    .ascii  = ascii2
};

keymap_s keymap3 = {
    .decode = decode3,
    .ascii  = ascii3
};