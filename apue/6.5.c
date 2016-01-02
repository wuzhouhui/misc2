/* 
 * 获取当前时间, 并使用 strftime 将输出结果转换为类似
 * 于 date(1) 命令的默认输出
 */

#include "apue.h"
#include <stdio.h>
#include <time.h>

#ifndef MAXLINE
#define MAXLINE 1024
#endif

int main()
{
	time_t 		t;
	struct tm 	*tmptr;
	char 		buf[MAXLINE];

	if (time(&t) == (time_t)-1)
		err_sys("time() error");
	if ((tmptr = localtime(&t)) == NULL)
		err_sys("localtime() error");
	if (strftime(buf, MAXLINE, "%a %b %d %X %Z %Y", tmptr) == 0)
		err_sys("strftime() error");
	printf("%s\n", buf);
	exit(0);
}
