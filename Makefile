NASM 		   = nasm
I686_ELF_GCC   = i686-elf-gcc
I686_ELF_LD    = i686-elf-ld
CFLAGS 		   = -ffreestanding -O0 -nostdlib -g
KERNEL_CFLAGS  = $(CFLAGS) -I. -Iinclude -Iuapi
USER_CFLAGS    = $(CFLAGS) -I. -Iuserspace/libc -Iuserspace/server -Iuserspace/ata -Iuapi
KERNEL_LDFLAGS = -T boot/linker.ld --no-warn-rwx-segments
USER_LDFLAGS   = -T userspace/startup/linker.ld --no-warn-rwx-segments

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
                  kernel/irq.c     \
                  kernel/kernel.c  \
                  kernel/panic.c   \
                  kernel/sched.c   \
                  kernel/servers.c \
                  kernel/syscall.c \
                  kernel/task.c    \
                  kernel/thread.c  \
                  libk/string.c    \
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
KERNEL_TARGET   = build/boot/m1.elf

#----------------------------------------------------------------------------------------------------
# User files
#----------------------------------------------------------------------------------------------------

LIBC_SRCS = userspace/libc/stdio.c  \
            userspace/libc/string.c \
            userspace/libc/syscall.c
LIBC_OBJS = $(LIBC_SRCS:%.c=build/%.o)

SERVER_SRCS = userspace/server/server.c
SERVER_OBJS = $(SERVER_SRCS:%.c=build/%.o)

PS2_SRCS   = userspace/ps2/kbd/driver.c          \
             userspace/ps2/kbd/dispatch.c        \
             userspace/ps2/kbd/keymaps/keymap.c  \
             userspace/ps2/kbd/keymaps/keymap1.c \
             userspace/ps2/kbd/keymaps/keymap2.c \
             userspace/ps2/kbd/keymaps/keymap3.c \
             userspace/ps2/kbd/main.c            \
             userspace/ps2/ps2.c
PS2_C_OBJS = $(PS2_SRCS:%.c=build/%.o)
PS2_OBJS   = $(LIBC_OBJS) $(SERVER_OBJS) $(PS2_C_OBJS) $(STARTUP_ASMS_OBJS)
PS2_TARGET = build/userspace/ps2/kbd.elf

VGA_SRCS   = userspace/vga/driver.c   \
             userspace/vga/dispatch.c \
             userspace/vga/main.c
VGA_C_OBJS = $(VGA_SRCS:%.c=build/%.o)
VGA_OBJS   = $(LIBC_OBJS) $(SERVER_OBJS) $(VGA_C_OBJS) $(STARTUP_ASMS_OBJS)
VGA_TARGET = build/userspace/vga/vga.elf

STARTUP_ASMS = userspace/startup/crt0.asm
STARTUP_ASMS_OBJS = $(STARTUP_ASMS:%.asm=build/%.o)

ISO = iso/boot/m1.elf iso/boot/vga.elf iso/boot/keyboard.elf

#----------------------------------------------------------------------------------------------------
# Build targets
#----------------------------------------------------------------------------------------------------

all: $(KERNEL_TARGET) $(PS2_TARGET) $(VGA_TARGET)

run: iso/boot/grub/grub.cfg $(ISO)
	grub-mkrescue -o build/m1.iso iso
	qemu-system-i386 -cdrom build/m1.iso

$(KERNEL_TARGET): $(KERNEL_OBJS)
	@mkdir -p $(dir $@) iso/boot
	$(I686_ELF_LD) $(KERNEL_LDFLAGS) $(KERNEL_OBJS) -o $@
	cp $@ iso/boot/m1.elf

$(PS2_TARGET): $(PS2_OBJS)
	@mkdir -p $(dir $@) iso/boot
	$(I686_ELF_LD) $(USER_LDFLAGS) $(PS2_OBJS) -o $@
	cp $@ iso/boot/keyboard.elf

$(VGA_TARGET): $(VGA_OBJS)
	@mkdir -p $(dir $@) iso/boot
	$(I686_ELF_LD) $(USER_LDFLAGS) $(VGA_OBJS) -o $@
	cp $@ iso/boot/vga.elf

$(KERNEL_C_OBJS): build/%.o: %.c
	@mkdir -p $(dir $@)
	$(I686_ELF_GCC) $(KERNEL_CFLAGS) -c $< -o $@

$(LIBC_OBJS) $(SERVER_OBJS) $(VGA_C_OBJS) $(PS2_C_OBJS): build/%.o: %.c
	@mkdir -p $(dir $@)
	$(I686_ELF_GCC) $(USER_CFLAGS) -c $< -o $@

build/%.o: %.asm
	@mkdir -p $(dir $@)
	$(NASM) -f elf32 -I. -Iinclude $< -o $@

clean:
	rm -rf build/* iso/boot/m1.elf iso/boot/vga.elf iso/boot/keyboard.elf iso/boot/ata.elf