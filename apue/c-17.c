#include <stdio.h>
#include <poll.h>

void sleep_us(unsigned int nusecs)
{
	int		timeout;

	if ((timeout = nusecs / 1000) <= 0)
		timeout = 1;

	poll(NULL, 0, timeout);
}
