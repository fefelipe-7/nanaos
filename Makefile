# Build system for nanaos nanacore phase 2

OSNAME = nanaos
KERNEL_DIR = kernel
BUILD_DIR = build
ISO_DIR = $(BUILD_DIR)/isodir
KERNEL_ELF = $(BUILD_DIR)/kernel.elf
ISO_IMAGE = $(BUILD_DIR)/$(OSNAME).iso
.DEFAULT_GOAL := all

CC = gcc
LD = ld
CFLAGS = -std=gnu17 -O2 -ffreestanding -mno-sse -mno-mmx -fno-builtin -fno-stack-protector -Wall -Wextra -Werror -g -I$(KERNEL_DIR)
LDFLAGS = -T $(KERNEL_DIR)/linker.ld -nostdlib -static -z max-page-size=0x1000

USERS_DIR = userspace
RUST_DIR = rust/nanarust
RUST_LIB = $(BUILD_DIR)/libnanarust.a
DISK_IMAGE = $(BUILD_DIR)/disk.img

RUST_REQUIRED ?= 0

KERNEL_SOURCES := $(shell find $(KERNEL_DIR) -name '*.c')
KERNEL_ASM_SOURCES := $(shell find $(KERNEL_DIR) -name '*.S')
KERNEL_OBJECTS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(KERNEL_SOURCES)) $(patsubst %.S,$(BUILD_DIR)/%.o,$(KERNEL_ASM_SOURCES))

.PHONY: all clean run disk-image

RUST_SOURCES := $(shell find $(RUST_DIR) -name '*.rs' -o -name 'Cargo.toml' -o -name 'config.toml')

$(RUST_LIB): $(RUST_SOURCES)
	mkdir -p $(BUILD_DIR)
	@if [ "$(RUST_REQUIRED)" = "1" ]; then \
		command -v cargo >/dev/null 2>&1 || (echo "[ERRO] cargo nao encontrado (instale Rust)"; exit 1); \
		test -d "$(RUST_DIR)" || (echo "[ERRO] diretório Rust ausente: $(RUST_DIR)"; exit 1); \
		cd "$(RUST_DIR)" && cargo build --release --target x86_64-unknown-none; \
		cp "$(CURDIR)/$(RUST_DIR)/target/x86_64-unknown-none/release/libnanarust.a" "$(CURDIR)/$(RUST_LIB)"; \
	else \
		# UI phase default: allow building without Rust target installed \
		if command -v cargo >/dev/null 2>&1 && [ -d "$(RUST_DIR)" ]; then \
			(cd "$(RUST_DIR)" && cargo build --release --target x86_64-unknown-none) && \
				cp "$(CURDIR)/$(RUST_DIR)/target/x86_64-unknown-none/release/libnanarust.a" "$(CURDIR)/$(RUST_LIB)" || \
				(ar rcs "$(CURDIR)/$(RUST_LIB)"); \
		else \
			ar rcs "$(CURDIR)/$(RUST_LIB)"; \
		fi; \
	fi

all: $(ISO_IMAGE)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(ISO_DIR):
	mkdir -p $(ISO_DIR)/boot/grub

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.S | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/boot.o: $(KERNEL_DIR)/boot.s | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $(KERNEL_DIR)/boot.s -o $@

# Build userspace hello program and generate embedded header
$(USERS_DIR)/hello/hello.elf: $(USERS_DIR)/hello/start.o $(USERS_DIR)/hello/hello.o
	$(CC) -nostdlib -static -T $(USERS_DIR)/hello/linker.ld -o $@ $^

$(USERS_DIR)/hello/hello.o: $(USERS_DIR)/hello/hello.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@


$(USERS_DIR)/hello/start.o: $(USERS_DIR)/hello/start.S
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Generate header used by kernel to populate RAMFS
$(KERNEL_DIR)/exec/hello_elf.h: $(USERS_DIR)/hello/hello.elf
	xxd -i $< > $@

# Ensure vfs.c is rebuilt after embedding header is generated
$(BUILD_DIR)/kernel/fs/vfs.o: $(KERNEL_DIR)/exec/hello_elf.h $(KERNEL_DIR)/fs/vfs.c | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $(KERNEL_DIR)/fs/vfs.c -o $@

$(KERNEL_ELF): $(BUILD_DIR)/boot.o $(KERNEL_OBJECTS) $(RUST_LIB)
	$(LD) $(LDFLAGS) -o $(KERNEL_ELF) $(BUILD_DIR)/boot.o $(KERNEL_OBJECTS) $(RUST_LIB)
$(DISK_IMAGE):
	mkdir -p $(BUILD_DIR)
	bash tools/create_fat32_image.sh "$(DISK_IMAGE)"

disk-image: $(DISK_IMAGE)


$(ISO_IMAGE): $(KERNEL_ELF) $(ISO_DIR) boot/grub/grub.cfg
	cp $(KERNEL_ELF) $(ISO_DIR)/boot/kernel.elf
	cp boot/grub/grub.cfg $(ISO_DIR)/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO_IMAGE) $(ISO_DIR) 2> /dev/null || xorrisofs -o $(ISO_IMAGE) -b boot/grub/i386-pc/eltorito.img -c boot.catalog $(ISO_DIR)

clean:
	rm -rf $(BUILD_DIR)

run: $(ISO_IMAGE) $(DISK_IMAGE)
	qemu-system-x86_64 -boot order=d -cdrom $(ISO_IMAGE) -drive file=$(DISK_IMAGE),format=raw,if=ide,index=0,media=disk -m 512M -smp 2 -serial stdio
