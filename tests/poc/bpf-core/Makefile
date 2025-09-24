TARGET      := example
ARCH        := $(shell uname -m | sed 's/x86_64/x86/' | sed 's/aarch64/arm64/')
BPF_SRCS    := $(wildcard *.bpf.c)
BPF_OBJS    := $(BPF_SRCS:.bpf.c=.bpf.o)
MERGED_OBJ  := merged.bpf.o
SKEL        := $(TARGET).skel.h
USER_C      := $(TARGET).c

all: vmlinux.h $(TARGET)

$(BPF_OBJS): %.bpf.o: %.bpf.c
	clang -target bpf -D __TARGET_ARCH_$(ARCH) -Wall -O2 -g -c $< -o $@
	llvm-strip -g $@

$(MERGED_OBJ): $(BPF_OBJS)
	bpftool gen object $@ $(BPF_OBJS)

$(SKEL): $(MERGED_OBJ)
	bpftool gen skeleton $< > $@

$(TARGET): $(USER_C) $(SKEL)
	gcc -Wall -o $@ $(USER_C) -L../libbpf/src -l:libbpf.so -lelf -lz

vmlinux.h:
	bpftool btf dump file /sys/kernel/btf/vmlinux format c > vmlinux.h

clean:
	- rm -f $(BPF_OBJS) $(MERGED_OBJ) $(SKEL) $(TARGET)
