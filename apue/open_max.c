/*
 * =====================================================================================
 *
 *       Filename:  2-17.c
 *
 *    Description:  determing the max of fd
 *
 *        Version:  1.0
 *        Created:  09/21/2014 08:05:26 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Wu Zhouhui (), 1530108435@qq.com
 *   Organization:  
 *
 * =====================================================================================
 */

#include "apue.h"
#include <errno.h>
#include <limits.h>

#ifdef OPEN_MAX
static long openmax = OPEN_MAX;
#else
static long openmax = 0;
#endif
#define OPEN_MAX_GUESS 256

long open_max()
{
	if (openmax == 0) {
		errno = 0;
		if ((openmax = sysconf(_SC_OPEN_MAX)) < 0) {
			if (errno == 0)
				openmax = OPEN_MAX_GUESS;
			else
				err_sys("sysconf error for _SC_OPEN_MAX");
		}
	}

	return openmax;
}
