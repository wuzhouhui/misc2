#include "apue.h"
#include <netdb.h>
#include <errno.h>
#include <syslog.h>
#include <sys/socket.h>
#include <sys/select.h>

#define BUFLEN	128
#define QLEN	10
#define SIZE	16

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 256
#endif

struct fd_addr {
	struct addrinfo addr;
	int		fd;
};

extern int initserver(int, const struct sockaddr *, socklen_t, int);

static void cloexec(const struct fd_addr *fd_addrs, int len)
{
	while (len >= 0)
		set_cloexec(fd_addrs[--len].fd);
}

void serve(fd_set *fdset, int maxfd, struct fd_addr *fd_addrs, int len)
{
	int	clfd;
	int 	i;
	FILE	*fp;
	char	buf[BUFLEN];

	cloexec(fd_addrs, len);
	for (;;) {
		if (select(maxfd + 1, fdset, NULL, NULL, NULL) == -1) {
			syslog(LOG_ERR, "ruptime: select error: %s",
					strerror(errno));
			exit(1);
		}
		for (i = 0; i < len; i++) {
			if (!FD_ISSET(fd_addrs[i].fd, fdset))
				continue;

			if ((clfd = accept(fd_addrs[i].fd, NULL, NULL)) < 0) {
				syslog(LOG_ERR, "ruptime: accept error: %s",
						strerror(errno));
				exit(1);
			}
			set_cloexec(clfd);
			if ((fp = popen("/usr/bin/uptime", "r")) == NULL) {
				sprintf(buf, "error: %s\n", strerror(errno));
				send(clfd, buf, strlen(buf), 0);
			} else {
				while (fgets(buf, BUFLEN, fp) != NULL)
					send(clfd, buf, strlen(buf), 0);
				pclose(fp);
			}
			close(clfd);
		}
	}
}

int main(int argc, char *argv[])
{
	struct addrinfo	*ailist, *aip;
	struct addrinfo	hint;
	struct fd_addr	fd_addrs[SIZE];
	fd_set		fdset;
	char		*host;
	int		i, sockfd, maxfd;
	int		n, err;

	if (argc != 1)
		err_quit("usage: ruptimed");
	if ((n = sysconf(_SC_HOST_NAME_MAX)) < 0)
		n = HOST_NAME_MAX; /* best guess */
	if ((host = malloc(n)) == NULL)
		err_sys("malloc error");
	if (gethostname(host, n) < 0)
		err_sys("gethostname error");
	daemonize("ruptimed");
	memset(&hint, 0, sizeof(hint));
	hint.ai_flags		= AI_CANONNAME;
	hint.ai_socktype	= SOCK_STREAM;
	hint.ai_canonname	= NULL;
	hint.ai_addr		= NULL;
	hint.ai_next		= NULL;
	if ((err = getaddrinfo(host, "ruptime", &hint, &ailist)) != 0) {
		syslog(LOG_ERR, "ruptimed: getaddrinfo error: %s",
				gai_strerror(err));
		exit(1);
	}
	FD_ZERO(&fdset);
	i = 0;
	for (aip = ailist; aip; aip = aip->ai_next) {
		if ((sockfd = initserver(SOCK_STREAM, aip->ai_addr,
						aip->ai_addrlen, 
						QLEN)) >= 0) {
			if (sockfd > maxfd)
				maxfd = sockfd;
			FD_SET(sockfd, &fdset);
			fd_addrs[i].fd	= sockfd;
			fd_addrs[i].addr = *aip;
			i++;
		}
	}
	if (i > 0)
		serve(&fdset, maxfd, fd_addrs, i);
	exit(1);
}
