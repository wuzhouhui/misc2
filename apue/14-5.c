#include <fcntl.h>

/* 'lock_reg' short for 'lock region' */
int lock_reg(int fd, int cmd, int type, off_t offset, int whence, off_t len)
{
	struct flock lock;

	lock.l_type	= type;
	lock.l_start	= offset;
	lock.l_whence	= whence;
	lock.l_len	= len;

	return (fcntl(fd, cmd, &lock));
}
