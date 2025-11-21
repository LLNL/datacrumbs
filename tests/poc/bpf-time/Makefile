# Define the names of our source and output files
TARGET = victim
BPF_PROG_NAME = malloc
BPF_PROG_SRC = $(BPF_PROG_NAME).bpf.c
BPF_PROG_OBJ = $(BPF_PROG_NAME).bpf.o
BPF_LOADER_SRC = $(BPF_PROG_NAME).c
BPF_LOADER = $(BPF_PROG_NAME)
BPF_SKEL = $(BPF_PROG_NAME).skel.h

# C compiler and flags for the user-space programs
CC ?= gcc
CLANG ?= clang
CFLAGS ?= -Wall -O2 -g
LDFLAGS ?= -l:libbpf.so -lrt -lz -lelf -ldl -lpthread

# BPF compiler and flags
BPF_CFLAGS = -Wall -O2 -g -target bpf
# Generate vmlinux.h from the current kernel's BTF data
VMLINUX_H_PATH = ./vmlinux.h

# BPFTOOL for generating the skeleton header
BPFTOOL ?= bpftool

# libbpf and headers location
LIBBPF_DIR ?= $(PREFIX)
LIBBPF_INCLUDE = $(LIBBPF_DIR)/include
LIBBPF_LIB = $(LIBBPF_DIR)/lib64
ARCH        := $(shell uname -m | sed 's/x86_64/x86/' | sed 's/aarch64/arm64/')

# Build rule for all targets
all: $(TARGET) $(BPF_LOADER)

# Rule to compile the victim application
$(TARGET): victim.c
	$(CC) $(CFLAGS) $< -o $@

# Rule to generate vmlinux.h for BPF compilation
$(VMLINUX_H_PATH):
	@echo "Generating $(VMLINUX_H_PATH)..."
	$(BPFTOOL) btf dump file /sys/kernel/btf/vmlinux format c > $@

# Rule to compile the BPF program
$(BPF_PROG_OBJ): $(BPF_PROG_SRC) $(VMLINUX_H_PATH)
	$(CLANG) $(BPF_CFLAGS) -D __TARGET_ARCH_$(ARCH) -I$(LIBBPF_INCLUDE) -I. -c $< -o $@

# Rule to generate the BPF skeleton header
$(BPF_SKEL): $(BPF_PROG_OBJ)
	$(BPFTOOL) gen skeleton $< > $@

# Rule to compile the user-space loader
$(BPF_LOADER): $(BPF_LOADER_SRC) $(BPF_SKEL)
	$(CC) $(CFLAGS) -I$(LIBBPF_INCLUDE) -o $@ $(BPF_LOADER_SRC) $(LIBBPF_LIB)/libbpf.so $(LDFLAGS)

# Clean up all generated files
clean:
	rm -f $(TARGET) $(BPF_LOADER) $(BPF_PROG_OBJ) $(BPF_SKEL) $(VMLINUX_H_PATH)
