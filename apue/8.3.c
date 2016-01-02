#include "apue.h"
#include <sys/wait.h>
#include <sys/types.h>

void pr_exit(int);

int main(void)
{
	pid_t pid;
	siginfo_t sinfo;

	if ((pid = fork()) < 0)
		err_sys("fork() error");
	else if (pid == 0)
		exit(7);

	if (waitid(P_PID, pid, &sinfo, WEXITED) == -1)
		err_sys("waitid() error");
	pr_exit(sinfo.si_status);
	exit(0);
}

void pr_exit(int status)
{
	if (WIFEXITED(status))
		printf("normal termination, exit status = %d\n", WEXITSTATUS(status));
	else if (WIFSIGNALED(status))
		printf("abnormal termination, signal number = %d %s\n", WTERMSIG(status),
#ifdef WCOREDUMP
				WCOREDUMP(status)? "(core file generated)" : "");
#else
	"");
#endif
	else if (WIFSTOPPED(status))
		printf("child stopped, signal number = %d\n", WSTOPSIG(status));
}

