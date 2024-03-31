CC		:= clang
BPFTOOL		:= bpftool

LIBBPF_REPO	:= https://github.com/libbpf/libbpf
LIBBPF_DIR	:= libbpf

CFLAGS		:= -g -Wall -O2 -I. -I$(LIBBPF_DIR)/build
BPF_CFLAGS	:= -target bpf -D__TARGET_ARCH_x86_64

# Source files
SRC_BPF		:= src/main.bpf.c
SRC_MAIN	:= src/main.c

# Object files
OBJ_BPF		:= $(patsubst %.c, %.o, $(SRC_BPF))
OBJ_MAIN	:= $(patsubst %.c, %.o, $(SRC_MAIN))
LIBBPF_A	:= $(LIBBPF_DIR)/build/libbpf.a

.PHONY: all clean

all: $(LIBBPF_A) vmlinux.h main

$(LIBBPF_A): $(LIBBPF_DIR)
	$(MAKE) -C $(LIBBPF_DIR)/src BUILD_STATIC_ONLY=1 \
			OBJDIR=../build/$(LIBBPF_DIR) \
			DESTDIR=../build \
			INCLUDEDIR= LIBDIR= UAPIDIR= install

$(LIBBPF_DIR):
	git clone $(LIBBPF_REPO) $(LIBBPF_DIR)

vmlinux.h:
	$(BPFTOOL) btf dump file /sys/kernel/btf/vmlinux format c > vmlinux.h

$(OBJ_BPF): %.o: %.c
	$(CC) $(CFLAGS) $(BPF_CFLAGS) -c $< -o $@
	$(BPFTOOL) gen skeleton $@ > $(patsubst %.o, %.skel.h, $@)

$(OBJ_MAIN): %.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

main: $(OBJ_BPF) $(OBJ_MAIN)
	$(CC) $(CFLAGS) $(OBJ_MAIN) $(LIBBPF_DIR)/build/libbpf.a -lelf -lz -o $@

clean:
	rm -rf src/*.o src/*.skel.h main vmlinux.h
