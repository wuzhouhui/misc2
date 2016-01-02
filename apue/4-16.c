#include "apue.h"
#include <fcntl.h>
int main()
{
	open("tempfile", O_RDWR);
	unlink("tempfile");
	printf("file unlinked\n");
	sleep(15);
	printf("done\n");
	exit(0);
}
