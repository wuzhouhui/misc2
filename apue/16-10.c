#include "apue.h"
#include <sys/socket.h>

#define MAXSLEEP 128

int connect_retry(int sockfd, const struct sockaddr *addr, socklen_t alen)
{
	int numsec;

	/*
	 * try to connect with exponential backoff
	 */
	for (numsec = 1; numsec <= MAXSLEEP; numsec <<= 1)
	{
		if (connect(sockfd, addr, alen) == 0) {
			/*
			 * connection accepted
			 */
			return 0;
		}

		/*
		 * delay before trying again
		 */
		if (numsec <= MAXSLEEP / 2)
			sleep(numsec);
	}
	return -1;
}
