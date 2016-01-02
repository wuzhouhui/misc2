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
#include <limits.h>
#include <sys/resource.h>

#define OPEN_MAX_GUESS 256

long open_max()
{
	long openmax;
	struct rlimit rl;
	if ((openmax = sysconf(_SC_OPEN_MAX)) < 0 || openmax == LONG_MAX) {
		if (getrlimit(RLIMIT_NOFILE, &rl) < 0)
			err_sys("can't get file limit");
		if (rl.rlim_max == RLIM_INFINITY) 
			openmax = OPEN_MAX_GUESS;
		else
			openmax = rl.rlim_max;
	}

	return openmax;
}
