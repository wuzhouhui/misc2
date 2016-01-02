#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void)
{
	printf("%s\n", ctermid(NULL));
	return 0;
}
