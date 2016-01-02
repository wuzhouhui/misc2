/*
 * =====================================================================================
 *
 *       Filename:  1-4.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  09/21/2014 11:22:07 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Wu Zhouhui (), 1530108435@qq.com
 *   Organization:  
 *
 * =====================================================================================
 */

#include "apue.h"

#ifndef BUFSIZE
#define BUFSIZE 1024
#endif

int main()
{
	char	buf[BUFSIZE];
	int		n;
	
	while ((n = read(STDIN_FILENO, buf, BUFSIZE)) > 0)
		if (write(STDOUT_FILENO, buf, n) != n)
			err_sys("write error");

	if (n < 0)
		err_sys("read error");
	exit(0);
}
