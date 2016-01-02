#include "apue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>

struct mymesg {
	long mtype;
	void *mtext;
};

#define NLOOPS	5

int main(void)
{
	int		i;
	int		fd;
	int		msgid;
	char		fname[] = "message";
	key_t		msgkey = (key_t)0;
	struct mymesg msg = { 
		.mtype	= 0,
		.mtext	= fname
	};

	for (i = 0; i < NLOOPS; i++) {
		if ((msgid = msgget(msgkey, 0600)) == -1)
			err_sys("msgget error");
		printf("%d\n", msgid);
		if (msgctl(msgid, IPC_RMID, NULL) == -1)
			err_sys("msgctl error");
	}

	for (i = 0; i < NLOOPS; i++) {
		if ((msgid = msgget(IPC_PRIVATE, 0600)) == -1)
			err_sys("msgget error");
		if (msgsnd(msgid, fname, strlen(fname), IPC_NOWAIT) == -1)
			err_sys("msgsnd error");
	}

	exit(0);
}
