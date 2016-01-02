#include "apue.h"
#include <sys/types.h>
#include <unistd.h>
#include <poll.h>

/*
 * child turn off pipe both read and write,
 * parent read from pipe. test poll() of pipe
 * in parent
 */
int main(void)
{
	int		fd[2];
	pid_t		pid;
	struct pollfd	pollfd[1];

	if (pipe(fd) == -1)
		err_sys("pipe error");
	TELL_WAIT();

	if ((pid = fork()) == -1) {
		err_sys("fork error");
	} else if (pid == 0) { /* child */
		close(fd[0]);
		close(fd[1]);
		TELL_PARENT(getppid());
	} else { /* parent */
		close(fd[1]);
		WAIT_CHILD();

		pollfd[0].fd	 = fd[0];
		pollfd[0].events = POLLRDNORM | POLLOUT;

		if (poll(&pollfd[0], 1, 0) == -1)
			err_sys("poll error");
		if (pollfd[0].revents & POLLHUP)
			printf("HUP\n");
	}
	exit(0);
}
