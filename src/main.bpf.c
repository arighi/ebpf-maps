/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2024 Andrea Righi <andrea.righi@canonical.com>
 */
#include "vmlinux.h"
#include "bpf/bpf_helpers.h"
#include "main.h"

#define MAX_ITEMS	1024

char _license[] SEC("license") = "GPL";

struct {
	__uint(type, BPF_MAP_TYPE_RINGBUF);
	__uint(max_entries, MAX_ITEMS);
} items SEC(".maps");

SEC("tracepoint/syscalls/sys_enter_execve")
int
tracepoint__syscalls__sys_enter_execve(struct trace_event_raw_sys_enter *ctx)
{
	struct item *item;

	item = bpf_ringbuf_reserve(&items, sizeof(*item), 0);
	if (!item)
		return 0;

	item->pid = (pid_t)bpf_get_current_pid_tgid();
	item->uid = (uid_t)bpf_get_current_uid_gid();
	bpf_get_current_comm(&item->comm, sizeof(item->comm));
	bpf_ringbuf_submit(item, 0);

	return 0;
}
