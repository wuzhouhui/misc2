#include <errno.h>
#include <stdio.h>
#include <signal.h>

void pr_mask(const char *str)
{
	sigset_t sigset;
	int      errno_save;

	errno_save = errno;
	if (sigprocmask(0, NULL, &sigset) < 0) {
		printf("sigprocmask() error\n");
	} else {
		printf("%s", str);
		if (sigismember(&sigset, SIGQUIT))
			printf(" SIGQUIT\n");
		if (sigismember(&sigset, SIGINT))
			printf(" SIGINT\n");
		if (sigismember(&sigset, SIGUSR1))
			printf(" SIGUSR1\n");
		if (sigismember(&sigset, SIGUSR2))
			printf(" SIGUSR2\n");
	}
}
