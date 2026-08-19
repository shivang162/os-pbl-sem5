PROJECT := StudyOS
ARCH := i386
CROSS_PREFIX ?= i686-elf-
CC := $(CROSS_PREFIX)gcc
LD := $(CROSS_PREFIX)ld
ASFLAGS := -c
CFLAGS := -std=gnu99 -ffreestanding -O2 -Wall -Wextra
LDFLAGS := -T linker.ld -nostdlib
QEMU := qemu-system-$(ARCH)

BUILD_DIR := build
ISO_DIR := $(BUILD_DIR)/iso
GRUB_DIR := $(ISO_DIR)/boot/grub
KERNEL := $(BUILD_DIR)/kernel.elf
ISO := $(BUILD_DIR)/$(PROJECT).iso

C_SOURCES := \
	kernel/kernel.c \
	kernel/memory.c \
	kernel/terminal.c \
	drivers/keyboard.c \
	drivers/screen.c \
	filesystem/filesystem.c \
	security/login.c \
	security/permissions.c \
	security/security.c \
	shell/commands.c \
	shell/shell.c

OBJ := $(BUILD_DIR)/boot.o $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SOURCES))

.PHONY: all help iso run clean

all: $(KERNEL)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/boot.o: boot/boot.s | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL): $(OBJ) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJ)

$(GRUB_DIR):
	mkdir -p $(GRUB_DIR)

$(ISO): $(KERNEL) boot/grub/grub.cfg | $(GRUB_DIR)
	cp $(KERNEL) $(ISO_DIR)/boot/kernel.elf
	cp boot/grub/grub.cfg $(GRUB_DIR)/grub.cfg
	grub-mkrescue -o $@ $(ISO_DIR)

iso: $(ISO)

run: $(ISO)
	$(QEMU) -cdrom $(ISO)

help:
	@echo "Available targets:"
	@echo "  make all   - build kernel ELF"
	@echo "  make iso   - build bootable GRUB ISO"
	@echo "  make run   - run ISO in QEMU"
	@echo "  make clean - remove build artifacts"

clean:
	rm -rf $(BUILD_DIR)
