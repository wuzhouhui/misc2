/*
 * =====================================================================================
 *
 *       Filename:  3-1.c
 *
 *    Description:  测试标准输入能否设置偏移量
 *
 *        Version:  1.0
 *        Created:  09/23/2014 11:04:24 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Wu Zhouhui (), 1530108435@qq.com
 *   Organization:  
 *
 * =====================================================================================
 */

#include "apue.h"

int main()
{
	if (lseek(STDIN_FILENO, 0, SEEK_CUR) == -1)
		printf("cannot seek\n");
	else
		printf("seek ok\n");
	return 0;
}
