/*
 * =============================================================================
 *
 *       Filename:  example.c
 *
 *    Description:  a example for __func__
 *
 *        Version:  1.0
 *        Created:  03/20/2015 06:13:31 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =============================================================================
 */

#include <stdio.h>

static void example(void)
{
	printf("this is function: %s\n", __func__);
}

void main(void)
{
	printf("this is function: %s\n", __func__);
	example();
}
