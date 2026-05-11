ENTRY(_start)

HIGHER_HALF_OFFSET = 0xC0000000;

SECTIONS
{
    . = 1M;

    .multiboot BLOCK(4K) :
	{
		*(.multiboot)
	}

	. += HIGHER_HALF_OFFSET;
	. = ALIGN(4K);
	_skernel = .;

	.init BLOCK(4K) : AT(ADDR(.init) - HIGHER_HALF_OFFSET)
	{
		_sinit = .;
		*(.init)
		_einit = .;
	}

	.text BLOCK(4K) : AT(ADDR(.text) - HIGHER_HALF_OFFSET)
	{
		_stext = .;
		*(.text)
		_etext = .;
	}

	.rodata BLOCK(4K) : AT(ADDR(.rodata) - HIGHER_HALF_OFFSET)
	{
		_srodata = .;
		*(.rodata)
		_erodata = .;
	}

	.data BLOCK(4K) : AT(ADDR(.data) - HIGHER_HALF_OFFSET)
	{
		_sdata = .;
		*(.data)
		_edata = .;
	}

	.bss BLOCK(4K) : AT(ADDR(.bss) - HIGHER_HALF_OFFSET)
	{
		_sbss = .;
		*(COMMON)
		*(.bss)
		_ebss = .;
	}

	. = ALIGN(4K);
	_ekernel = .;

	.kheap BLOCK(4K) (NOLOAD) :
	{
		_skheap = .;
		. += 16M;
		_ekheap = .;
	}
}