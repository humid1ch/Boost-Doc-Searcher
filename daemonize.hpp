#pragma once

#include <cstdio>
#include <fcntl.h>
#include <iostream>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

void daemonize() {
	int fd = 0;

	// 1. 忽略SIGPIPE
	signal(SIGPIPE, SIG_IGN);
	// 2. 改变工作路径
	// chdir(const char *__path);
	// 3. 不要成为进程组组长

	if (fork() > 0) {
		exit(0);
	}
	// 4. 创建独立会话
	setsid();
	// 重定向文件描述符0 1 2
	if ((fd = open("/dev/null", O_RDWR)) != -1) { // 执行成功fd大概率为3
		dup2(fd, STDIN_FILENO);
		dup2(fd, STDOUT_FILENO);
		dup2(fd, STDERR_FILENO);

		// dup2三个标准流之后, fd就没有用了
		if (fd > STDERR_FILENO) {
			close(fd);
		}
	}
}
