#include "apue.h"
#include <termios.h>

int main()
{
	struct termios term;

	if (tcgetattr(STDIN_FILENO, &term) < 0)
		err_sys("tcgetattr error");

	switch (term.c_cflag & CSIZE) {	/* CSIZE: character size mask */
	case CS5:
		printf("5 bit per byte\n");
		break;
	case CS6:
		printf("6 bit per byte\n");
		break;
	case CS7:
		printf("7 bit per byte\n");
		break;
	case CS8:
		printf("8 bit per byte\n");
		break;
	default:
		printf("unknown bit\n");
	}
	return 0;
}
