#include "apue.h"
#include <sys/socket.h>

#define MAXSLEEP 128

int connect_retry(int domain, int type, int protocol,
		const struct sockaddr *addr, socklen_t alen)
{
	int numsec, fd;

	/*
	 * try to connect with exponential backoff
	 */
	for (numsec = 1; numsec <= MAXSLEEP; numsec <<= 1) {
		if ((fd = socket(domain, type, protocol)) < 0)
			return -1;
		if (connect(fd, addr, alen) == 0) {
			/*
			 * connection accepted
			 */
			return fd;
		}
		close(fd);

		/*
		 * delay before trying again
		 */
		if (numsec <= MAXSLEEP / 2)
			sleep(numsec);
	}
}
