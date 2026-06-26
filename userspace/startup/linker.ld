ENTRY(_start)

PHDRS
{
    text   PT_LOAD FLAGS(5);
    rodata PT_LOAD FLAGS(4);
    note   PT_NOTE FLAGS(4);
    data   PT_LOAD FLAGS(6);
    bss    PT_LOAD FLAGS(6);
}

SECTIONS
{
    . = 0x00100000;

    .text BLOCK(4K) :
    {
        *(.text)
    } :text

    .rodata BLOCK(4K) :
    {
        *(.rodata)
    } :rodata

    .note BLOCK(4K) :
    {
        KEEP(*(.note))
    } :note

    . = ALIGN(4K);

    .data BLOCK(4K) :
    {
        *(.data)
    } :data

    .bss BLOCK(4K) :
    {
        *(.bss)
        *(COMMON)
    } :bss
}