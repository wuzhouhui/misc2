#include "apue.h"
#include <unistd.h>
#include <stdlib.h>

int main(void)
{
	pid_t pid;

	if ((pid = fork()) < 0)
		err_sys("fork() error");
	else if (pid == 0) /* child */
		exit(0);

	/* parent */
	sleep(4); /* enough time for child to die */
	system("ps -o pid,ppid,state,tty,command");
	exit(0);
}
