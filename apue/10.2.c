#include <string.h>
#include <signal.h>

int my_sig2str(int signo, char *str)
{
	if (signo <= 0 || signo >= NSIG)
		return -1;
	strcpy(str, sys_siglist[signo]);
	return 1;
}
