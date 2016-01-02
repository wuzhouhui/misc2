#include "apue.h"
#include <sys/shm.h>

#define NLOOPS	100
#define SIZE	sizeof(long) /* size of shared memory area */

static int update(long *ptr)
{
	return (*ptr)++;
}

int main(void)
{
	int	fd, i, counter;
	pid_t	pid;
	void	*shmptr;
	int	shmid;

	if ((shmid = shmget(IPC_PRIVATE, SIZE, 0600)) == -1)
		err_sys("shmget error");
	if ((shmptr = shmat(shmid, 0, 0)) == (void *)-1)
		err_sys("shmat error");
	*(long *)shmptr = 0;

	TELL_WAIT();

	if ((pid = fork()) < 0) {
		err_sys("fork error");
	} else if (pid > 0) {
		for (i = 0; i < NLOOPS; i += 2) {
			if ((counter = update((long *)shmptr) != i))
				err_quit("parent: expected %d, got %d",
						i, counter);
			printf("%d ", counter);
			TELL_CHILD(pid);
			WAIT_CHILD();
		}
	} else {
		for (i = 1; i < NLOOPS + 1; i += 2) {
			WAIT_PARENT();

			if ((counter = update((long *)shmptr)) != i)
				err_quit("child: expected %d, got %d", 
						i, counter);
			printf("%d ", counter);
			TELL_PARENT(getppid());
		}
	}
}
