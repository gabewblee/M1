NASM 		   = nasm
I686_ELF_GCC   = i686-elf-gcc
I686_ELF_LD    = i686-elf-ld
CFLAGS 		   = -ffreestanding -O0 -nostdlib -g -Iinclude
KERNEL_LDFLAGS = -T boot/linker.ld --no-warn-rwx-segments
USER_LDFLAGS   = --no-warn-rwx-segments

.PHONY: all run clean

#----------------------------------------------------------------------------------------------------
# Kernel files
#----------------------------------------------------------------------------------------------------

KERNEL_SRCS   = arch/x86/idt.c      \
                arch/x86/pic.c    \
                boot/setup.c      \
                dev/console.c     \
                dev/evga.c        \
                dev/klog.c        \
                kernel/ipc.c      \
                kernel/kernel.c   \
                kernel/panic.c    \
                kernel/sched.c    \
                kernel/services.c \
                kernel/syscall.c  \
                kernel/task.c     \
                kernel/thread.c   \
                lib/string.c      \
                loader/loader.c   \
                mm/kheap.c        \
                mm/pmm.c          \
                mm/vmm.c
KERNEL_ASMS   = arch/x86/isr.asm  \
	   		    boot/boot.asm     \
	   		    kernel/switch.asm
KERNEL_OBJS   = $(KERNEL_SRCS:%.c=build/%.o) $(KERNEL_ASMS:%.asm=build/%.o)
KERNEL_TARGET = build/boot/m1.bin

#----------------------------------------------------------------------------------------------------
# User files
#----------------------------------------------------------------------------------------------------

VGA_SRCS   = services/lib/syscall.c \
	         services/vga/main.c
VGA_ASMS   = services/vga/crt0.asm
VGA_OBJS   = $(VGA_SRCS:%.c=build/%.o) $(VGA_ASMS:%.asm=build/%.o)
VGA_TARGET = build/services/vga.bin.o

all: $(VGA_TARGET) $(KERNEL_TARGET)

$(KERNEL_TARGET): $(KERNEL_OBJS) $(VGA_TARGET)
	$(I686_ELF_LD) $(KERNEL_LDFLAGS) $(KERNEL_OBJS) $(VGA_TARGET) -o $@
	@mkdir -p iso/boot
	cp $@ iso/boot/m1.bin

$(VGA_TARGET): $(VGA_OBJS)
	@mkdir -p $(dir $@)
	$(I686_ELF_LD) $(USER_LDFLAGS) -T services/vga/linker.ld $(VGA_OBJS) -o build/services/vga.bin
	$(I686_ELF_LD) -r -b binary build/services/vga.bin -o $@

run: iso/boot/m1.bin
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
