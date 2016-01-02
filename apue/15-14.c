#include "apue.h"
#include <ctype.h>

int main(void)
{
	int c;
	while ((c = getchar()) != EOF) {
		if (isupper(c))
			c = tolower(c);
		if (putchar(c) == EOF)
			err_sys("output error");
		if (c == '\n')
			/*
			 * fflush() flushes all unwriten data 
			 * to kernel of stream
			 */
			fflush(stdout);
	}
	exit(0);
}
