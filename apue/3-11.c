#include "apue.h"
#include <fcntl.h>
int main(int argc, char *argv[]){
	int val;
	if (argc != 2)
		err_quit("usage: a.out <descriptor>");
	val = fcntl(atoi(argv[1]), F_GETFL, 0);
	switch (val & O_ACCMODE) {
	case O_RDONLY:
		printf("read only");
		break;
	case O_WRONLY:
		printf("write only");
		break;
	case O_RDWR:
		printf("read and write");
		break;
	default:
		printf("someting else");
	}
	if (val & O_APPEND)
		printf(", append");
	if (val & O_NONBLOCK)
		printf(", nonblock");
	if (val & O_SYNC)
		printf(", synchronous writes");

	putchar('\n');
	exit(0);
}
