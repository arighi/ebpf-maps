/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2024 Andrea Righi <andrea.righi@canonical.com>
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <sys/resource.h>
#include <argp.h>
#include <assert.h>

#include "bpf/libbpf.h"
#include "bpf/bpf.h"

#include "main.bpf.skel.h"
#include "main.h"

/* Maximum amount of records to consume at once from the BPF ring buffer */
#define MAX_EVENTS	4

const char argp_program_doc[] =
	"A simple eBPF CO-RE program to test libbpf maps\n";

static const struct argp_option opts[] = {
	{NULL, 'h', NULL, OPTION_HIDDEN, "Show the full help"},
	{},
};

static volatile bool exiting = false;

static void sig_handler(int sig)
{
	exiting = true;
}

static error_t parse_arg(int key, char *arg, struct argp_state *state)
{
	switch (key) {
	case 'h':
		argp_usage(state);
		break;
	default:
		return ARGP_ERR_UNKNOWN;
	}

	return 0;
}

static int handle_event(void *ctx, void *data, size_t data_sz)
{
	const struct item *item = data;

	fprintf(stdout, "pid=%u uid=%u cmd=%s\n",
		item->pid, item->uid, item->comm);

	return 0;
}

int main(int argc, char **argv)
{
	struct rlimit rlim = {
		.rlim_cur = RLIM_INFINITY,
		.rlim_max = RLIM_INFINITY,
	};
	const struct argp argp = {
		.options = opts,
		.parser = parse_arg,
		.doc = argp_program_doc,
	};
	struct main_bpf *skel = NULL;
	struct ring_buffer *rb = NULL;
	int err = 0;

	err = setrlimit(RLIMIT_MEMLOCK, &rlim);
	if (err) {
		fprintf(stderr, "failed to change rlimit\n");
		return 1;
	}

	err = argp_parse(&argp, argc, argv, 0, NULL, NULL);
	if (err) {
		fprintf(stderr, "failed to parse command line arguments\n");
		return 1;
	}

	skel = main_bpf__open();
	if (!skel) {
		fprintf(stderr, "failed to open and/or load BPF object\n");
		return 1;
	}

	err = main_bpf__load(skel);
	if (err) {
		fprintf(stderr, "failed to load BPF object %d\n", err);
		goto cleanup;
	}

	err = main_bpf__attach(skel);
	if (err) {
		fprintf(stderr, "failed to attach BPF programs\n");
		goto cleanup;
	}

	rb = ring_buffer__new(bpf_map__fd(skel->maps.items), handle_event,
			      NULL, NULL);
	if (!rb) {
		err = -1;
		fprintf(stderr, "failed to create ring buffer\n");
		goto cleanup;
	}

	signal(SIGINT, sig_handler);
	signal(SIGTERM, sig_handler);

	while (!exiting) {
		err = ring_buffer__consume_n(rb, MAX_EVENTS);
		if (err == -EINTR) {
			err = 0;
			break;
		}
		if (err < 0) {
			fprintf(stderr,
				"failed polling from ring buffer: %d\n", err);
			break;
		}
		if (err > 0) {
			fprintf(stdout, "consumed %d records\n", err);
			assert(err <= MAX_EVENTS);
		}
	}

cleanup:
	ring_buffer__free(rb);
	main_bpf__destroy(skel);

	return err < 0 ? : 0;
}
