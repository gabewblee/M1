ENTRY(_start)

PHDRS
{
    text   PT_LOAD FLAGS(5);
    rodata PT_LOAD FLAGS(4);
    data   PT_LOAD FLAGS(6);
}

SECTIONS
{
    . = 0x00400000;

    .text BLOCK(4K) :
    {
        *(.text)
    } :text

    .rodata BLOCK(4K) :
    {
        *(.rodata)
    } :rodata

    .data BLOCK(4K) :
    {
        *(.data)
    } :data

    .bss BLOCK(4K) :
    {
        *(COMMON)
        *(.bss)
    } :data
}
