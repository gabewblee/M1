#include "keymap.h"
#include "keymap1.h"
#include "keymap2.h"
#include "keymap3.h"

const keymap_s keymap1 = {
    .decode = decode1,
    .ascii  = ascii1
};

const keymap_s keymap2 = {
    .decode = decode2,
    .ascii  = ascii2
};

const keymap_s keymap3 = {
    .decode = decode3,
    .ascii  = ascii3
};