NASM = nasm
I686_ELF_GCC = i686-elf-gcc
I686_ELF_LD = i686-elf-ld

CFLAGS = -ffreestanding -O0 -nostdlib -g -Iinclude
LDFLAGS = -T boot/linker.ld --no-warn-rwx-segments

.PHONY: all run clean

SRCS = arch/x86/idt.c   \
	   arch/x86/pic.c   \
	   boot/setup.c     \
	   dev/vga.c        \
	   kernel/ipc.c     \
	   kernel/kernel.c  \
	   kernel/panic.c   \
	   kernel/sched.c   \
	   kernel/syscall.c \
	   kernel/task.c    \
	   kernel/thread.c  \
	   lib/string.c     \
	   mm/kheap.c       \
	   mm/pmm.c         \
	   mm/vmm.c

ASMS = arch/x86/isr.asm  \
	   boot/boot.asm     \
	   kernel/switch.asm

OBJS = $(SRCS:%.c=build/%.o) $(ASMS:%.asm=build/%.o)

all: iso/boot/m1.bin

iso/boot/m1.bin: $(OBJS)
	$(I686_ELF_LD) $(LDFLAGS) $(OBJS) -o $@

run:
	grub-mkrescue -o build/m1.iso iso
	qemu-system-i386 -cdrom build/m1.iso

build/%.o: %.c
	@mkdir -p $(dir $@)
	$(I686_ELF_GCC) $(CFLAGS) -c $< -o $@

build/%.o: %.asm
	@mkdir -p $(dir $@)
	$(NASM) -f elf32 $< -o $@

clean:
	rm -rf build/* iso/boot/m1.bin
