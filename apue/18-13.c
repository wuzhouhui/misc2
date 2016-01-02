#include <termios.h>

int isatty(int fd)
{
	struct termios ts;

	return (tcgetattr(fd, &ts) != -1);
}
