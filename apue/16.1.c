#include "apue.h"

int main(void)
{
	long int a = 1;

	printf("%s\n", (*(char *)&a ? "small end" : "big end"));
	exit(0);
}
