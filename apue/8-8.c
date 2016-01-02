#include "apue.h"
#include <sys/wait.h>

int main(void)
{
    pid_t pid;
    if ((pid = fork()) == 0) {
        if ((pid = fork()) > 0)
            exit(0);
        sleep(2);
        printf("second child, parent pid = %ld\n", (long)getppid());
        exit(0);
    }

    waitpid(pid, NULL, 0);
    exit(0);
}
