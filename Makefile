NASM 	       = nasm
I686_ELF_GCC   = i686-elf-gcc
I686_ELF_LD    = i686-elf-ld
HOST_CC        = cc
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
                  kernel/capability/prep.c       \
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
            userspace/libc/exec.c       \
            userspace/libc/heap.c       \
            userspace/libc/minmax.c     \
            userspace/libc/start.c      \
            userspace/libc/stdio.c      \
            userspace/libc/string.c     \
            userspace/libc/syscall.c    \
            userspace/libc/vfs.c
LIBC_OBJS = $(LIBC_SRCS:%.c=build/%.o)

SERVER_SRCS = userspace/server/server.c
SERVER_OBJS = $(SERVER_SRCS:%.c=build/%.o)

FSLIB_SRCS = userspace/fs/lib/dispatch.c \
             userspace/fs/lib/icache.c   \
             userspace/fs/lib/radix.c    \
             userspace/fs/lib/super.c
FSLIB_OBJS = $(FSLIB_SRCS:%.c=build/%.o)

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

VFS_SRCS   = userspace/vfs/dcache.c   \
             userspace/vfs/dispatch.c \
             userspace/vfs/file.c     \
             userspace/vfs/fsclient.c \
             userspace/vfs/main.c     \
             userspace/vfs/mount.c    \
             userspace/vfs/namei.c    \
             userspace/vfs/rnode.c    \
             userspace/vfs/selftest.c \
             userspace/vfs/super.c
VFS_C_OBJS = $(VFS_SRCS:%.c=build/%.o)
VFS_OBJS   = $(LIBC_OBJS) $(SERVER_OBJS) $(VFS_C_OBJS) $(STARTUP_ASMS_OBJS)
VFS_TARGET = build/userspace/vfs/vfs.elf

TMPFS_SRCS   = userspace/fs/tmpfs/main.c \
               userspace/fs/tmpfs/tmpfs.c
TMPFS_C_OBJS = $(TMPFS_SRCS:%.c=build/%.o)
TMPFS_OBJS   = $(LIBC_OBJS) $(SERVER_OBJS) $(FSLIB_OBJS) $(TMPFS_C_OBJS) $(STARTUP_ASMS_OBJS)
TMPFS_TARGET = build/userspace/fs/tmpfs/tmpfs.elf

EXT2_SRCS   = userspace/fs/ext2/bcache.c \
              userspace/fs/ext2/bitmap.c \
              userspace/fs/ext2/blk.c    \
              userspace/fs/ext2/dir.c    \
              userspace/fs/ext2/ext2.c   \
              userspace/fs/ext2/inode.c  \
              userspace/fs/ext2/main.c
EXT2_C_OBJS = $(EXT2_SRCS:%.c=build/%.o)
EXT2_OBJS   = $(LIBC_OBJS) $(SERVER_OBJS) $(FSLIB_OBJS) $(EXT2_C_OBJS) $(STARTUP_ASMS_OBJS)
EXT2_TARGET = build/userspace/fs/ext2/ext2.elf

SHELL_SRCS   = userspace/shell/builtins.c \
               userspace/shell/line.c     \
               userspace/shell/main.c     \
               userspace/shell/path.c
SHELL_C_OBJS = $(SHELL_SRCS:%.c=build/%.o)
SHELL_OBJS   = $(LIBC_OBJS) $(SERVER_OBJS) $(SHELL_C_OBJS)          \
               build/userspace/ps2/kbd/keymaps/keymap1.o            \
               $(STARTUP_ASMS_OBJS)
SHELL_TARGET = build/userspace/shell/shell.elf

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

HELLO_SRCS   = userspace/hello/main.c
HELLO_C_OBJS = $(HELLO_SRCS:%.c=build/%.o)
HELLO_OBJS   = $(LIBC_OBJS) $(HELLO_C_OBJS) $(STARTUP_ASMS_OBJS)
HELLO_TARGET = build/userspace/hello/hello.elf

MKFS_SRCS   = tools/mkfs_ext2.c
MKFS_TARGET = build/tools/mkfs_ext2

ISO_ELFS = m1.elf root.elf vga.elf keyboard.elf ata.elf vfs.elf tmpfs.elf ext2.elf shell.elf
ISO      = $(ISO_ELFS:%=iso/boot/%)
TARGETS  = $(KERNEL_TARGET) $(ROOT_TARGET) $(VGA_TARGET) $(PS2_TARGET) $(ATA_TARGET) \
           $(VFS_TARGET) $(TMPFS_TARGET) $(EXT2_TARGET) $(SHELL_TARGET) $(HELLO_TARGET)

#----------------------------------------------------------------------------------------------------
# Build targets
#----------------------------------------------------------------------------------------------------

all: $(TARGETS) $(MKFS_TARGET)

