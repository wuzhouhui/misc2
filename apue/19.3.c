#include "apue.h"
#include <sys/select.h>

#define BUFFSIZE 512

void loop(int ptym, int ignoreeof)
{
	fd_set	fdset;
	int	nread;
	char	buf[BUFFSIZE];
	int	maxfd;

	FD_ZERO(&fdset);
	FD_SET(ptym, &fdset);
	FD_SET(STDIN_FILENO, &fdset);

	if (ptym > STDIN_FILENO)
		maxfd = ptym;
	else 
		maxfd = STDIN_FILENO;

	for (;;) {
		if (select(maxfd + 1, &fdset, NULL, NULL, NULL) < 0)
			err_sys("select error");

		if (FD_ISSET(STDIN_FILENO, &fdset)) {
			if ((nread = read(STDIN_FILENO, buf, BUFFSIZE)) < 0)
				err_sys("read error from stdin");
			else if (nread > 0 && writen(ptym, buf, nread) != nread)
				err_sys("writen error to master pty");
		}

		if (FD_ISSET(ptym, &fdset)) {
			if ((nread = read(ptym, buf, BUFFSIZE)) <= 0)
				break;	/* signal caught, error or EOF */
			else if (writen(STDOUT_FILENO, buf, nread) != nread)
				err_sys("writen error to stdout");
		}
		
	}
}
