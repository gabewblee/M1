ENTRY(_start)

PHDRS
{
    text PT_LOAD FLAGS(5);
    data PT_LOAD FLAGS(6);
}

SECTIONS
{
    . = 0x00400000;

    .text BLOCK(4K) :
    {
        *(.text)
        *(.rodata)
    } :text

    . = ALIGN(4K);

    .data BLOCK(4K) :
    {
        *(.data)
        *(.bss)
        *(COMMON)
    } :data
}
