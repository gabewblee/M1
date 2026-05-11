NASM = nasm
I686_ELF_GCC = i686-elf-gcc
I686_ELF_LD = i686-elf-ld

CFLAGS = -ffreestanding -O0 -nostdlib -g
LDFLAGS = -T boot/linker.ld

.PHONY: all run clean

all: build/boot.o build/idt.o build/isr.o build/kernel.o build/panic.o build/pic.o build/pmm.o build/string.o build/vga.o
	$(I686_ELF_LD) $(LDFLAGS) build/boot.o build/idt.o build/isr.o build/kernel.o build/panic.o build/pic.o build/pmm.o build/string.o build/vga.o -o iso/boot/m1.bin

run:
	grub-mkrescue -o build/m1.iso iso
	qemu-system-i386 -cdrom build/m1.iso

build/boot.o: boot/boot.asm
	$(NASM) -f elf32 boot/boot.asm -o build/boot.o

build/idt.o: kernel/idt.c
	$(I686_ELF_GCC) $(CFLAGS) -c kernel/idt.c -o build/idt.o

build/isr.o: kernel/isr.asm
	$(NASM) -f elf32 kernel/isr.asm -o build/isr.o

build/kernel.o: kernel/kernel.c
	$(I686_ELF_GCC) $(CFLAGS) -c kernel/kernel.c -o build/kernel.o

build/panic.o: kernel/panic.c
	$(I686_ELF_GCC) $(CFLAGS) -c kernel/panic.c -o build/panic.o

build/pic.o: kernel/pic.c
	$(I686_ELF_GCC) $(CFLAGS) -c kernel/pic.c -o build/pic.o

build/pmm.o: mm/pmm.c
	$(I686_ELF_GCC) $(CFLAGS) -c mm/pmm.c -o build/pmm.o

build/string.o: lib/string.c
	$(I686_ELF_GCC) $(CFLAGS) -c lib/string.c -o build/string.o

build/vga.o: drivers/vga.c
	$(I686_ELF_GCC) $(CFLAGS) -c drivers/vga.c -o build/vga.o

clean:
	rm -rf build/* iso/boot/m1.bin