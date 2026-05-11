NASM = nasm
I686_ELF_GCC = i686-elf-gcc
I686_ELF_LD = i686-elf-ld

CFLAGS = -ffreestanding -O0 -nostdlib -g
LDFLAGS = -T boot/linker.ld

.PHONY: all run clean

all: build/boot.o build/kernel.o build/vga.o
	$(I686_ELF_LD) $(LDFLAGS) build/boot.o build/kernel.o build/vga.o -o iso/boot/m1.bin

run:
	grub-mkrescue -o build/m1.iso iso
	qemu-system-i386 -cdrom build/m1.iso

build/boot.o: boot/boot.asm
	$(NASM) -f elf32 boot/boot.asm -o build/boot.o

build/kernel.o: kernel/kernel.c
	$(I686_ELF_GCC) $(CFLAGS) -c kernel/kernel.c -o build/kernel.o

build/vga.o: drivers/vga.c
	$(I686_ELF_GCC) $(CFLAGS) -c drivers/vga.c -o build/vga.o

clean:
	rm -rf build/* iso/boot/m1.bin