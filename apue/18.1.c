#include "apue.h"

extern int tty_raw(int);

int main(void)
{
	if (tty_raw(STDIN_FILENO) < 0)
		err_sys("tty_raw() error");
	exit(0);
}
