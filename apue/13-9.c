#include "apue.h"
#include <fcntl.h>

int set_cloexec(int fd)
{
	int val;

	/*
	 * F_GETFD: get flags of fd
	 */
	if ((val = fcntl(fd, F_GETFD, 0)) < 0)
		return -1;

	val |= FD_CLOEXEC; /* enable close-on-exec */

	/*
	 * F_SETFD: set falgs of fd.
	 */
	return (fcntl(fd, F_SETFD, val));
}
