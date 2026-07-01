NASM 	       = nasm
I686_ELF_GCC   = i686-elf-gcc
I686_ELF_LD    = i686-elf-ld
CFLAGS 	       = -ffreestanding -O0 -nostdlib -g -MMD -MP
KERNEL_CFLAGS  = $(CFLAGS) -I. -Iinclude -Iuapi
USER_CFLAGS    = $(CFLAGS) -I. -Iuserspace/libc -Iuserspace/server -Iuserspace/ata -Iuapi
KERNEL_LDFLAGS = -T boot/linker.ld --no-warn-rwx-segments
USER_LDFLAGS   = -T userspace/startup/linker.ld --no-warn-rwx-segments
ATA_IMG        = build/ata.img
ATA_IMG_SZ     = 64

.PHONY: all clean run test

#----------------------------------------------------------------------------------------------------
# Kernel files
#----------------------------------------------------------------------------------------------------

KERNEL_SRCS     = arch/x86/idt.c                 \
                  arch/x86/pic.c                 \
                  boot/setup.c                   \
                  dev/console.c                  \
                  dev/evga.c                     \
                  dev/klog.c                     \
                  dev/serial.c                   \
                  kernel/core/panic.c            \
                  kernel/core/sched.c            \
                  kernel/core/task.c             \
                  kernel/core/thread.c           \
                  kernel/capability/capability.c \
                  kernel/capability/endpoint.c   \
                  kernel/capability/invoke.c     \
                  kernel/capability/bootstrap.c  \
                  kernel/irq/irq.c               \
                  kernel/kernel.c                \
                  kernel/syscall.c               \
                  libk/string.c                  \
                  loader/loader.c                \
                  mm/pmm.c                       \
                  mm/vmm.c                       \
                  mm/kheap.c
KERNEL_ASMS     = arch/x86/isr.asm \
	   	          boot/boot.asm    \
       		      kernel/core/switch.asm
KERNEL_C_OBJS   = $(KERNEL_SRCS:%.c=build/%.o)
KERNEL_ASM_OBJS = $(KERNEL_ASMS:%.asm=build/%.o)
KERNEL_OBJS     = $(KERNEL_C_OBJS) $(KERNEL_ASM_OBJS)
KERNEL_TARGET   = build/boot/m1.elf

#----------------------------------------------------------------------------------------------------
# User files
#----------------------------------------------------------------------------------------------------

LIBC_SRCS = userspace/libc/capability.c \
            userspace/libc/stdio.c      \
            userspace/libc/string.c     \
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

STARTUP_ASMS      = userspace/startup/crt0.asm
STARTUP_ASMS_OBJS = $(STARTUP_ASMS:%.asm=build/%.o)

ROOT_SRCS   = userspace/root/main.c
ROOT_C_OBJS = $(ROOT_SRCS:%.c=build/%.o)
ROOT_OBJS   = $(LIBC_OBJS) $(ROOT_C_OBJS) $(STARTUP_ASMS_OBJS)
ROOT_TARGET = build/userspace/root/root.elf

ATA_SRCS   = userspace/ata/driver.c   \
             userspace/ata/dispatch.c \
             userspace/ata/main.c     \
             userspace/ata/pio/pio.c
ATA_C_OBJS = $(ATA_SRCS:%.c=build/%.o)
ATA_OBJS   = $(LIBC_OBJS) $(SERVER_OBJS) $(ATA_C_OBJS) $(STARTUP_ASMS_OBJS)
ATA_TARGET = build/userspace/ata/ata.elf

ISO = iso/boot/m1.elf iso/boot/vga.elf iso/boot/keyboard.elf iso/boot/ata.elf iso/boot/root.elf

#----------------------------------------------------------------------------------------------------
# Build targets
#----------------------------------------------------------------------------------------------------

all: $(ATA_TARGET) $(KERNEL_TARGET) $(PS2_TARGET) $(VGA_TARGET) $(ROOT_TARGET)

clean:
	rm -rf build/* iso/boot/m1.elf iso/boot/vga.elf iso/boot/keyboard.elf iso/boot/ata.elf iso/boot/root.elf

run: iso/boot/grub/grub.cfg $(ISO)
	@mkdir -p build
	@if [ ! -f $(ATA_IMG) ]; then                                                \
        dd if=/dev/zero of=$(ATA_IMG) bs=1M count=$(ATA_IMG_SZ) >/dev/null 2>&1; \
	fi
	grub-mkrescue -o build/m1.iso iso
	qemu-system-i386 -cdrom build/m1.iso -drive file=$(ATA_IMG),format=raw,if=ide,index=0,media=disk

test: iso/boot/grub/grub.cfg $(ISO)
	@mkdir -p build
	@if [ ! -f $(ATA_IMG) ]; then                                                \
        dd if=/dev/zero of=$(ATA_IMG) bs=1M count=$(ATA_IMG_SZ) >/dev/null 2>&1; \
	fi
	@grub-mkrescue -o build/m1.iso iso >/dev/null 2>&1
	@rm -f build/serial.log
	-@timeout 8 qemu-system-i386 -cdrom build/m1.iso -drive file=$(ATA_IMG),format=raw,if=ide,index=0,media=disk \
        -serial file:build/serial.log -display none -monitor none -no-reboot </dev/null >/dev/null 2>&1
	@cat build/serial.log

$(ATA_TARGET): $(ATA_OBJS)
	@mkdir -p $(dir $@) iso/boot
	$(I686_ELF_LD) $(USER_LDFLAGS) $(ATA_OBJS) -o $@
	cp $@ iso/boot/ata.elf

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

$(ROOT_TARGET): $(ROOT_OBJS)
	@mkdir -p $(dir $@) iso/boot
	$(I686_ELF_LD) $(USER_LDFLAGS) $(ROOT_OBJS) -o $@
	cp $@ iso/boot/root.elf

$(ATA_C_OBJS) $(LIBC_OBJS) $(PS2_C_OBJS) $(SERVER_OBJS) $(VGA_C_OBJS) $(ROOT_C_OBJS): build/%.o: %.c
	@mkdir -p $(dir $@)
	$(I686_ELF_GCC) $(USER_CFLAGS) -c $< -o $@

$(KERNEL_C_OBJS): build/%.o: %.c
	@mkdir -p $(dir $@)
	$(I686_ELF_GCC) $(KERNEL_CFLAGS) -c $< -o $@

build/%.o: %.asm
	@mkdir -p $(dir $@)
	$(NASM) -f elf32 -I. -Iinclude $< -o $@

DEPS = $(KERNEL_C_OBJS:.o=.d) $(LIBC_OBJS:.o=.d) $(SERVER_OBJS:.o=.d) \
       $(PS2_C_OBJS:.o=.d) $(VGA_C_OBJS:.o=.d) $(ATA_C_OBJS:.o=.d) $(ROOT_C_OBJS:.o=.d)
-include $(DEPS)