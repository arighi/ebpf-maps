/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2024 Andrea Righi <andrea.righi@canonical.com>
 */
#ifndef MAIN_H
#define MAIN_H

#define TASK_COMM_LEN 16

struct item {
	char comm[TASK_COMM_LEN];
	pid_t pid;
	uid_t uid;
};

#endif /* MAIN_H */