clean:
	rm -rf build/* $(ISO)

run: iso/boot/grub/grub.cfg $(TARGETS) $(ATA_IMG)
	grub-mkrescue -o build/m1.iso iso
	qemu-system-i386 -cdrom build/m1.iso -drive file=$(ATA_IMG),format=raw,if=ide,index=0,media=disk

test: iso/boot/grub/grub.cfg $(TARGETS) $(ATA_IMG)
	@grub-mkrescue -o build/m1.iso iso >/dev/null 2>&1
	@rm -f build/serial.log
	-@timeout 15 qemu-system-i386 -cdrom build/m1.iso -drive file=$(ATA_IMG),format=raw,if=ide,index=0,media=disk \
        -serial file:build/serial.log -display none -monitor none -no-reboot </dev/null >/dev/null 2>&1
	@cat build/serial.log

$(ATA_IMG): $(MKFS_TARGET) $(HELLO_TARGET)
	@mkdir -p $(dir $@)
	$(MKFS_TARGET) $@ $(ATA_IMG_SZ) $(HELLO_TARGET)

$(MKFS_TARGET): $(MKFS_SRCS)
	@mkdir -p $(dir $@)
	$(HOST_CC) -O2 -Wall $< -o $@

$(KERNEL_TARGET): $(KERNEL_OBJS)
	@mkdir -p $(dir $@) iso/boot
	$(I686_ELF_LD) $(KERNEL_LDFLAGS) $(KERNEL_OBJS) -o $@
	cp $@ iso/boot/m1.elf

$(ROOT_TARGET): $(ROOT_OBJS)
	@mkdir -p $(dir $@) iso/boot
	$(I686_ELF_LD) $(USER_LDFLAGS) $(ROOT_OBJS) -o $@
	cp $@ iso/boot/root.elf

$(VGA_TARGET): $(VGA_OBJS)
	@mkdir -p $(dir $@) iso/boot
	$(I686_ELF_LD) $(USER_LDFLAGS) $(VGA_OBJS) -o $@
	cp $@ iso/boot/vga.elf

$(PS2_TARGET): $(PS2_OBJS)
	@mkdir -p $(dir $@) iso/boot
	$(I686_ELF_LD) $(USER_LDFLAGS) $(PS2_OBJS) -o $@
	cp $@ iso/boot/keyboard.elf

$(ATA_TARGET): $(ATA_OBJS)
	@mkdir -p $(dir $@) iso/boot
	$(I686_ELF_LD) $(USER_LDFLAGS) $(ATA_OBJS) -o $@
	cp $@ iso/boot/ata.elf

$(VFS_TARGET): $(VFS_OBJS)
	@mkdir -p $(dir $@) iso/boot
	$(I686_ELF_LD) $(USER_LDFLAGS) $(VFS_OBJS) -o $@
	cp $@ iso/boot/vfs.elf

$(TMPFS_TARGET): $(TMPFS_OBJS)
	@mkdir -p $(dir $@) iso/boot
	$(I686_ELF_LD) $(USER_LDFLAGS) $(TMPFS_OBJS) -o $@
	cp $@ iso/boot/tmpfs.elf

$(EXT2_TARGET): $(EXT2_OBJS)
	@mkdir -p $(dir $@) iso/boot
	$(I686_ELF_LD) $(USER_LDFLAGS) $(EXT2_OBJS) -o $@
	cp $@ iso/boot/ext2.elf

$(SHELL_TARGET): $(SHELL_OBJS)
	@mkdir -p $(dir $@) iso/boot
	$(I686_ELF_LD) $(USER_LDFLAGS) $(SHELL_OBJS) -o $@
	cp $@ iso/boot/shell.elf

$(HELLO_TARGET): $(HELLO_OBJS)
	@mkdir -p $(dir $@)
	$(I686_ELF_LD) $(USER_LDFLAGS) $(HELLO_OBJS) -o $@

USER_C_OBJS = $(LIBC_OBJS) $(SERVER_OBJS) $(FSLIB_OBJS) $(PS2_C_OBJS) $(VGA_C_OBJS)  \
              $(VFS_C_OBJS) $(TMPFS_C_OBJS) $(EXT2_C_OBJS) $(SHELL_C_OBJS)           \
              $(ROOT_C_OBJS) $(ATA_C_OBJS) $(HELLO_C_OBJS)

$(USER_C_OBJS): build/%.o: %.c
	@mkdir -p $(dir $@)
	$(I686_ELF_GCC) $(USER_CFLAGS) -c $< -o $@

$(KERNEL_C_OBJS): build/%.o: %.c
	@mkdir -p $(dir $@)
	$(I686_ELF_GCC) $(KERNEL_CFLAGS) -c $< -o $@

build/%.o: %.asm
	@mkdir -p $(dir $@)
	$(NASM) -f elf32 -I. -Iinclude $< -o $@

DEPS = $(KERNEL_C_OBJS:.o=.d) $(USER_C_OBJS:.o=.d)
-include $(DEPS)
