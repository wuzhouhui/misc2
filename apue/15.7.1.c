#include "apue.h"
#include <sys/types.h>
#include <sys/select.h>
#include <unistd.h>

/*
 * child turn off pipe both read and write,
 * parent read from pipe. test select() of pipe
 * in parent
 */
int main(void)
{
	int	fd[2];
	pid_t	pid;
	fd_set	rdfdset, wrfdset;

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

		FD_ZERO(&rdfdset);
		FD_ZERO(&wrfdset);
		FD_SET(fd[0], &rdfdset);
		FD_SET(fd[0], &wrfdset);
		if (select(fd[0] + 1, &rdfdset, &wrfdset, NULL, NULL) == -1)
			err_sys("select error");
		printf("%s\n", (FD_ISSET(fd[0], &rdfdset) ? "readable" : "unreadable"));
		printf("%s\n", (FD_ISSET(fd[0], &wrfdset) ? "writable" : "unwritable"));
	}
	exit(0);
}
