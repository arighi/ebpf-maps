# eBPF maps example

## Overview

This is a simple example to show how to use eBPF maps via libbpf, specifically
a `BPF_MAP_TYPE_RINGBUF` to produce kernel events from an eBPF program and
consume them from the user-space counterpart.

This simple program can be used also as a simple eBPF test case or as a
template to implement more complex eBPF tests.

## Usage

```
 $ make
 ...
 $ sudo ./main
 pid=129278 uid=1000 cmd=bash
 ...
```

## Implementation

The program consists of an eBPF part (`src/main.bpf.c`) and a user-space
counterpart (`src/main.c`).

The eBPF part intercepts the `execve()` syscall (using the `sys_enter_execve`
tracepoint), it collects some information about the task that is performing the
syscall and stores all the information into a ring buffer.

The ring buffer is then consumed from the user-space counterpart, printing the
collected information to stdout.
