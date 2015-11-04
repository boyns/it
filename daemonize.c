/******************************************************************************
*
*	daemonize.c
*
*	by Mark R Boyns
*
*/

#include <sys/types.h>
#include <sys/file.h>
#include <sys/ioctl.h>

daemonize()
{
	int fd;

	if (fork() != 0) {
		exit(0);
	}

	if (setpgrp(0, getpid())	< 0) {
		perror("setpgrp");
		exit(1);
	}

	if ((fd = open("/dev/tty", O_WRONLY, 0)) >= 0) {
		if (ioctl(fd, TIOCNOTTY, 0) < 0) {
			perror("ioctl");
			exit(1);
		}
		close(fd);
	}
	close(0);
	close(1);
	close(2);
}
