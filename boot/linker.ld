ENTRY(_start)

KERNEL_BASE = 0xC0000000;

SECTIONS
{
    . = 1M;

    .multiboot BLOCK(4K) :
	{
		*(.multiboot)
	}

	. += KERNEL_BASE;
	. = ALIGN(4K);
	skernel = .;

	.init BLOCK(4K) : AT(ADDR(.init) - KERNEL_BASE)
	{
		sinit = .;
		*(.init)
		einit = .;
	}

	.text BLOCK(4K) : AT(ADDR(.text) - KERNEL_BASE)
	{
		stext = .;
		*(.text)
		etext = .;
	}

	.rodata BLOCK(4K) : AT(ADDR(.rodata) - KERNEL_BASE)
	{
		srodata = .;
		*(.rodata)
		erodata = .;
	}

	.data BLOCK(4K) : AT(ADDR(.data) - KERNEL_BASE)
	{
		sdata = .;
		*(.data)
		edata = .;
	}

	.bss BLOCK(4K) : AT(ADDR(.bss) - KERNEL_BASE)
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