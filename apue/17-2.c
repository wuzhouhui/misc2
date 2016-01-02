#include "apue.h"
#include <sys/socket.h>

/*
 * return a full-duplex pipe (a unix domain socket) with
 * the two file descriptors returned in fd[0] and fd[1]
 */
int fd_pipe(int fd[2])
{
	/*
	 * AF_UNIX: unix domain.
	 * SOCK_STREAM: ordered, reliable, bidirected and
	 * connected byte stream.
	 * 0: default protocol according to domain and 
	 * type
	 */
	return socketpair(AF_UNIX, SOCK_STREAM, 0, fd);
}
