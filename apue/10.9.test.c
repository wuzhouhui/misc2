#include <signal.h>
#include <stdlib.h>

void pr_mask(const char *);

int main(void)
{
	sigset_t sigset;

	pr_mask("before sigprocmask(): ");

	sigemptyset(&sigset);
	sigaddset(&sigset, SIGUSR1);
	sigaddset(&sigset, SIGUSR2);
	sigaddset(&sigset, SIGINT);

	if (sigprocmask(SIG_BLOCK, &sigset, NULL) == -1)
		perror("sigprocmask() error");
	else
		pr_mask("after sigprocmask(): ");

	exit(0);
}
