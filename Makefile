NASM = nasm
I686_ELF_GCC = i686-elf-gcc
I686_ELF_LD = i686-elf-ld

CFLAGS = -ffreestanding -O0 -nostdlib -g
LDFLAGS = -T boot/linker.ld --no-warn-rwx-segments

.PHONY: all run clean

all: build/boot.o build/gdt.o build/idt.o build/isr.o build/kernel.o build/kheap.o build/panic.o build/pic.o build/pmm.o build/setup.o build/string.o build/switch.o build/syscall.o build/task.o build/vga.o build/vmm.o
	$(I686_ELF_LD) $(LDFLAGS) build/boot.o build/gdt.o build/idt.o build/isr.o build/kernel.o build/kheap.o build/panic.o build/pic.o build/pmm.o build/setup.o build/string.o build/switch.o build/syscall.o build/task.o build/vga.o build/vmm.o -o iso/boot/m1.bin

run:
	grub-mkrescue -o build/m1.iso iso
	qemu-system-i386 -cdrom build/m1.iso

build/boot.o: boot/boot.asm
	$(NASM) -f elf32 boot/boot.asm -o build/boot.o

build/gdt.o: kernel/gdt.c
	$(I686_ELF_GCC) $(CFLAGS) -c kernel/gdt.c -o build/gdt.o

build/idt.o: kernel/idt.c
	$(I686_ELF_GCC) $(CFLAGS) -c kernel/idt.c -o build/idt.o

build/isr.o: kernel/isr.asm
	$(NASM) -f elf32 kernel/isr.asm -o build/isr.o

build/kernel.o: kernel/kernel.c
	$(I686_ELF_GCC) $(CFLAGS) -c kernel/kernel.c -o build/kernel.o

build/kheap.o: mm/kheap.c
	$(I686_ELF_GCC) $(CFLAGS) -c mm/kheap.c -o build/kheap.o

build/panic.o: kernel/panic.c
	$(I686_ELF_GCC) $(CFLAGS) -c kernel/panic.c -o build/panic.o

build/pic.o: kernel/pic.c
	$(I686_ELF_GCC) $(CFLAGS) -c kernel/pic.c -o build/pic.o

build/pmm.o: mm/pmm.c
	$(I686_ELF_GCC) $(CFLAGS) -c mm/pmm.c -o build/pmm.o

build/setup.o: boot/setup.c
	$(I686_ELF_GCC) $(CFLAGS) -c boot/setup.c -o build/setup.o

build/string.o: lib/string.c
	$(I686_ELF_GCC) $(CFLAGS) -c lib/string.c -o build/string.o

build/switch.o: kernel/switch.asm
	$(NASM) -f elf32 kernel/switch.asm -o build/switch.o

build/syscall.o: kernel/syscall.c
	$(I686_ELF_GCC) $(CFLAGS) -c kernel/syscall.c -o build/syscall.o

build/task.o: kernel/task.c
	$(I686_ELF_GCC) $(CFLAGS) -c kernel/task.c -o build/task.o

build/vga.o: drivers/vga.c
	$(I686_ELF_GCC) $(CFLAGS) -c drivers/vga.c -o build/vga.o

build/vmm.o: mm/vmm.c
	$(I686_ELF_GCC) $(CFLAGS) -c mm/vmm.c -o build/vmm.o

clean:
	rm -rf build/* iso/boot/m1.bin