#include "apue.h"

int main(void)
{
	FILE	*fp;
	int	ret;

	if ((fp = popen("nocmd", "r")) == NULL)
		err_sys("popen() error");
	if ((ret = pclose(fp)) == -1)
		err_sys("pclose() error");

	printf("exit code of cmdstring: %d\n", ret);
	exit(0);
}
