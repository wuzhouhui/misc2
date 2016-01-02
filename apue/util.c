/*
 * =====================================================================================
 *
 *       Filename:  util.c
 *
 *    Description:  util program for print and printd
 *
 *        Version:  1.0
 *        Created:  01/18/2015 04:07:49 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  apue 3rd edition
 *   Organization:  
 *
 * =====================================================================================
 */

#include "apue.h"
#include "print.h"
#include <ctype.h>
#include <sys/select.h>

#define MAXCFGLINE 512
#define MAXKWLEN   16
#define MAXFMTLEN  16

/*
 * get the address list for the given host and service and 
 * return through ailistpp. returns 0 on success or an error 
 * code on failure. note that we do not set errno if we 
 * encounter an error.
 *
 * LOCKING: none.
 */
int getaddrlist(const char *host, const char *service,
		struct addrinfo **ailistpp)
{
	int	err;
	struct addrinfo hint;

	hint.ai_flags	  = AI_CANONNAME;
	hint.ai_family	  = AF_INET;
	hint.ai_socktype  = SOCK_STREAM;
	hint.ai_protocol  = 0;
	hint.ai_canonname = NULL;
	hint.ai_addr	  = NULL;
	hint.ai_next      = NULL;
	err = getaddrinfo(host, service, &hint, ailistpp);
	return err;
}

/*
 * giver a key word, scan the configuration file for a match 
 * and return the string value corresponding to the key word.
 *
 * LOCKING: none.
 */
static char *scan_configfile(const char *keyword)
{
	int	n, match;
	FILE	*fp;
	char	keybuf[MAXKWLEN], pattern[MAXFMTLEN];
	char	line[MAXCFGLINE];
	static	char valbuf[MAXCFGLINE];

	if ((fp = fopen(CONFIG_FILE, "r")) == NULL)
		log_sys("can't open %s", CONFIG_FILE);
	sprintf(pattern, "%%%ds %%%ds", MAXKWLEN - 1, MAXCFGLINE - 1);
	match = 0;
	while (fgets(line, MAXCFGLINE, fp) != NULL) {
		n = sscanf(line, pattern, keybuf, valbuf);
		if (n == 2 && strcmp(keyword, keybuf) == 0) {
			match = 1;
			break;
		}
	}
	fclose(fp);
	if (match != 0)
		return valbuf;
	else
		return NULL;
}

/*
 * return the host name running the print server or NULL on error.
 *
 * LOCKING: none.
 */
char *get_printserver(void)
{
	return scan_configfile("printserver");
}

/*
 * return the address of the network printer or NULL on error.
 *
 * LOCKING: none.
 */
struct addrinfo *get_printaddr(void)
{
	int	err;
	char	*p;
	struct	addrinfo *ailist;

	if ((p = scan_configfile("printer")) != NULL) {
		if ((err = getaddrlist(p, "ipp", &ailist)) != 0) {
			log_msg("no address information for %s", p);
			return NULL;
		}
		return ailist;
	}
	log_msg("no printer address specified");
	return NULL;
}

/*
 * "timed" read - timeout specifies the # of seconds to wait before 
 * giving up (5th argument to select controls how long to wait for 
 * data to be readable). returns # of bytes read or -1 on error.
 */
ssize_t tread(int fd, void *buf, size_t nbytes, unsigned int timeout)
{
	int	nfds;
	fd_set	readfds;
	struct	timeval tv;

	tv.tv_sec  = timeout;
	tv.tv_usec = 0;
	FD_ZERO(&readfds);
	FD_SET(fd, &readfds);
	nfds = select(fd + 1, &readfds, NULL, NULL, &tv);
	if (nfds <= 0) {
		if (nfds == 0)
			errno = ETIME;
		return -1;
	}
	return read(fd, buf, nbytes);
}

/*
 * "timed" read - timeout specifies the number of seconds to wait 
 * per read call before giving up, but read exactly nbytes bytes.
 * returns number of bytes read or -1 on error.
 *
 * LOCKING: none.
 */
ssize_t treadn(int fd, void *buf, size_t nbytes, unsigned int timeout)
{
	size_t	nleft;
	ssize_t	nread;

	nleft = nbytes;
	while (nleft > 0) {
		if ((nread = tread(fd, buf, nleft, timeout)) < 0) {
			if (nleft == nbytes)
				return -1; /* error, return -1 */
			else 
				break;	/* error, return amount read so far */
		} else if (nread == 0) {
			break;	/* EOF */
		}
		nleft	-= nread;
		buf	+= nread;
	}
	return (nbytes - nleft);	/* return >= 0 */
}
