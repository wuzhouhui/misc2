#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>

void pflags(int fd);

int main(void)
{
	DIR *dir;
	const char *dirname = "/";
	int fd;

	if ((dir = opendir(dirname)) == NULL) {
		perror("opendir");
		return EXIT_FAILURE;
	}
	pflags(dirfd(dir));

	if ((fd = open(dirname, O_RDONLY)) == -1) {
		closedir(dir);
		perror("open");
		return EXIT_FAILURE;
	}
	pflags(fd);
	close(fd);
	closedir(dir);
	return EXIT_SUCCESS;
}

void pflags(int fd)
{
	int flags;
	if ((flags = fcntl(fd, F_GETFD)) != -1)
		printf("close-on-exec: %s\n", (flags & FD_CLOEXEC ? "on" : "off"));
}
