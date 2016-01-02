#include "apue.h"
#include <errno.h>
#include <fcntl.h>
#include <sys/wait.h>

void set_fl(int fd, int flags)
{
	int val;

	if ((val = fcntl(fd, F_GETFL, 0)) < 0)
		err_sys("fcntl F_GETFL error");
	val |= flags;
	if (fcntl(fd, F_SETFL, val) < 0)
		err_sys("fcntl F_SETFL error");
}

int lock_reg(int fd, int cmd, int type, off_t offset, int whence, off_t len)
{
	struct flock lock;

	lock.l_type	= type;
	lock.l_start	= offset;
	lock.l_whence	= whence;
	lock.l_len	= len;

	return (fcntl(fd, cmd, &lock));
}
static volatile sig_atomic_t sigflag; // set nozero by sig handler
static sigset_t newmask, oldmask, zeromask;

// one signal handler for SIGUSR1 and SIGUSR2
static void sig_usr(int signo) 
{
	sigflag = 1;
}

void TELL_WAIT(void)
{
	if (signal(SIGUSR1, sig_usr) == SIG_ERR)
		err_sys("signal(SIGUSR1) error");
	if (signal(SIGUSR2, sig_usr) == SIG_ERR)
		err_sys("signal(SIGUSR2) error");

	sigemptyset(&zeromask);
	sigemptyset(&newmask);
	sigaddset(&newmask, SIGUSR1);
	sigaddset(&newmask, SIGUSR2);

	// block SIGUSR1 and SIGUSR2, and save current signal
	if (sigprocmask(SIG_BLOCK, &newmask, &oldmask) < 0)
		err_sys("SIG_BLOCK error\n");
}

void TELL_PARENT(pid_t pid)
{
	kill(pid, SIGUSR2);
}

void WAIT_PARENT(void)
{
	while (sigflag == 0)
		sigsuspend(&zeromask);
	sigflag = 0;

	// reset signal mask to orignal value
	if (sigprocmask(SIG_SETMASK, &oldmask, NULL) < 0)
		err_sys("SIG_SETMASK error");
}

void TELL_CHILD(pid_t pid)
{
	kill(pid, SIGUSR1);
}

void WAIT_CHILD(void)
{
	while (sigflag == 0)
		sigsuspend(&zeromask);
	sigflag = 0;

	// reset signal mask to orignal value
	if (sigprocmask(SIG_SETMASK, &oldmask, NULL) < 0)
		err_sys("SIG_SETMASK error");
}
int main(int argc, char *argv[])
{
	int		fd;
	pid_t		pid;
	char		buf[5];
	struct stat	statbuf;

	if (argc != 2) {
		fprintf(stderr, "usage: %s filename\n", argv[0]);
		exit(1);
	}
	if ((fd = open(argv[1], O_RDWR | O_CREAT | O_TRUNC, FILE_MODE)) < 0)
		err_sys("open error");
	if (write(fd, "abcdef", 6) != 6)
		err_sys("write error");

	if (fstat(fd, &statbuf) < 0)
		err_sys("fstat error");
	if (fchmod(fd, (statbuf.st_mode & ~S_IXGRP) | S_ISGID) < 0)
		err_sys("fchmod error");

	TELL_WAIT();

	if ((pid = fork()) < 0) {
		err_sys("fork error");
	} else if (pid > 0) {
		if (write_lock(fd, 0, SEEK_SET, 0) < 0)
			err_sys("write_lock error");
		TELL_CHILD(pid);
		if (waitpid(pid, NULL, 0) < 0)
			err_sys("waitpid error");
	} else {
		WAIT_PARENT();
		set_fl(fd, O_NONBLOCK);
		if (read_lock(fd, 0, SEEK_SET, 0) != -1)
			err_sys("child: read_lock succeeded");
		printf("read_lock of already-locked region returns %d\n",
				errno);

		if (lseek(fd, 0, SEEK_SET) == -1)
			err_sys("lseek error");
		if (read(fd, buf, 2) < 0)
			err_ret("read failed (mandatory locking works)");
		else 
			printf("read OK(no mandatory locking), buf = %2.2s\n",
					buf);
	}
	exit(0);
}
