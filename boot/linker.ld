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
	skernel = .;

	.init BLOCK(4K) : AT(ADDR(.init) - HIGHER_HALF_OFFSET)
	{
		sinit = .;
		*(.init)
		. = ALIGN(4);
		sinitcall = .;
		KEEP(*(.initcall0.init))
		KEEP(*(.initcall1.init))
		KEEP(*(.initcall2.init))
		KEEP(*(.initcall3.init))
		einitcall = .;
		einit = .;
	}

	.text BLOCK(4K) : AT(ADDR(.text) - HIGHER_HALF_OFFSET)
	{
		stext = .;
		*(.text)
		etext = .;
	}

	.rodata BLOCK(4K) : AT(ADDR(.rodata) - HIGHER_HALF_OFFSET)
	{
		srodata = .;
		*(.rodata)
		erodata = .;
	}

	.data BLOCK(4K) : AT(ADDR(.data) - HIGHER_HALF_OFFSET)
	{
		sdata = .;
		*(.data)
		edata = .;
	}

	.bss BLOCK(4K) : AT(ADDR(.bss) - HIGHER_HALF_OFFSET)
	{
		sbss = .;
		*(COMMON)
		*(.bss)
		ebss = .;
	}

	. = ALIGN(4K);
	ekernel = .;

	.kheap BLOCK(4K) (NOLOAD) :
	{
		skheap = .;
		. += 16M;
		ekheap = .;
	}
}