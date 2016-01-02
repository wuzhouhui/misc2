#include "apue.h"
#include <stdio.h>

static void sig_pipe(int);

int main(void)
{
	int	n, fd1[2], fd2[2];
	pid_t	pid;
	char	line[MAXLINE];
	FILE	*fin, *fout;

	if (signal(SIGPIPE, sig_pipe) == SIG_ERR)
		err_sys("signal error");

	if (pipe(fd1) < 0 || pipe(fd2) < 0)
		err_sys("pipe error");

	if ((pid = fork()) < 0) {
		err_sys("fork error");
	} else if (pid > 0) { /* parent */
		close(fd1[0]);
		close(fd2[1]);

		if ((fout = fdopen(fd1[1], "w")) == NULL)
			err_sys("fdopen() for write error");
		if ((fin = fdopen(fd2[0], "r")) == NULL)
			err_sys("fdopen() for read error");

		/*
		 * the stream is full buffered when read from
		 * or write to pipe
		 */
		if (setvbuf(fin, NULL, _IOLBF, 0))
			err_sys("setvbuf for read error");
		if (setvbuf(fout, NULL, _IOLBF, 0))
			err_sys("setvbuf for write error");

		while (fgets(line, MAXLINE, stdin) != NULL) {
			n = strlen(line);

			if (fputs(line, fout) == EOF)
				err_sys("fputs error");
			if (fgets(line, MAXLINE, fin) == NULL) {
				err_msg("child closed pipe");
				break;
			}

			/*
			 * when read a pipe that write terminal is closed,
			 * read() return zero when all data are readed.
			 */
			if (n == 0) {
				err_msg("child closed pipe");
				break;
			}
			line[n] = '\0';
			if (fputs(line, stdout) == EOF)
				err_sys("fputs error");
		}

		if (ferror(stdin))
			err_sys("fgets error on stdin");
		exit(0);
	} else { /* child */
		close(fd1[1]);
		close(fd2[0]);

		if (fd1[0] != STDIN_FILENO) {
			if (dup2(fd1[0], STDIN_FILENO) != STDIN_FILENO)
				err_sys("dup2 error to stdin");
			close(fd1[0]);
		}

		if (fd2[1] != STDOUT_FILENO) {
			if (dup2(fd2[1], STDOUT_FILENO) != STDOUT_FILENO)
				err_sys("dup2 error to stdout");
			close(fd2[1]);
		}

		if (execl("./add2", "add2", (char *)0) < 0)
			err_sys("execl error");
	}
	exit(0);
}

/*
 * our signal handler
 */
static void sig_pipe(int signo)
{
	printf("SIGPIPE caught\n");
	exit(1);
}
