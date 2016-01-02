#include "apue.h"
#include <errno.h>
#include <sys/socket.h>

int initserver(int type, const struct sockaddr *addr, socklen_t alen,
		int qlen)
{
	int fd;
	int err = 0;

	/*
	 * 0 to select default protocol for domain and type specified
	 */
	if ((fd = socket(addr->sa_family, type, 0)) < 0)
		return -1;
	if (bind(fd, addr, alen) < 0)
		goto errout;
	if (type == SOCK_STREAM || type == SOCK_SEQPACKET) {
		/*
		 * it's connecte faced, call listen() to demanstrate
		 * that the fd can accept connect request
		 */
		if (listen(fd, qlen) < 0)
			goto errout;
	}
	return fd;

errout:
	err = errno;
	close(fd);
	errno = err;
	return -1;
}
