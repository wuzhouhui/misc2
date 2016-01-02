#include "apue.h"
#include <termios.h>

int main()
{
	struct termios  term;
	long            vdisable;

	if (isatty(STDIN_FILENO) == 0)
		err_quit("standard input is not a terminal device");
	if ((vdisable = fpathconf(STDIN_FILENO, _PC_VDISABLE)) < 0)
		err_quit("fpathconf error or _PC_VDISABLE not in effect");
	if (tcgetattr(STDIN_FILENO, &term) < 0)
		err_sys("tcgetattr error");
	term.c_cc[VINTR]    = vdisable;
	term.c_cc[VEOF]     = 2;
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &term) < 0)
		err_sys("tcsetattr error");
	return 0;
}
