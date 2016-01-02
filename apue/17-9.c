#include "apue.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <errno.h>

/*
 * client's name can't be older than this (sec).
 * "stale" means unfresh
 */
#define STALE 30

/*
 * wait for a client connection to arrive, and accept it.
 * we also obtain the client's user id from the pathname
 * that it must bind before calling us.
 * returns new fd if all ok, negative on error
 */
int serv_accept(int listenfd, uid_t *uidptr)
{
	int			clifd, err, rval;
	socklen_t		len;
	struct sockaddr_un	un;
	time_t			staletime;
	struct stat		statbuf;
	char			*name;

	/* 
	 * allocate enough space for longest name plus
	 * terminating null
	 */
	if ((name = malloc(sizeof(un.sun_path) + 1)) == NULL)
		return -1;
	len = sizeof(un);
	if ((clifd = accept(listenfd, (struct sockaddr *)&un, &len)) < 0) {
		free(name);
		return -2; /* often errno = EINTR, if signal caught */
	}

	/*
	 * obtain the client's uid from its calling address 
	 */
	len -= offsetof(struct sockaddr_un, sun_path); /* length of pathname */
	memcpy(name, un.sun_path, len);
	name[len] = 0;
	if (stat(name, &statbuf) < 0) {
		rval = -3;
		goto errout;
	}

#ifdef S_ISSOCK	/* not defined for SVR4 */
	if (S_ISSOCK(statbuf.st_mode) == 0) {
		rval = -4;	/* not a socket */
		goto errout;
	}
#endif

	/*
	 * S_IRWXG: ---rwx---
	 * S_IRWXO: ------rwx
	 * S_IRWXU: rwx------
	 */
	if ((statbuf.st_mode & (S_IRWXG | S_IRWXO)) ||
			(statbuf.st_mode & S_IRWXU) != S_IRWXU) {
		rval = -5;	/* is not rwx------ */
		goto errout;
	}

	staletime = time(NULL) - STALE;
	if (statbuf.st_atime < staletime ||
			statbuf.st_ctime < staletime ||
			statbuf.st_mtime < staletime) {
		rval = -6; /* i-node is too old */
		goto errout;
	}

	if (uidptr != NULL)
		*uidptr = statbuf.st_uid; /* return uid of caller */
	unlink(name); /* we're done with pathname now */
	free(name);
	return clifd;

errout:
	err = errno;
	close(clifd);
	free(name);
	errno = err;
	return rval;
}
