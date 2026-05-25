NASM 		   = nasm
I686_ELF_GCC   = i686-elf-gcc
I686_ELF_LD    = i686-elf-ld
CFLAGS 		   = -ffreestanding -O0 -nostdlib -g -Iinclude
KERNEL_CFLAGS  = $(CFLAGS) -DM1
USER_CFLAGS    = $(CFLAGS) -Iservers/libc
KERNEL_LDFLAGS = -T boot/linker.ld --no-warn-rwx-segments
USER_LDFLAGS   = --no-warn-rwx-segments

.PHONY: all run clean

#----------------------------------------------------------------------------------------------------
# Kernel files
#----------------------------------------------------------------------------------------------------

KERNEL_SRCS     = arch/x86/idt.c   \
                  arch/x86/pic.c   \
                  boot/setup.c     \
                  dev/console.c    \
                  dev/evga.c       \
                  dev/klog.c       \
                  kernel/ipc.c     \
                  kernel/kernel.c  \
                  kernel/panic.c   \
                  kernel/sched.c   \
                  kernel/servers.c \
                  kernel/syscall.c \
                  kernel/task.c    \
                  kernel/thread.c  \
                  lib/string.c     \
                  loader/loader.c  \
                  mm/kheap.c       \
                  mm/pmm.c         \
                  mm/vmm.c
KERNEL_ASMS     = arch/x86/isr.asm \
	   		      boot/boot.asm    \
	   		      kernel/switch.asm
KERNEL_C_OBJS   = $(KERNEL_SRCS:%.c=build/%.o)
KERNEL_ASM_OBJS = $(KERNEL_ASMS:%.asm=build/%.o)
KERNEL_OBJS     = $(KERNEL_C_OBJS) $(KERNEL_ASM_OBJS)
KERNEL_TARGET   = build/boot/m1.bin

#----------------------------------------------------------------------------------------------------
# User files
#----------------------------------------------------------------------------------------------------

LIBC_SRCS = servers/libc/server.c  \
            servers/libc/stdio.c   \
            servers/libc/string.c  \
            servers/libc/syscall.c
LIBC_OBJS = $(LIBC_SRCS:%.c=build/%.o)

PS2_KEYBOARD_SRCS     = servers/ps2/keyboard/driver.c   \
                        servers/ps2/keyboard/dispatch.c \
                        servers/ps2/keyboard/main.c
PS2_KEYBOARD_ASMS     = servers/ps2/keyboard/crt0.asm
PS2_KEYBOARD_C_OBJS   = $(PS2_KEYBOARD_SRCS:%.c=build/%.o)
PS2_KEYBOARD_ASM_OBJS = $(PS2_KEYBOARD_ASMS:%.asm=build/%.o)
PS2_KEYBOARD_OBJS     = $(LIBC_OBJS) $(PS2_KEYBOARD_C_OBJS) $(PS2_KEYBOARD_ASM_OBJS)
PS2_KEYBOARD_TARGET   = build/servers/ps2/keyboard.bin.o

VGA_SRCS     = servers/vga/driver.c   \
               servers/vga/dispatch.c \
               servers/vga/main.c
VGA_ASMS     = servers/vga/crt0.asm
VGA_C_OBJS   = $(VGA_SRCS:%.c=build/%.o)
VGA_ASM_OBJS = $(VGA_ASMS:%.asm=build/%.o)
VGA_OBJS     = $(LIBC_OBJS) $(VGA_C_OBJS) $(VGA_ASM_OBJS)
VGA_TARGET   = build/servers/vga.bin.o

#----------------------------------------------------------------------------------------------------
# Build targets
#----------------------------------------------------------------------------------------------------

all: $(KERNEL_TARGET) $(VGA_TARGET)

run: iso/boot/m1.bin
	grub-mkrescue -o build/m1.iso iso
	qemu-system-i386 -cdrom build/m1.iso

$(KERNEL_TARGET): $(KERNEL_OBJS) $(PS2_KEYBOARD_TARGET) $(VGA_TARGET)
	$(I686_ELF_LD) $(KERNEL_LDFLAGS) $(KERNEL_OBJS) $(PS2_KEYBOARD_TARGET) $(VGA_TARGET) -o $@
	@mkdir -p iso/boot
	cp $@ iso/boot/m1.bin

$(PS2_KEYBOARD_TARGET): $(PS2_KEYBOARD_OBJS) servers/ps2/keyboard/linker.ld
	@mkdir -p $(dir $@)
	$(I686_ELF_LD) $(USER_LDFLAGS) -T servers/ps2/keyboard/linker.ld $(PS2_KEYBOARD_OBJS) -o build/servers/ps2/keyboard.bin
	$(I686_ELF_LD) -r -b binary build/servers/ps2/keyboard.bin -o $@

$(VGA_TARGET): $(VGA_OBJS) servers/vga/linker.ld
	@mkdir -p $(dir $@)
	$(I686_ELF_LD) $(USER_LDFLAGS) -T servers/vga/linker.ld $(VGA_OBJS) -o build/servers/vga.bin
	$(I686_ELF_LD) -r -b binary build/servers/vga.bin -o $@

$(KERNEL_C_OBJS): build/%.o: %.c
	@mkdir -p $(dir $@)
	$(I686_ELF_GCC) $(KERNEL_CFLAGS) -c $< -o $@

$(LIBC_OBJS) $(VGA_C_OBJS) $(PS2_KEYBOARD_C_OBJS): build/%.o: %.c
	@mkdir -p $(dir $@)
	$(I686_ELF_GCC) $(USER_CFLAGS) -c $< -o $@

build/%.o: %.asm
	@mkdir -p $(dir $@)
	$(NASM) -f elf32 -Iinclude $< -o $@

clean:
	rm -rf build/* iso/boot/m1.bin