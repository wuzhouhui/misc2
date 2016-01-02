#include <errno.h>
#include <signal.h>
#include <stdio.h>

static struct {
	int signo;
	char signame[16];
} sig_id[] = {
	{SIGINT, "SIGINT"},
	{SIGQUIT, "SIGQUIT"},
	{SIGUSR1, "SIGUSR1"},
	{SIGUSR2, "SIGUSR2"},
	{SIGALRM, "SIGALRM"}
};

void pr_mask(const char *str)
{
	sigset_t sigset;
	int errno_save;
	int i, nsig = sizeof(sig_id) / sizeof(sig_id[0]);

	if (sigprocmask(0, NULL, &sigset) == -1) {
		perror("sigprocmask() error");
	} else {
		printf("%s", str);

		for (i = 0; i < nsig; i++) {
			if (sigismember(&sigset, sig_id[i].signo))
				printf(" %s", sig_id[i].signame);
		}
	}

	putchar('\n');
}
