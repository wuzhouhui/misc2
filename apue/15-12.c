/*
 * our own popen() and pclose()
 */

#include "apue.h"
#include <errno.h>
#include <fcntl.h>
#include <sys/wait.h>

/*
 * pointer to array allocated at run-time.
 */
static pid_t *childpid = NULL;
/*
 * our own open_max(), {Figure 2.17}
 */
static int maxfd;

FILE *popen(const char *cmdstring, const char *type)
{
	int	i;
	int	pfd[2];
	pid_t	pid;
	FILE	*fp;

	/* only allow "r" or "w" */
	if ((type[0] != 'r' && type[0] != 'w') || type[1] != '\0') {
		errno = EINVAL;
		return (NULL);
	}

	if (childpid == NULL) {
		maxfd = open_max();
		if ((childpid = calloc(maxfd, sizeof(pid_t))) == NULL)
			return (NULL);
	}

	if (pipe(pfd) < 0)
		return (NULL);
	if (pfd[0] >= maxfd || pfd[1] >= maxfd) {
		close(pfd[0]);
		close(pfd[1]);
		errno = EMFILE;
		return (NULL);
	}
	
	if ((pid = fork()) < 0) {
		return (NULL);
	} else if (pid == 0) {
		if (*type == 'r') {
			close(pfd[0]);
			if (pfd[1] != STDOUT_FILENO) {
				dup2(pfd[1], STDOUT_FILENO);
				close(pfd[0]);
			}
		} else {
			close(pfd[1]);
			if (pfd[0] != STDOUT_FILENO) {
				dup2(pfd[0], STDOUT_FILENO);
				close(pfd[0]);
			}
		}

		/* close all descriptors in childpid */
		for (i = 0; i < maxfd; i++)
			close(i);

		execl("/bin/sh", "sh", "-c", cmdstring, (char *)0);
		_exit(127); /* if it executes, that would be a error */
	}

	/* parent continue */
	if (*type == 'r') {
		close(pfd[1]);
		if ((fp = fdopen(pfd[0], type)) == NULL)
			return (NULL);
	} else {
		close(pfd[0]);
		if ((fp = fdopen(pfd[1], type)) == NULL)
			return (NULL);
	}
	childpid[fileno(fp)] = pid;
	return (fp);
}

int pclose(FILE *fp)
{
	int	fd, stat;
	pid_t	pid;

	if (childpid == NULL) {
		errno = EINVAL;
		return (-1);
	}

	fd = fileno(fp);
	if (fd >= maxfd) {
		errno = EINVAL;
		return (-1);
	}
	if ((pid = childpid[fd]) == 0) {
		errno = EINVAL;
		return (-1);
	}

	childpid[fd] = 0;
	if (fclose(fp) == EOF)
		return (-1);

	while (waitpid(pid, &stat, 0) < 0)
		if (errno != EINTR)
			return (-1);

	return stat;
}
