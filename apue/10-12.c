#include "apue.h"
#include <stdio.h>

void pr_bits(int no, int len)
{
	int i = 0;
	for (; i < len; i++) {
		if (((no = 1 << no) & 0x80000000) != 0)
			putchar('1');
		else 
			putchar('0');
	}
	putchar('\n');
}

int main(void)
{
	int j = 8 * sizeof(int);

	printf("%#x\n", 0);
	printf("%#x\n", 1);
	printf("%#x\n", 2);

	/*
	pr_bits((int)0, j);
	pr_bits((int)1, j);
	pr_bits((int)2, j);
	*/

	exit(0);
}

